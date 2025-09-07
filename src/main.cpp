#include <chrono>
#include <print>
#include <thread>
#include <utility>

#include <SFML/Graphics.hpp>
#include <exec/repeat_effect_until.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "mandelbrot.hpp"
#include "mandelbrot_renderer.hpp"
#include "sfml_events_handler.hpp"
#include "sfml_renderer.hpp"

using namespace std::chrono_literals;
class FrameClock {
public:
    FrameClock() { Reset(); }

    void Reset() noexcept { frame_start_ = std::chrono::steady_clock::now(); }
    auto GetFrameTime() const noexcept { return std::chrono::steady_clock::now() - frame_start_; }

private:
    std::chrono::time_point<std::chrono::steady_clock> frame_start_;
};

class WaitForFPS {
public:
    FrameClock &frame_clock_;
    int target_fps_;

    WaitForFPS(FrameClock &frame_clock, int target_fps) : frame_clock_{frame_clock}, target_fps_{target_fps} {}

    using completion_signatures =
        stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_error_t(std::exception_ptr),
                                       stdexec::set_stopped_t()>;

    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        FrameClock &frame_clock_;
        int target_fps_;

        template <typename R>
        explicit OperationState(R &&r, FrameClock &frame_clock, int target_fps)
            : receiver_{std::forward<R>(r)}, frame_clock_{frame_clock}, target_fps_{target_fps} {}

        void start() noexcept {
            try {
                // Вычисляем время, которое должно пройти для достижения целевого FPS
                const auto target_frame_time = std::chrono::milliseconds{1000 / target_fps_};
                const auto elapsed_time = frame_clock_.GetFrameTime();

                if (elapsed_time < target_frame_time) {
                    // Если прошло недостаточно времени, ждем
                    const auto sleep_time = target_frame_time - elapsed_time;
                    std::this_thread::sleep_for(sleep_time);
                }

                // Сбрасываем таймер для следующего кадра
                frame_clock_.Reset();

                // Отправляем сигнал о завершении
                stdexec::set_value(std::move(receiver_));
            } catch (...) {
                stdexec::set_error(std::move(receiver_), std::current_exception());
            }
        }
    };

    template <typename Receiver>
    friend auto tag_invoke(stdexec::connect_t, WaitForFPS &&self, Receiver &&r) {
        return OperationState<std::decay_t<Receiver>>{std::forward<Receiver>(r), self.frame_clock_, self.target_fps_};
    }

    template <typename Env>
    friend auto tag_invoke(stdexec::get_completion_signatures_t, const WaitForFPS &, Env) -> completion_signatures {
        return {};
    }
};

class MandelbrotApp {
private:
    RenderSettings render_settings_{.width = 800, .height = 600, .max_iterations = 100, .escape_radius = 2.0};

    sf::RenderWindow window_;
    sf::Image image_;
    sf::Texture texture_;
    sf::Sprite sprite_;
    MandelbrotRenderer renderer_;
    AppState state_;

public:
    MandelbrotApp()
        : window_{sf::VideoMode{render_settings_.width, render_settings_.height}, "Mandelbrot Fractal"},
          renderer_{THREAD_POOL_SIZE} {

        image_.create(render_settings_.width, render_settings_.height);
        texture_.create(render_settings_.width, render_settings_.height);

        window_.setKeyRepeatEnabled(false);
    }

    void Run() {
        FrameClock frame_clock;
        sf::Clock zoom_clock;

        while (!state_.should_exit) {
            // Обрабатываем события
            sf::Event event;
            while (window_.pollEvent(event)) {
                switch (event.type) {
                case sf::Event::Closed:
                    state_.should_exit = true;
                    break;
                case sf::Event::MouseButtonPressed:
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        state_.left_mouse_pressed = true;
                        state_.need_rerender = true;
                    } else if (event.mouseButton.button == sf::Mouse::Right) {
                        state_.right_mouse_pressed = true;
                        state_.need_rerender = true;
                    }
                    break;
                case sf::Event::MouseButtonReleased:
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        state_.left_mouse_pressed = false;
                    } else if (event.mouseButton.button == sf::Mouse::Right) {
                        state_.right_mouse_pressed = false;
                    }
                    break;
                case sf::Event::KeyPressed:
                    if (event.key.code == sf::Keyboard::Escape) {
                        state_.should_exit = true;
                    } else if (event.key.code == sf::Keyboard::R) {
                        // Сброс к начальному виду
                        state_.viewport = mandelbrot::ViewPort{-2.5, 1.5, -2.0, 2.0};
                        state_.need_rerender = true;
                    }
                    break;
                default:
                    break;
                }
            }

            // Обрабатываем непрерывный зум
            if ((state_.left_mouse_pressed || state_.right_mouse_pressed) &&
                zoom_clock.getElapsedTime().asMilliseconds() >= 100.0f) {

                sf::Vector2i mouse_pos = sf::Mouse::getPosition(window_);

                if (mouse_pos.x >= 0 && mouse_pos.x < static_cast<int>(render_settings_.width) && mouse_pos.y >= 0 &&
                    mouse_pos.y < static_cast<int>(render_settings_.height)) {

                    // Зум к точке
                    const double target_x =
                        state_.viewport.x_min +
                        (static_cast<double>(mouse_pos.x) / render_settings_.width) * state_.viewport.width();
                    const double target_y =
                        state_.viewport.y_min +
                        (static_cast<double>(mouse_pos.y) / render_settings_.height) * state_.viewport.height();

                    const double zoom_factor = state_.left_mouse_pressed ? 0.8 : 1.25;
                    const double new_width = state_.viewport.width() * zoom_factor;
                    const double new_height = state_.viewport.height() * zoom_factor;

                    state_.viewport.x_min = target_x - new_width / 2.0;
                    state_.viewport.x_max = target_x + new_width / 2.0;
                    state_.viewport.y_min = target_y - new_height / 2.0;
                    state_.viewport.y_max = target_y + new_height / 2.0;
                    state_.need_rerender = true;

                    zoom_clock.restart();
                }
            }

            // Рендерим, если нужно
            if (state_.need_rerender) {
                auto render_sender = renderer_.RenderAsync<THREAD_POOL_SIZE>(state_.viewport, render_settings_);

                // Синхронно ждем результат
                auto result = stdexec::sync_wait(std::move(render_sender));
                if (result.has_value()) {
                    auto render_result = std::get<0>(result.value());

                    // Обновляем изображение
                    for (std::uint32_t y = 0; y < render_result.color_data.size(); ++y) {
                        for (std::uint32_t x = 0; x < render_result.color_data[y].size(); ++x) {
                            const auto &color = render_result.color_data[y][x];
                            image_.setPixel(x, y, sf::Color{color.r, color.g, color.b});
                        }
                    }

                    // Обновляем текстуру и спрайт
                    texture_.update(image_);
                    sprite_.setTexture(texture_);

                    state_.need_rerender = false;
                }
            }

            // Отрисовываем
            window_.clear(sf::Color::Black);
            window_.draw(sprite_);
            window_.display();

            // Ограничиваем FPS
            const auto target_frame_time = std::chrono::milliseconds{1000 / 60};
            const auto elapsed_time = frame_clock.GetFrameTime();

            if (elapsed_time < target_frame_time) {
                const auto sleep_time = target_frame_time - elapsed_time;
                std::this_thread::sleep_for(sleep_time);
            }

            frame_clock.Reset();
        }
    }
};

int main() {
    try {
        MandelbrotApp app;
        app.Run();
    } catch (const std::exception &e) {
        std::println("Error: {}", e.what());
        return 1;
    }
    return 0;
}