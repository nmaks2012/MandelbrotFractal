#pragma once

#include <stdexec/execution.hpp>

#include "types.hpp"

template <typename Receiver>
struct MandelbrotOperationState {
    Receiver receiver_;
    mandelbrot::ViewPort viewport_;
    RenderSettings settings_;
    PixelRegion region_;

    template <typename R>
    explicit MandelbrotOperationState(R &&r, mandelbrot::ViewPort viewport, RenderSettings settings, PixelRegion region)
        : receiver_{std::forward<R>(r)}, viewport_{viewport}, settings_{settings}, region_{region} {}

    void start() noexcept {
        try {
            // Вычисляем множество Мандельброта для заданной области
            PixelMatrix pixel_data(region_.end_row - region_.start_row,
                                   std::vector<std::uint32_t>(region_.end_col - region_.start_col));
            ColorMatrix color_data(region_.end_row - region_.start_row,
                                   std::vector<mandelbrot::RgbColor>(region_.end_col - region_.start_col));

            for (std::uint32_t y = region_.start_row; y < region_.end_row; ++y) {
                for (std::uint32_t x = region_.start_col; x < region_.end_col; ++x) {
                    const auto complex_point =
                        mandelbrot::Pixel2DToComplex(x, y, viewport_, settings_.width, settings_.height);
                    const auto iterations = mandelbrot::CalculateIterationsForPoint(
                        complex_point, settings_.max_iterations, settings_.escape_radius);
                    const auto color = mandelbrot::IterationsToColor(iterations, settings_.max_iterations);

                    const auto local_y = y - region_.start_row;
                    const auto local_x = x - region_.start_col;
                    pixel_data[local_y][local_x] = iterations;
                    color_data[local_y][local_x] = color;
                }
            }

            // Создаем результат рендеринга
            RenderResult result{
                .pixel_data = std::move(pixel_data),
                .color_data = std::move(color_data),
                .viewport = viewport_,
                .settings = settings_,
                .render_time = std::chrono::milliseconds{0}  // Время рендеринга можно добавить позже
            };

            // Отправляем результат получателю
            stdexec::set_value(std::move(receiver_), std::move(result));
        } catch (...) {
            stdexec::set_error(std::move(receiver_), std::current_exception());
        }
    }
};

template <typename Receiver>
struct MandelbrotSender {
    mandelbrot::ViewPort viewport_;
    RenderSettings settings_;
    PixelRegion region_;

    using completion_signatures =
        stdexec::completion_signatures<stdexec::set_value_t(RenderResult), stdexec::set_error_t(std::exception_ptr),
                                       stdexec::set_stopped_t()>;

    template <typename Env>
    auto get_completion_signatures(Env) const -> completion_signatures {
        return {};
    }

    template <typename R>
    auto connect(R &&r) {
        return MandelbrotOperationState<std::decay_t<R>>{std::forward<R>(r), viewport_, settings_, region_};
    }
};

// Специализация для void
template <>
struct MandelbrotSender<void> {
    mandelbrot::ViewPort viewport_;
    RenderSettings settings_;
    PixelRegion region_;

    using completion_signatures =
        stdexec::completion_signatures<stdexec::set_value_t(RenderResult), stdexec::set_error_t(std::exception_ptr),
                                       stdexec::set_stopped_t()>;

    template <typename Env>
    auto get_completion_signatures(Env) const -> completion_signatures {
        return {};
    }

    template <typename R>
    auto connect(R &&r) {
        return MandelbrotOperationState<std::decay_t<R>>{std::forward<R>(r), viewport_, settings_, region_};
    }
};

[[nodiscard]] inline auto MakeMandelbrotSender(mandelbrot::ViewPort viewport, RenderSettings settings,
                                               PixelRegion region) {
    return MandelbrotSender<void>{viewport, settings, region};
}

// Включаем поддержку sender для MandelbrotSender
namespace stdexec {
template <>
inline constexpr bool enable_sender<MandelbrotSender<void>> = true;
}
