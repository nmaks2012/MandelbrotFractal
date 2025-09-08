#pragma once

#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "mandelbrot_sender.hpp"
#include "types.hpp"

class MandelbrotRenderer {
private:
    exec::static_thread_pool thread_pool_;

public:
    explicit MandelbrotRenderer(std::uint32_t num_threads = std::thread::hardware_concurrency())
        : thread_pool_{num_threads} {}

    template <size_t N>
    [[nodiscard]] auto RenderAsync(mandelbrot::ViewPort viewport, RenderSettings settings) {
        const std::uint32_t rows_per_task = settings.height / N;

        // Создаем sender для вычисления одной области
        auto create_region_sender = [this, viewport, settings](PixelRegion region) {
            return stdexec::on(
                thread_pool_.get_scheduler(),
                stdexec::just(viewport, settings, region) |
                    stdexec::then([this](mandelbrot::ViewPort vp, RenderSettings s, PixelRegion r) {
                        // Вычисляем множество Мандельброта для заданной области
                        PixelMatrix pixel_data(r.end_row - r.start_row,
                                               std::vector<std::uint32_t>(r.end_col - r.start_col));
                        ColorMatrix color_data(r.end_row - r.start_row,
                                               std::vector<mandelbrot::RgbColor>(r.end_col - r.start_col));

                        for (std::uint32_t y = r.start_row; y < r.end_row; ++y) {
                            for (std::uint32_t x = r.start_col; x < r.end_col; ++x) {
                                const auto complex_point = mandelbrot::Pixel2DToComplex(x, y, vp, s.width, s.height);
                                const auto iterations = mandelbrot::CalculateIterationsForPoint(
                                    complex_point, s.max_iterations, s.escape_radius);
                                const auto color = mandelbrot::IterationsToColor(iterations, s.max_iterations);

                                const auto local_y = y - r.start_row;
                                const auto local_x = x - r.start_col;
                                pixel_data[local_y][local_x] = iterations;
                                color_data[local_y][local_x] = color;
                            }
                        }

                        return RenderResult{.pixel_data = std::move(pixel_data),
                                            .color_data = std::move(color_data),
                                            .viewport = vp,
                                            .settings = s,
                                            .render_time = std::chrono::milliseconds{0}};
                    }));
        };

        // Создаем сендеры для всех областей
        if constexpr (N == 1) {
            PixelRegion region{0, settings.height, 0, settings.width};
            return create_region_sender(region);
        } else {
            PixelRegion region1{0, rows_per_task, 0, settings.width};
            PixelRegion region2{rows_per_task, settings.height, 0, settings.width};
            return stdexec::when_all(create_region_sender(region1), create_region_sender(region2)) |
                   stdexec::then([viewport, settings, rows_per_task](auto &&result1, auto &&result2) {
                       // Объединяем результаты
                       PixelMatrix full_pixel_data(settings.height, std::vector<std::uint32_t>(settings.width));
                       ColorMatrix full_color_data(settings.height, std::vector<mandelbrot::RgbColor>(settings.width));

                       // Копируем данные из первой области
                       for (std::uint32_t y = 0; y < result1.pixel_data.size(); ++y) {
                           for (std::uint32_t x = 0; x < result1.pixel_data[y].size(); ++x) {
                               full_pixel_data[y][x] = result1.pixel_data[y][x];
                               full_color_data[y][x] = result1.color_data[y][x];
                           }
                       }

                       // Копируем данные из второй области
                       for (std::uint32_t y = 0; y < result2.pixel_data.size(); ++y) {
                           for (std::uint32_t x = 0; x < result2.pixel_data[y].size(); ++x) {
                               full_pixel_data[rows_per_task + y][x] = result2.pixel_data[y][x];
                               full_color_data[rows_per_task + y][x] = result2.color_data[y][x];
                           }
                       }

                       return RenderResult{.pixel_data = std::move(full_pixel_data),
                                           .color_data = std::move(full_color_data),
                                           .viewport = viewport,
                                           .settings = settings,
                                           .render_time = std::chrono::milliseconds{0}};
                   });
        }
    }
};
