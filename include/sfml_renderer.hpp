#pragma once

#include <SFML/Graphics.hpp>
#include <stdexec/execution.hpp>

#include "types.hpp"

class SFMLRender {
public:
    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        RenderResult render_result_;
        sf::Image &image_;
        sf::Texture &texture_;
        sf::Sprite &sprite_;
        sf::RenderWindow &window_;
        RenderSettings render_settings_;

        template <typename R>
        explicit OperationState(R &&r, RenderResult render_result, sf::Image &image, sf::Texture &texture,
                                sf::Sprite &sprite, sf::RenderWindow &window, RenderSettings render_settings)
            : receiver_{std::forward<R>(r)}, render_result_{std::move(render_result)}, image_{image}, texture_{texture},
              sprite_{sprite}, window_{window}, render_settings_{render_settings} {}

        void start() noexcept {
            try {
                // Обновляем изображение с результатами рендеринга
                for (std::uint32_t y = 0; y < render_result_.color_data.size(); ++y) {
                    for (std::uint32_t x = 0; x < render_result_.color_data[y].size(); ++x) {
                        const auto &color = render_result_.color_data[y][x];
                        image_.setPixel(x, y, sf::Color{color.r, color.g, color.b});
                    }
                }

                // Обновляем текстуру и спрайт
                texture_.update(image_);
                sprite_.setTexture(texture_);

                // Очищаем окно и отрисовываем спрайт
                window_.clear(sf::Color::Black);
                window_.draw(sprite_);
                window_.display();

                // Отправляем сигнал о завершении рендеринга
                stdexec::set_value(std::move(receiver_));
            } catch (...) {
                stdexec::set_error(std::move(receiver_), std::current_exception());
            }
        }
    };

    RenderResult render_result_;
    sf::Image &image_;
    sf::Texture &texture_;
    sf::Sprite &sprite_;
    sf::RenderWindow &window_;
    RenderSettings render_settings_;

    using completion_signatures =
        stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_error_t(std::exception_ptr),
                                       stdexec::set_stopped_t()>;

    SFMLRender(RenderResult render_result, sf::Image &image, sf::Texture &texture, sf::Sprite &sprite,
               sf::RenderWindow &window, RenderSettings render_settings)
        : render_result_(std::move(render_result)), image_{image}, texture_{texture}, sprite_{sprite}, window_{window},
          render_settings_{render_settings} {}

    template <typename Env>
    auto get_completion_signatures(Env) const -> completion_signatures {
        return {};
    }

    template <typename Receiver>
    auto connect(Receiver &&r) {
        return OperationState<std::decay_t<Receiver>>{
            std::forward<Receiver>(r), std::move(render_result_), image_, texture_, sprite_, window_, render_settings_};
    }
};
