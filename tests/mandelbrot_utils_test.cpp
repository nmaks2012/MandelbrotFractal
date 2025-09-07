#include "mandelbrot_fractal_utils.hpp"
#include <gtest/gtest.h>

using namespace mandelbrot;

class MandelbrotUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        max_iterations = 100;
        escape_radius = 2.0;
        viewport = ViewPort{-2.5, 1.5, -2.0, 2.0};
        screen_width = 800;
        screen_height = 600;
    }

    std::uint32_t max_iterations;
    double escape_radius;
    ViewPort viewport;
    std::uint32_t screen_width;
    std::uint32_t screen_height;
};

TEST_F(MandelbrotUtilsTest, CalculateIterationsForPoint_InsideSet) {
    // Точка внутри множества Мандельброта (c = 0)
    Complex c{0.0, 0.0};
    auto iterations = CalculateIterationsForPoint(c, max_iterations, escape_radius);
    EXPECT_EQ(iterations, max_iterations);
}

TEST_F(MandelbrotUtilsTest, CalculateIterationsForPoint_OutsideSet) {
    // Точка вне множества Мандельброта (c = 2)
    Complex c{2.0, 0.0};
    auto iterations = CalculateIterationsForPoint(c, max_iterations, escape_radius);
    EXPECT_LT(iterations, max_iterations);
    EXPECT_GT(iterations, 0);
}

TEST_F(MandelbrotUtilsTest, CalculateIterationsForPoint_EdgeCase) {
    // Точка на границе (c = 2)
    Complex c{2.0, 0.0};
    auto iterations = CalculateIterationsForPoint(c, max_iterations, escape_radius);
    EXPECT_LT(iterations, max_iterations);
}

TEST_F(MandelbrotUtilsTest, Pixel2DToComplex_CornerCases) {
    // Левый верхний угол
    auto complex1 = Pixel2DToComplex(0, 0, viewport, screen_width, screen_height);
    EXPECT_DOUBLE_EQ(complex1.real(), viewport.x_min);
    EXPECT_DOUBLE_EQ(complex1.imag(), viewport.y_min);

    // Правый нижний угол
    auto complex2 = Pixel2DToComplex(screen_width - 1, screen_height - 1, viewport, screen_width, screen_height);
    EXPECT_NEAR(complex2.real(), viewport.x_max, 1e-2);
    EXPECT_NEAR(complex2.imag(), viewport.y_max, 1e-2);

    // Центр
    auto complex3 = Pixel2DToComplex(screen_width / 2, screen_height / 2, viewport, screen_width, screen_height);
    EXPECT_DOUBLE_EQ(complex3.real(), (viewport.x_min + viewport.x_max) / 2.0);
    EXPECT_DOUBLE_EQ(complex3.imag(), (viewport.y_min + viewport.y_max) / 2.0);
}

TEST_F(MandelbrotUtilsTest, IterationsToColor_InsideSet) {
    // Точка внутри множества должна быть черной
    auto color = IterationsToColor(max_iterations, max_iterations);
    EXPECT_EQ(color.r, 0);
    EXPECT_EQ(color.g, 0);
    EXPECT_EQ(color.b, 0);
}

TEST_F(MandelbrotUtilsTest, IterationsToColor_OutsideSet) {
    // Точка вне множества должна иметь цвет
    auto color = IterationsToColor(50, max_iterations);
    // Цвет может быть любым, но не все компоненты должны быть нулевыми одновременно
    EXPECT_TRUE(color.r != 0 || color.g != 0 || color.b != 0);
}

TEST_F(MandelbrotUtilsTest, IterationsToColor_ZeroIterations) {
    // Нулевые итерации должны давать цвет
    auto color = IterationsToColor(0, max_iterations);
    // Цвет может быть любым, но не все компоненты должны быть нулевыми одновременно
    EXPECT_TRUE(color.r != 0 || color.g != 0 || color.b != 0);
}

TEST_F(MandelbrotUtilsTest, ViewPort_WidthHeight) {
    EXPECT_DOUBLE_EQ(viewport.width(), 4.0);
    EXPECT_DOUBLE_EQ(viewport.height(), 4.0);
}

TEST_F(MandelbrotUtilsTest, RgbColors_Constants) {
    EXPECT_EQ(RgbColors::BLACK.r, 0);
    EXPECT_EQ(RgbColors::BLACK.g, 0);
    EXPECT_EQ(RgbColors::BLACK.b, 0);
}
