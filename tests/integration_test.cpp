#include "mandelbrot_renderer.hpp"
#include "mandelbrot_sender.hpp"
#include "types.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <stdexec/execution.hpp>

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        render_settings = RenderSettings{.width = 100, .height = 100, .max_iterations = 50, .escape_radius = 2.0};
        viewport = mandelbrot::ViewPort{-2.0, 2.0, -2.0, 2.0};
        renderer = std::make_unique<MandelbrotRenderer>(2);
    }

    RenderSettings render_settings;
    mandelbrot::ViewPort viewport;
    std::unique_ptr<MandelbrotRenderer> renderer;
};

TEST_F(IntegrationTest, MandelbrotSender_WithRenderer_Consistency) {
    // Тестируем, что MandelbrotSender и MandelbrotRenderer дают одинаковые результаты
    // для одной и той же области

    // Создаем sender для всей области через MandelbrotSender
    PixelRegion full_region{0, render_settings.height, 0, render_settings.width};
    auto sender1 = MakeMandelbrotSender(viewport, render_settings, full_region);

    // Создаем sender через MandelbrotRenderer с одним потоком
    auto sender2 = renderer->RenderAsync<1>(viewport, render_settings);

    auto result1 = stdexec::sync_wait(std::move(sender1));
    auto result2 = stdexec::sync_wait(std::move(sender2));

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    auto render_result1 = std::get<0>(result1.value());
    auto render_result2 = std::get<0>(result2.value());

    // Результаты должны быть одинаковыми
    EXPECT_EQ(render_result1.pixel_data.size(), render_result2.pixel_data.size());
    EXPECT_EQ(render_result1.color_data.size(), render_result2.color_data.size());

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

TEST_F(IntegrationTest, MultiThreadedRenderer_Consistency) {
    // Тестируем, что многопоточный рендеринг дает тот же результат, что и однопоточный
    auto single_thread_sender = renderer->RenderAsync<1>(viewport, render_settings);
    auto multi_thread_sender = renderer->RenderAsync<2>(viewport, render_settings);

    auto result1 = stdexec::sync_wait(std::move(single_thread_sender));
    auto result2 = stdexec::sync_wait(std::move(multi_thread_sender));

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    auto render_result1 = std::get<0>(result1.value());
    auto render_result2 = std::get<0>(result2.value());

    // Результаты должны быть одинаковыми
    EXPECT_EQ(render_result1.pixel_data.size(), render_result2.pixel_data.size());
    EXPECT_EQ(render_result1.color_data.size(), render_result2.color_data.size());

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

TEST_F(IntegrationTest, PartialRegion_Consistency) {
    // Тестируем, что частичные области дают правильный результат
    PixelRegion region1{0, 50, 0, 50};
    PixelRegion region2{50, 100, 50, 100};

    auto sender1 = MakeMandelbrotSender(viewport, render_settings, region1);
    auto sender2 = MakeMandelbrotSender(viewport, render_settings, region2);

    auto result1 = stdexec::sync_wait(std::move(sender1));
    auto result2 = stdexec::sync_wait(std::move(sender2));

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    auto render_result1 = std::get<0>(result1.value());
    auto render_result2 = std::get<0>(result2.value());

    // Проверяем размеры
    EXPECT_EQ(render_result1.pixel_data.size(), 50);
    EXPECT_EQ(render_result1.pixel_data[0].size(), 50);
    EXPECT_EQ(render_result2.pixel_data.size(), 50);
    EXPECT_EQ(render_result2.pixel_data[0].size(), 50);

    // Проверяем, что данные корректны
    for (size_t y = 0; y < render_result1.pixel_data.size(); ++y) {
        for (size_t x = 0; x < render_result1.pixel_data[y].size(); ++x) {
            EXPECT_LE(render_result1.pixel_data[y][x], render_settings.max_iterations);
            EXPECT_GE(render_result1.pixel_data[y][x], 0);
        }
    }

    for (size_t y = 0; y < render_result2.pixel_data.size(); ++y) {
        for (size_t x = 0; x < render_result2.pixel_data[y].size(); ++x) {
            EXPECT_LE(render_result2.pixel_data[y][x], render_settings.max_iterations);
            EXPECT_GE(render_result2.pixel_data[y][x], 0);
        }
    }
}

TEST_F(IntegrationTest, DifferentViewports_Behavior) {
    // Тестируем поведение с разными viewport'ами
    auto viewport1 = mandelbrot::ViewPort{-2.0, 2.0, -2.0, 2.0};
    auto viewport2 = mandelbrot::ViewPort{-1.0, 1.0, -1.0, 1.0};
    auto viewport3 = mandelbrot::ViewPort{0.0, 0.5, 0.0, 0.5};

    auto sender1 = renderer->RenderAsync<1>(viewport1, render_settings);
    auto sender2 = renderer->RenderAsync<1>(viewport2, render_settings);
    auto sender3 = renderer->RenderAsync<1>(viewport3, render_settings);

    auto result1 = stdexec::sync_wait(std::move(sender1));
    auto result2 = stdexec::sync_wait(std::move(sender2));
    auto result3 = stdexec::sync_wait(std::move(sender3));

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_TRUE(result3.has_value());

    auto render_result1 = std::get<0>(result1.value());
    auto render_result2 = std::get<0>(result2.value());
    auto render_result3 = std::get<0>(result3.value());

    // Все результаты должны иметь одинаковые размеры
    EXPECT_EQ(render_result1.pixel_data.size(), render_settings.height);
    EXPECT_EQ(render_result2.pixel_data.size(), render_settings.height);
    EXPECT_EQ(render_result3.pixel_data.size(), render_settings.height);

    // Но содержимое должно быть разным
    bool found_difference_1_2 = false;
    bool found_difference_2_3 = false;

    if (!render_result1.pixel_data.empty() && !render_result2.pixel_data.empty()) {
        for (size_t y = 0; y < std::min(render_result1.pixel_data.size(), render_result2.pixel_data.size()); ++y) {
            for (size_t x = 0; x < std::min(render_result1.pixel_data[y].size(), render_result2.pixel_data[y].size());
                 ++x) {
                if (render_result1.pixel_data[y][x] != render_result2.pixel_data[y][x]) {
                    found_difference_1_2 = true;
                    break;
                }
            }
            if (found_difference_1_2)
                break;
        }
    }

    if (!render_result2.pixel_data.empty() && !render_result3.pixel_data.empty()) {
        for (size_t y = 0; y < std::min(render_result2.pixel_data.size(), render_result3.pixel_data.size()); ++y) {
            for (size_t x = 0; x < std::min(render_result2.pixel_data[y].size(), render_result3.pixel_data[y].size());
                 ++x) {
                if (render_result2.pixel_data[y][x] != render_result3.pixel_data[y][x]) {
                    found_difference_2_3 = true;
                    break;
                }
            }
            if (found_difference_2_3)
                break;
        }
    }

    EXPECT_TRUE(found_difference_1_2);
    EXPECT_TRUE(found_difference_2_3);
}

TEST_F(IntegrationTest, Performance_Comparison) {
    // Простой тест производительности - проверяем, что многопоточность работает
    auto start_time = std::chrono::high_resolution_clock::now();

    auto single_thread_sender = renderer->RenderAsync<1>(viewport, render_settings);
    auto single_result = stdexec::sync_wait(std::move(single_thread_sender));

    auto single_time = std::chrono::high_resolution_clock::now();

    auto multi_thread_sender = renderer->RenderAsync<2>(viewport, render_settings);
    auto multi_result = stdexec::sync_wait(std::move(multi_thread_sender));

    auto multi_time = std::chrono::high_resolution_clock::now();

    ASSERT_TRUE(single_result.has_value());
    ASSERT_TRUE(multi_result.has_value());

    auto single_duration = std::chrono::duration_cast<std::chrono::milliseconds>(single_time - start_time);
    auto multi_duration = std::chrono::duration_cast<std::chrono::milliseconds>(multi_time - single_time);

    // Многопоточная версия должна быть быстрее или такой же (в худшем случае)
    // В реальных условиях многопоточность должна давать преимущество
    EXPECT_LE(multi_duration.count(), single_duration.count() + 100);  // +100ms допуск
}
