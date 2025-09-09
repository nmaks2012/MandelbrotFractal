#include "mandelbrot_renderer.hpp"
#include "types.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <stdexec/execution.hpp>

class MandelbrotRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        render_settings = RenderSettings{.width = 100, .height = 100, .max_iterations = 50, .escape_radius = 2.0};
        viewport = mandelbrot::ViewPort{-2.0, 2.0, -2.0, 2.0};
        renderer = std::make_unique<MandelbrotRenderer>(2);  // 2 потока для тестов
    }

    RenderSettings render_settings;
    mandelbrot::ViewPort viewport;
    std::unique_ptr<MandelbrotRenderer> renderer;
};

TEST_F(MandelbrotRendererTest, RenderAsync_SingleThread) {
    // Тестируем рендеринг с одним потоком
    auto sender = renderer->RenderAsync<1>(viewport, render_settings);

    auto result = stdexec::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto render_result = std::get<0>(result.value());

    // Проверяем размеры результата
    EXPECT_EQ(render_result.pixel_data.size(), render_settings.height);
    EXPECT_EQ(render_result.color_data.size(), render_settings.height);

    if (!render_result.pixel_data.empty()) {
        EXPECT_EQ(render_result.pixel_data[0].size(), render_settings.width);
        EXPECT_EQ(render_result.color_data[0].size(), render_settings.width);
    }

    // Проверяем, что viewport и settings сохранены
    EXPECT_EQ(render_result.viewport.x_min, viewport.x_min);
    EXPECT_EQ(render_result.viewport.x_max, viewport.x_max);
    EXPECT_EQ(render_result.viewport.y_min, viewport.y_min);
    EXPECT_EQ(render_result.viewport.y_max, viewport.y_max);
    EXPECT_EQ(render_result.settings.width, render_settings.width);
    EXPECT_EQ(render_result.settings.height, render_settings.height);
}

TEST_F(MandelbrotRendererTest, RenderAsync_TwoThreads) {
    // Тестируем рендеринг с двумя потоками
    auto sender = renderer->RenderAsync<2>(viewport, render_settings);

    auto result = stdexec::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto render_result = std::get<0>(result.value());

    // Проверяем размеры результата
    EXPECT_EQ(render_result.pixel_data.size(), render_settings.height);
    EXPECT_EQ(render_result.color_data.size(), render_settings.height);

    if (!render_result.pixel_data.empty()) {
        EXPECT_EQ(render_result.pixel_data[0].size(), render_settings.width);
        EXPECT_EQ(render_result.color_data[0].size(), render_settings.width);
    }
}

TEST_F(MandelbrotRendererTest, RenderAsync_Consistency) {
    // Тестируем, что результаты одинаковы при разных количествах потоков
    auto sender1 = renderer->RenderAsync<1>(viewport, render_settings);
    auto sender2 = renderer->RenderAsync<2>(viewport, render_settings);

    auto result1 = stdexec::sync_wait(std::move(sender1));
    auto result2 = stdexec::sync_wait(std::move(sender2));

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    auto render_result1 = std::get<0>(result1.value());
    auto render_result2 = std::get<0>(result2.value());

    // Проверяем, что размеры одинаковы
    EXPECT_EQ(render_result1.pixel_data.size(), render_result2.pixel_data.size());
    EXPECT_EQ(render_result1.color_data.size(), render_result2.color_data.size());

    // Проверяем несколько пикселей на совпадение
    if (!render_result1.pixel_data.empty() && !render_result2.pixel_data.empty()) {
        for (size_t y = 0; y < std::min(render_result1.pixel_data.size(), render_result2.pixel_data.size()); ++y) {
            for (size_t x = 0; x < std::min(render_result1.pixel_data[y].size(), render_result2.pixel_data[y].size());
                 ++x) {
                EXPECT_EQ(render_result1.pixel_data[y][x], render_result2.pixel_data[y][x]);
                EXPECT_EQ(render_result1.color_data[y][x].r, render_result2.color_data[y][x].r);
                EXPECT_EQ(render_result1.color_data[y][x].g, render_result2.color_data[y][x].g);
                EXPECT_EQ(render_result1.color_data[y][x].b, render_result2.color_data[y][x].b);
            }
        }
    }
}

TEST_F(MandelbrotRendererTest, RenderAsync_DifferentViewports) {
    // Тестируем рендеринг с разными viewport'ами
    auto viewport1 = mandelbrot::ViewPort{-2.0, 2.0, -2.0, 2.0};
    auto viewport2 = mandelbrot::ViewPort{-1.0, 1.0, -1.0, 1.0};

    auto sender1 = renderer->RenderAsync<1>(viewport1, render_settings);
    auto sender2 = renderer->RenderAsync<1>(viewport2, render_settings);

    auto result1 = stdexec::sync_wait(std::move(sender1));
    auto result2 = stdexec::sync_wait(std::move(sender2));

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    auto render_result1 = std::get<0>(result1.value());
    auto render_result2 = std::get<0>(result2.value());

    // Результаты должны быть разными из-за разных viewport'ов
    bool found_difference = false;
    if (!render_result1.pixel_data.empty() && !render_result2.pixel_data.empty()) {
        for (size_t y = 0; y < std::min(render_result1.pixel_data.size(), render_result2.pixel_data.size()); ++y) {
            for (size_t x = 0; x < std::min(render_result1.pixel_data[y].size(), render_result2.pixel_data[y].size());
                 ++x) {
                if (render_result1.pixel_data[y][x] != render_result2.pixel_data[y][x]) {
                    found_difference = true;
                    break;
                }
            }
            if (found_difference)
                break;
        }
    }
    EXPECT_TRUE(found_difference);
}

TEST_F(MandelbrotRendererTest, RenderAsync_EmptyRegion) {
    // Тестируем рендеринг с очень маленьким viewport'ом
    auto small_viewport = mandelbrot::ViewPort{0.0, 0.1, 0.0, 0.1};
    auto small_settings = RenderSettings{.width = 10, .height = 10, .max_iterations = 10, .escape_radius = 2.0};

    auto sender = renderer->RenderAsync<1>(small_viewport, small_settings);
    auto result = stdexec::sync_wait(std::move(sender));

    ASSERT_TRUE(result.has_value());
    auto render_result = std::get<0>(result.value());

    EXPECT_EQ(render_result.pixel_data.size(), small_settings.height);
    EXPECT_EQ(render_result.color_data.size(), small_settings.height);
}
