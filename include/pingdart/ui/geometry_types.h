#pragma once

#include <pingdart/rhi/geometry.h>
#include <pingdart/rhi/paint.h>
#include <algorithm>
#include <cmath>

namespace pingdart::ui {

using Color = pingdart::rhi::Color;
using Size = pingdart::rhi::Size;
using Rect = pingdart::rhi::Rect;
using RRect = pingdart::rhi::RRect;
using CornerRadii = pingdart::rhi::CornerRadii;
using Matrix2D = pingdart::rhi::Matrix2D;

struct Offset {
    float dx{0.0f};
    float dy{0.0f};

    constexpr Offset() noexcept = default;
    constexpr Offset(float x, float y) noexcept : dx(x), dy(y) {}
    constexpr Offset(const pingdart::rhi::Point& p) noexcept : dx(p.x), dy(p.y) {}

    constexpr bool operator==(const Offset&) const noexcept = default;

    constexpr Offset operator+(const Offset& other) const noexcept {
        return Offset{dx + other.dx, dy + other.dy};
    }

    constexpr Offset operator-(const Offset& other) const noexcept {
        return Offset{dx - other.dx, dy - other.dy};
    }

    constexpr Offset& operator+=(const Offset& other) noexcept {
        dx += other.dx;
        dy += other.dy;
        return *this;
    }

    constexpr Offset& operator-=(const Offset& other) noexcept {
        dx -= other.dx;
        dy -= other.dy;
        return *this;
    }

    constexpr Offset operator*(float factor) const noexcept {
        return Offset{dx * factor, dy * factor};
    }

    [[nodiscard]] constexpr pingdart::rhi::Point to_point() const noexcept {
        return pingdart::rhi::Point{dx, dy};
    }

    [[nodiscard]] float distance() const noexcept {
        return std::sqrt(dx * dx + dy * dy);
    }
};

struct EdgeInsets {
    float left{0.0f};
    float top{0.0f};
    float right{0.0f};
    float bottom{0.0f};

    constexpr bool operator==(const EdgeInsets&) const noexcept = default;

    static constexpr EdgeInsets zero() noexcept {
        return EdgeInsets{0.0f, 0.0f, 0.0f, 0.0f};
    }

    static constexpr EdgeInsets all(float value) noexcept {
        return EdgeInsets{value, value, value, value};
    }

    static constexpr EdgeInsets symmetric(float vertical, float horizontal) noexcept {
        return EdgeInsets{horizontal, vertical, horizontal, vertical};
    }

    static constexpr EdgeInsets from_ltrb(float l, float t, float r, float b) noexcept {
        return EdgeInsets{l, t, r, b};
    }

    [[nodiscard]] constexpr float horizontal() const noexcept {
        return left + right;
    }

    [[nodiscard]] constexpr float vertical() const noexcept {
        return top + bottom;
    }

    [[nodiscard]] constexpr Size collapsed_size() const noexcept {
        return Size{horizontal(), vertical()};
    }
};

struct Alignment {
    float x{0.0f}; // -1.0 = left, 0.0 = center, 1.0 = right
    float y{0.0f}; // -1.0 = top, 0.0 = center, 1.0 = bottom

    constexpr Alignment() noexcept = default;
    constexpr Alignment(float ax, float ay) noexcept : x(ax), y(ay) {}

    constexpr bool operator==(const Alignment&) const noexcept = default;

    static constexpr Alignment top_left() noexcept { return Alignment{-1.0f, -1.0f}; }
    static constexpr Alignment top_center() noexcept { return Alignment{0.0f, -1.0f}; }
    static constexpr Alignment top_right() noexcept { return Alignment{1.0f, -1.0f}; }
    static constexpr Alignment center_left() noexcept { return Alignment{-1.0f, 0.0f}; }
    static constexpr Alignment center() noexcept { return Alignment{0.0f, 0.0f}; }
    static constexpr Alignment center_right() noexcept { return Alignment{1.0f, 0.0f}; }
    static constexpr Alignment bottom_left() noexcept { return Alignment{-1.0f, 1.0f}; }
    static constexpr Alignment bottom_center() noexcept { return Alignment{0.0f, 1.0f}; }
    static constexpr Alignment bottom_right() noexcept { return Alignment{1.0f, 1.0f}; }

    [[nodiscard]] constexpr Offset along_offset(const Offset& offset) const noexcept {
        float cx = (1.0f + x) / 2.0f;
        float cy = (1.0f + y) / 2.0f;
        return Offset{offset.dx * cx, offset.dy * cy};
    }

    [[nodiscard]] constexpr Offset along_size(const Size& other) const noexcept {
        float cx = (1.0f + x) / 2.0f;
        float cy = (1.0f + y) / 2.0f;
        return Offset{other.width * cx, other.height * cy};
    }
};

} // namespace pingdart::ui
