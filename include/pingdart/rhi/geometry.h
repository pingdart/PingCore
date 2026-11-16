#pragma once

#include <cmath>
#include <algorithm>
#include <array>

namespace pingdart::rhi {

struct Point {
    float x{0.0f};
    float y{0.0f};

    constexpr bool operator==(const Point&) const noexcept = default;

    constexpr Point operator+(const Point& other) const noexcept {
        return Point{x + other.x, y + other.y};
    }
    constexpr Point operator-(const Point& other) const noexcept {
        return Point{x - other.x, y - other.y};
    }
};

struct Size {
    float width{0.0f};
    float height{0.0f};

    constexpr bool operator==(const Size&) const noexcept = default;

    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return width <= 0.0f || height <= 0.0f;
    }
};

struct Rect {
    float left{0.0f};
    float top{0.0f};
    float right{0.0f};
    float bottom{0.0f};

    constexpr bool operator==(const Rect&) const noexcept = default;

    static constexpr Rect from_xywh(float x, float y, float w, float h) noexcept {
        return Rect{x, y, x + w, y + h};
    }

    static constexpr Rect from_ltrb(float l, float t, float r, float b) noexcept {
        return Rect{l, t, r, b};
    }

    [[nodiscard]] constexpr float x() const noexcept { return left; }
    [[nodiscard]] constexpr float y() const noexcept { return top; }
    [[nodiscard]] constexpr float width() const noexcept { return std::max(0.0f, right - left); }
    [[nodiscard]] constexpr float height() const noexcept { return std::max(0.0f, bottom - top); }

    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return left >= right || top >= bottom;
    }

    [[nodiscard]] constexpr bool contains(Point p) const noexcept {
        return p.x >= left && p.x <= right && p.y >= top && p.y <= bottom;
    }

    [[nodiscard]] constexpr bool intersects(const Rect& other) const noexcept {
        return !(left >= other.right || right <= other.left ||
                 top >= other.bottom || bottom <= other.top);
    }

    [[nodiscard]] constexpr Rect union_with(const Rect& other) const noexcept {
        if (is_empty()) return other;
        if (other.is_empty()) return *this;
        return Rect{
            std::min(left, other.left),
            std::min(top, other.top),
            std::max(right, other.right),
            std::max(bottom, other.bottom)
        };
    }

    [[nodiscard]] constexpr Rect intersect_with(const Rect& other) const noexcept {
        if (!intersects(other)) return Rect{};
        return Rect{
            std::max(left, other.left),
            std::max(top, other.top),
            std::min(right, other.right),
            std::min(bottom, other.bottom)
        };
    }
};

struct CornerRadii {
    float top_left{0.0f};
    float top_right{0.0f};
    float bottom_right{0.0f};
    float bottom_left{0.0f};

    constexpr bool operator==(const CornerRadii&) const noexcept = default;

    static constexpr CornerRadii all(float r) noexcept {
        return CornerRadii{r, r, r, r};
    }
};

struct RRect {
    Rect rect{};
    CornerRadii radii{};

    constexpr bool operator==(const RRect&) const noexcept = default;

    static constexpr RRect from_rect_and_radius(Rect r, float radius) noexcept {
        return RRect{r, CornerRadii::all(radius)};
    }

    static constexpr RRect from_rect_and_corners(Rect r, float tl, float tr, float br, float bl) noexcept {
        return RRect{r, CornerRadii{tl, tr, br, bl}};
    }

    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return rect.is_empty();
    }
};

// 3x3 2D Affine Transformation Matrix
// [ m00 m01 m02 ]   [ x ]
// [ m10 m11 m12 ] * [ y ]
// [  0   0   1  ]   [ 1 ]
struct Matrix2D {
    float m00{1.0f}; float m01{0.0f}; float m02{0.0f};
    float m10{0.0f}; float m11{1.0f}; float m12{0.0f};

    constexpr bool operator==(const Matrix2D&) const noexcept = default;

    static constexpr Matrix2D identity() noexcept {
        return Matrix2D{};
    }

    static constexpr Matrix2D translation(float dx, float dy) noexcept {
        Matrix2D m{};
        m.m02 = dx;
        m.m12 = dy;
        return m;
    }

    static constexpr Matrix2D scaling(float sx, float sy) noexcept {
        Matrix2D m{};
        m.m00 = sx;
        m.m11 = sy;
        return m;
    }

    static Matrix2D rotation(float radians) noexcept {
        float c = std::cos(radians);
        float s = std::sin(radians);
        Matrix2D m{};
        m.m00 = c;  m.m01 = -s;
        m.m10 = s;  m.m11 = c;
        return m;
    }

    [[nodiscard]] constexpr Point map_point(Point p) const noexcept {
        return Point{
            m00 * p.x + m01 * p.y + m02,
            m10 * p.x + m11 * p.y + m12
        };
    }

    [[nodiscard]] Rect map_rect(const Rect& r) const noexcept {
        if (r.is_empty()) return r;
        Point p0 = map_point(Point{r.left, r.top});
        Point p1 = map_point(Point{r.right, r.top});
        Point p2 = map_point(Point{r.left, r.bottom});
        Point p3 = map_point(Point{r.right, r.bottom});

        float min_x = std::min({p0.x, p1.x, p2.x, p3.x});
        float max_x = std::max({p0.x, p1.x, p2.x, p3.x});
        float min_y = std::min({p0.y, p1.y, p2.y, p3.y});
        float max_y = std::max({p0.y, p1.y, p2.y, p3.y});

        return Rect::from_ltrb(min_x, min_y, max_x, max_y);
    }

    [[nodiscard]] constexpr Matrix2D operator*(const Matrix2D& o) const noexcept {
        return Matrix2D{
            m00 * o.m00 + m01 * o.m10,
            m00 * o.m01 + m01 * o.m11,
            m00 * o.m02 + m01 * o.m12 + m02,

            m10 * o.m00 + m11 * o.m10,
            m10 * o.m01 + m11 * o.m11,
            m10 * o.m02 + m11 * o.m12 + m12
        };
    }
};

} // namespace pingdart::rhi
