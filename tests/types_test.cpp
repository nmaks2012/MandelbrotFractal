#include "types.hpp"
#include <gtest/gtest.h>

class TypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TypesTest, RenderSettings_DefaultValues) {
    RenderSettings settings;

    EXPECT_EQ(settings.width, 800);
    EXPECT_EQ(settings.height, 600);
    EXPECT_EQ(settings.max_iterations, 100);
    EXPECT_DOUBLE_EQ(settings.escape_radius, 2.0);
}

TEST_F(TypesTest, RenderSettings_CustomValues) {
    RenderSettings settings{.width = 1024, .height = 768, .max_iterations = 200, .escape_radius = 3.0};

    EXPECT_EQ(settings.width, 1024);
    EXPECT_EQ(settings.height, 768);
    EXPECT_EQ(settings.max_iterations, 200);
    EXPECT_DOUBLE_EQ(settings.escape_radius, 3.0);
}

TEST_F(TypesTest, PixelRegion_DefaultValues) {
    PixelRegion region;

    EXPECT_EQ(region.start_row, 0);
    EXPECT_EQ(region.end_row, 0);
    EXPECT_EQ(region.start_col, 0);
    EXPECT_EQ(region.end_col, 0);
}

TEST_F(TypesTest, PixelRegion_CustomValues) {
    PixelRegion region{.start_row = 10, .end_row = 20, .start_col = 5, .end_col = 15};

    EXPECT_EQ(region.start_row, 10);
    EXPECT_EQ(region.end_row, 20);
    EXPECT_EQ(region.start_col, 5);
    EXPECT_EQ(region.end_col, 15);
}

TEST_F(TypesTest, RenderResult_Structure) {
    PixelMatrix pixel_data(2, std::vector<std::uint32_t>(3));
    ColorMatrix color_data(2, std::vector<mandelbrot::RgbColor>(3));
    mandelbrot::ViewPort viewport{-1.0, 1.0, -1.0, 1.0};
    RenderSettings settings{100, 100, 50, 2.0};
    std::chrono::milliseconds render_time{100};

    RenderResult result{.pixel_data = std::move(pixel_data),
                        .color_data = std::move(color_data),
                        .viewport = viewport,
                        .settings = settings,
                        .render_time = render_time};

    EXPECT_EQ(result.pixel_data.size(), 2);
    EXPECT_EQ(result.color_data.size(), 2);
    EXPECT_EQ(result.pixel_data[0].size(), 3);
    EXPECT_EQ(result.color_data[0].size(), 3);
    EXPECT_DOUBLE_EQ(result.viewport.x_min, -1.0);
    EXPECT_DOUBLE_EQ(result.viewport.x_max, 1.0);
    EXPECT_DOUBLE_EQ(result.viewport.y_min, -1.0);
    EXPECT_DOUBLE_EQ(result.viewport.y_max, 1.0);
    EXPECT_EQ(result.settings.width, 100);
    EXPECT_EQ(result.settings.height, 100);
    EXPECT_EQ(result.settings.max_iterations, 50);
    EXPECT_DOUBLE_EQ(result.settings.escape_radius, 2.0);
    EXPECT_EQ(result.render_time.count(), 100);
}

TEST_F(TypesTest, AppState_DefaultValues) {
    AppState state;

    EXPECT_DOUBLE_EQ(state.viewport.x_min, -2.5);
    EXPECT_DOUBLE_EQ(state.viewport.x_max, 1.5);
    EXPECT_DOUBLE_EQ(state.viewport.y_min, -2.0);
    EXPECT_DOUBLE_EQ(state.viewport.y_max, 2.0);
    EXPECT_TRUE(state.need_rerender);
    EXPECT_FALSE(state.left_mouse_pressed);
    EXPECT_FALSE(state.right_mouse_pressed);
    EXPECT_FALSE(state.should_exit);
}

TEST_F(TypesTest, AppState_CustomValues) {
    mandelbrot::ViewPort custom_viewport{-1.0, 1.0, -1.0, 1.0};
    AppState state{.viewport = custom_viewport,
                   .need_rerender = true,
                   .left_mouse_pressed = true,
                   .right_mouse_pressed = false,
                   .should_exit = false};

    EXPECT_DOUBLE_EQ(state.viewport.x_min, -1.0);
    EXPECT_DOUBLE_EQ(state.viewport.x_max, 1.0);
    EXPECT_DOUBLE_EQ(state.viewport.y_min, -1.0);
    EXPECT_DOUBLE_EQ(state.viewport.y_max, 1.0);
    EXPECT_TRUE(state.need_rerender);
    EXPECT_TRUE(state.left_mouse_pressed);
    EXPECT_FALSE(state.right_mouse_pressed);
    EXPECT_FALSE(state.should_exit);
}

TEST_F(TypesTest, ThreadPoolSize_Constant) { EXPECT_EQ(THREAD_POOL_SIZE, 8); }

TEST_F(TypesTest, PixelMatrix_Type) {
    PixelMatrix matrix(2, std::vector<std::uint32_t>(3, 42));

    EXPECT_EQ(matrix.size(), 2);
    EXPECT_EQ(matrix[0].size(), 3);
    EXPECT_EQ(matrix[0][0], 42);
    EXPECT_EQ(matrix[1][2], 42);
}

TEST_F(TypesTest, ColorMatrix_Type) {
    mandelbrot::RgbColor test_color{255, 128, 64};
    ColorMatrix matrix(2, std::vector<mandelbrot::RgbColor>(3, test_color));

    EXPECT_EQ(matrix.size(), 2);
    EXPECT_EQ(matrix[0].size(), 3);
    EXPECT_EQ(matrix[0][0].r, 255);
    EXPECT_EQ(matrix[0][0].g, 128);
    EXPECT_EQ(matrix[0][0].b, 64);
}
