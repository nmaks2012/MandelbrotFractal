#include "mandelbrot_sender.hpp"
#include "types.hpp"
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

class MandelbrotSenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        viewport = mandelbrot::ViewPort{-2.0, 2.0, -2.0, 2.0};
        settings = RenderSettings{.width = 50, .height = 50, .max_iterations = 30, .escape_radius = 2.0};
        region = PixelRegion{.start_row = 0, .end_row = 25, .start_col = 0, .end_col = 25};
    }

    mandelbrot::ViewPort viewport;
    RenderSettings settings;
    PixelRegion region;
};

TEST_F(MandelbrotSenderTest, MakeMandelbrotSender_Creation) {
    auto sender = MakeMandelbrotSender(viewport, settings, region);

    // Проверяем, что sender создался
    EXPECT_TRUE(true);  // Если дошли до сюда, значит sender создался без ошибок
}

TEST_F(MandelbrotSenderTest, MandelbrotSender_Execution) {
    auto sender = MakeMandelbrotSender(viewport, settings, region);

    auto result = stdexec::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto render_result = std::get<0>(result.value());

    // Проверяем размеры результата
    EXPECT_EQ(render_result.pixel_data.size(), region.end_row - region.start_row);
    EXPECT_EQ(render_result.color_data.size(), region.end_row - region.start_row);

    if (!render_result.pixel_data.empty()) {
        EXPECT_EQ(render_result.pixel_data[0].size(), region.end_col - region.start_col);
        EXPECT_EQ(render_result.color_data[0].size(), region.end_col - region.start_col);
    }

    // Проверяем, что данные корректны
    for (size_t y = 0; y < render_result.pixel_data.size(); ++y) {
        for (size_t x = 0; x < render_result.pixel_data[y].size(); ++x) {
            EXPECT_LE(render_result.pixel_data[y][x], settings.max_iterations);
            EXPECT_GE(render_result.pixel_data[y][x], 0);
        }
    }
}

TEST_F(MandelbrotSenderTest, MandelbrotSender_DifferentRegions) {
    // Тестируем разные области
    PixelRegion region1{0, 10, 0, 10};
    PixelRegion region2{10, 20, 10, 20};

    auto sender1 = MakeMandelbrotSender(viewport, settings, region1);
    auto sender2 = MakeMandelbrotSender(viewport, settings, region2);

    auto result1 = stdexec::sync_wait(std::move(sender1));
    auto result2 = stdexec::sync_wait(std::move(sender2));

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    auto render_result1 = std::get<0>(result1.value());
    auto render_result2 = std::get<0>(result2.value());

    // Размеры должны соответствовать областям
    EXPECT_EQ(render_result1.pixel_data.size(), 10);
    EXPECT_EQ(render_result1.pixel_data[0].size(), 10);
    EXPECT_EQ(render_result2.pixel_data.size(), 10);
    EXPECT_EQ(render_result2.pixel_data[0].size(), 10);
}

TEST_F(MandelbrotSenderTest, MandelbrotSender_EmptyRegion) {
    // Тестируем пустую область
    PixelRegion empty_region{0, 0, 0, 0};
    auto sender = MakeMandelbrotSender(viewport, settings, empty_region);

    auto result = stdexec::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto render_result = std::get<0>(result.value());

    EXPECT_EQ(render_result.pixel_data.size(), 0);
    EXPECT_EQ(render_result.color_data.size(), 0);
}

TEST_F(MandelbrotSenderTest, MandelbrotSender_DifferentViewports) {
    // Тестируем разные viewport'ы
    auto viewport1 = mandelbrot::ViewPort{-2.0, 2.0, -2.0, 2.0};
    auto viewport2 = mandelbrot::ViewPort{-1.0, 1.0, -1.0, 1.0};

    auto sender1 = MakeMandelbrotSender(viewport1, settings, region);
    auto sender2 = MakeMandelbrotSender(viewport2, settings, region);

    auto result1 = stdexec::sync_wait(std::move(sender1));
    auto result2 = stdexec::sync_wait(std::move(sender2));

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    auto render_result1 = std::get<0>(result1.value());
    auto render_result2 = std::get<0>(result2.value());

    // Результаты должны быть разными
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

TEST_F(MandelbrotSenderTest, MandelbrotSender_DifferentSettings) {
    // Тестируем разные настройки
    auto settings1 = RenderSettings{50, 50, 10, 2.0};
    auto settings2 = RenderSettings{50, 50, 100, 2.0};

    auto sender1 = MakeMandelbrotSender(viewport, settings1, region);
    auto sender2 = MakeMandelbrotSender(viewport, settings2, region);

    auto result1 = stdexec::sync_wait(std::move(sender1));
    auto result2 = stdexec::sync_wait(std::move(sender2));

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    auto render_result1 = std::get<0>(result1.value());
    auto render_result2 = std::get<0>(result2.value());

    // Результаты должны быть разными из-за разных max_iterations
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

TEST_F(MandelbrotSenderTest, MandelbrotSender_Consistency) {
    // Тестируем, что одинаковые параметры дают одинаковый результат
    auto sender1 = MakeMandelbrotSender(viewport, settings, region);
    auto sender2 = MakeMandelbrotSender(viewport, settings, region);

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
