#pragma once

#include "geometry_types.h"
#include <limits>
#include <algorithm>
#include <cmath>

namespace pingdart::ui {

class BoxConstraints {
public:
    static constexpr float infinity = std::numeric_limits<float>::infinity();

    float min_width{0.0f};
    float max_width{infinity};
    float min_height{0.0f};
    float max_height{infinity};

    constexpr BoxConstraints() noexcept = default;

    constexpr BoxConstraints(float min_w, float max_w, float min_h, float max_h) noexcept
        : min_width(min_w), max_width(max_w), min_height(min_h), max_height(max_h) {}

    constexpr bool operator==(const BoxConstraints&) const noexcept = default;

    static constexpr BoxConstraints tight(Size size) noexcept {
        return BoxConstraints{size.width, size.width, size.height, size.height};
    }

    static constexpr BoxConstraints tight_for(float width = infinity, float height = infinity) noexcept {
        return BoxConstraints{
            width == infinity ? 0.0f : width,
            width == infinity ? infinity : width,
            height == infinity ? 0.0f : height,
            height == infinity ? infinity : height
        };
    }

    static constexpr BoxConstraints loose(Size size) noexcept {
        return BoxConstraints{0.0f, size.width, 0.0f, size.height};
    }

    static constexpr BoxConstraints expand(float width = infinity, float height = infinity) noexcept {
        return BoxConstraints{
            width == infinity ? infinity : width,
            width == infinity ? infinity : width,
            height == infinity ? infinity : height,
            height == infinity ? infinity : height
        };
    }

    [[nodiscard]] constexpr bool is_tight() const noexcept {
        return has_tight_width() && has_tight_height();
    }

    [[nodiscard]] constexpr bool has_tight_width() const noexcept {
        return min_width >= max_width;
    }

    [[nodiscard]] constexpr bool has_tight_height() const noexcept {
        return min_height >= max_height;
    }

    [[nodiscard]] constexpr bool is_bounded() const noexcept {
        return has_bounded_width() && has_bounded_height();
    }

    [[nodiscard]] constexpr bool has_bounded_width() const noexcept {
        return max_width < infinity;
    }

    [[nodiscard]] constexpr bool has_bounded_height() const noexcept {
        return max_height < infinity;
    }

    [[nodiscard]] bool is_normalized() const noexcept {
        return min_width >= 0.0f &&
               min_width <= max_width &&
               min_height >= 0.0f &&
               min_height <= max_height &&
               !std::isnan(min_width) &&
               !std::isnan(max_width) &&
               !std::isnan(min_height) &&
               !std::isnan(max_height);
    }

    [[nodiscard]] constexpr BoxConstraints loosen() const noexcept {
        return BoxConstraints{0.0f, max_width, 0.0f, max_height};
    }

    [[nodiscard]] constexpr float constrain_width(float width = infinity) const noexcept {
        if (width != width) return min_width;
        return std::clamp(width, min_width, max_width);
    }

    [[nodiscard]] constexpr float constrain_height(float height = infinity) const noexcept {
        if (height != height) return min_height;
        return std::clamp(height, min_height, max_height);
    }

    [[nodiscard]] constexpr Size constrain(Size size) const noexcept {
        return Size{
            constrain_width(size.width),
            constrain_height(size.height)
        };
    }

    [[nodiscard]] constexpr BoxConstraints deflate(const EdgeInsets& edges) const noexcept {
        float horizontal = edges.horizontal();
        float vertical = edges.vertical();
        float deflated_min_w = std::max(0.0f, min_width - horizontal);
        float deflated_max_w = std::max(deflated_min_w, max_width - horizontal);
        float deflated_min_h = std::max(0.0f, min_height - vertical);
        float deflated_max_h = std::max(deflated_min_h, max_height - vertical);
        return BoxConstraints{deflated_min_w, deflated_max_w, deflated_min_h, deflated_max_h};
    }

    [[nodiscard]] constexpr BoxConstraints enforce(const BoxConstraints& other) const noexcept {
        return BoxConstraints{
            std::clamp(min_width, other.min_width, other.max_width),
            std::clamp(max_width, other.min_width, other.max_width),
            std::clamp(min_height, other.min_height, other.max_height),
            std::clamp(max_height, other.min_height, other.max_height)
        };
    }
};

} // namespace pingdart::ui
