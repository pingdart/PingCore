#pragma once

#include "i_pipeline_state.h"
#include <cstdint>
#include <algorithm>

namespace pingdart::rhi {

struct Color {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};

    constexpr bool operator==(const Color&) const noexcept = default;

    static constexpr Color from_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) noexcept {
        return Color{
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f
        };
    }

    static constexpr Color from_rgba_f(float r, float g, float b, float a = 1.0f) noexcept {
        return Color{r, g, b, a};
    }

    [[nodiscard]] constexpr Color with_alpha(float new_alpha) const noexcept {
        return Color{r, g, b, std::clamp(new_alpha, 0.0f, 1.0f)};
    }

    [[nodiscard]] constexpr uint32_t to_argb_uint() const noexcept {
        uint32_t ia = static_cast<uint32_t>(std::clamp(a * 255.0f, 0.0f, 255.0f));
        uint32_t ir = static_cast<uint32_t>(std::clamp(r * 255.0f, 0.0f, 255.0f));
        uint32_t ig = static_cast<uint32_t>(std::clamp(g * 255.0f, 0.0f, 255.0f));
        uint32_t ib = static_cast<uint32_t>(std::clamp(b * 255.0f, 0.0f, 255.0f));
        return (ia << 24) | (ir << 16) | (ig << 8) | ib;
    }
};

enum class PaintStyle : uint8_t {
    Fill,
    Stroke
};

enum class StrokeCap : uint8_t {
    Butt,
    Round,
    Square
};

enum class StrokeJoin : uint8_t {
    Miter,
    Round,
    Bevel
};

enum class ClipOp : uint8_t {
    Intersect,
    Difference
};

struct Paint {
    Color color{Color::from_argb(255, 0, 0, 0)};
    PaintStyle style{PaintStyle::Fill};
    float stroke_width{1.0f};
    StrokeCap cap{StrokeCap::Butt};
    StrokeJoin join{StrokeJoin::Miter};
    BlendMode blend_mode{BlendMode::SrcOver};
    bool is_anti_alias{true};

    constexpr bool operator==(const Paint&) const noexcept = default;
};

} // namespace pingdart::rhi
