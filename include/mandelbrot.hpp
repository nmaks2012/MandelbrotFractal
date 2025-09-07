#pragma once

#include "mandelbrot_renderer.hpp"

class CalculateMandelbrotAsyncSender {
public:
    explicit CalculateMandelbrotAsyncSender(AppState &state, RenderSettings render_settings,
                                            MandelbrotRenderer &renderer)
        : state_(state), render_settings_{render_settings}, renderer_{renderer} {}

    using completion_signatures =
        stdexec::completion_signatures<stdexec::set_value_t(RenderResult), stdexec::set_error_t(std::exception_ptr),
                                       stdexec::set_stopped_t()>;

    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        AppState &state_;
        RenderSettings render_settings_;
        MandelbrotRenderer &renderer_;

        template <typename R>
        explicit OperationState(R &&r, AppState &state, RenderSettings render_settings, MandelbrotRenderer &renderer)
            : receiver_{std::forward<R>(r)}, state_{state}, render_settings_{render_settings}, renderer_{renderer} {}

        void start() noexcept {
            try {
                // Проверяем, нужен ли рендеринг
                if (!state_.need_rerender) {
                    // Если рендеринг не нужен, отправляем пустой результат
                    RenderResult empty_result{.pixel_data = PixelMatrix{},
                                              .color_data = ColorMatrix{},
                                              .viewport = state_.viewport,
                                              .settings = render_settings_,
                                              .render_time = std::chrono::milliseconds{0}};
                    stdexec::set_value(std::move(receiver_), std::move(empty_result));
                    return;
                }

                // Запускаем асинхронный рендеринг
                auto render_sender = renderer_.RenderAsync<THREAD_POOL_SIZE>(state_.viewport, render_settings_);

                // Подключаем получателя к сендеру рендеринга
                auto operation = stdexec::connect(std::move(render_sender), [this](RenderResult result) {
                    state_.need_rerender = false;  // Сбрасываем флаг после рендеринга
                    stdexec::set_value(std::move(receiver_), std::move(result));
                });

                stdexec::start(operation);
            } catch (...) {
                stdexec::set_error(std::move(receiver_), std::current_exception());
            }
        }
    };

    template <typename Env>
    auto get_completion_signatures(Env) const -> completion_signatures {
        return {};
    }

    template <typename Receiver>
    auto connect(Receiver &&r) {
        return OperationState<std::decay_t<Receiver>>{std::forward<Receiver>(r), state_, render_settings_, renderer_};
    }

private:
    RenderSettings render_settings_;
    MandelbrotRenderer &renderer_;
    AppState &state_;
};
