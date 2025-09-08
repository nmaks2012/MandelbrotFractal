#pragma once

#include <algorithm>
#include <array>
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
        auto sched = thread_pool_.get_scheduler();

        if constexpr (N == 0) {
            // Если N=0, нет работы для выполнения, возвращаем сендер с пустым результатом.
            return stdexec::just(RenderResult{});
        } else {

            // Разделение экрана на N полос.
            std::array<PixelRegion, N> regions;

            const std::uint32_t strip_height = settings.height / N;
            const std::uint32_t remainder = settings.height % N;
            std::uint32_t current_row = 0;

            for (size_t i = 0; i < N; ++i) {
                std::uint32_t height = strip_height + (i < remainder ? 1 : 0);
                regions[i] = {current_row, current_row + height, 0, settings.width};
                current_row += height;
            }

            // Планирование (schedule) и объединение сендеров.
            auto create_when_all = [&]<size_t... I>(std::index_sequence<I...>) {
                return stdexec::when_all((stdexec::on(sched, MakeMandelbrotSender(viewport, settings, regions[I])))...);
            };

            auto all_senders = create_when_all(std::make_index_sequence<N>{});

            // После завершения всех задач в `when_all`, объединяем их результаты в `then`.
            return all_senders | stdexec::then([regions, viewport, settings](auto &&...results) {
                       PixelMatrix full_pixel_data(settings.height, std::vector<std::uint32_t>(settings.width));
                       ColorMatrix full_color_data(settings.height, std::vector<mandelbrot::RgbColor>(settings.width));

                       size_t i = 0;
                       // Лямбда для слияния результата из одного региона в итоговое изображение.
                       auto merge_one_result = [&](auto &&result) {
                           const auto &region = regions[i];
                           for (size_t y = 0; y < result.pixel_data.size(); ++y) {
                               const auto dest_y = region.start_row + y;
                               if (dest_y < settings.height) {

                                   std::ranges::copy(result.pixel_data[y], full_pixel_data[dest_y].begin());
                                   std::ranges::copy(result.color_data[y], full_color_data[dest_y].begin());
                               }
                           }
                           i++;
                       };

                       // Вызываем `merge_one_result` для каждого из N результатов.
                       (merge_one_result(std::forward<decltype(results)>(results)), ...);

                       return RenderResult{.pixel_data = std::move(full_pixel_data),
                                           .color_data = std::move(full_color_data),
                                           .viewport = viewport,
                                           .settings = settings,
                                           .render_time = std::chrono::milliseconds{0}};
                   });
        }
    }
};
