#pragma once

#include <pingdart/rhi/paint.h>
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

namespace pingdart::text {

enum class FontWeight : uint32_t {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Normal = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900
};

enum class FontStyle : uint8_t {
    Normal,
    Italic
};

enum class TextDirection : uint8_t {
    LTR,
    RTL
};

enum class TextScript : uint8_t {
    Common,
    Latin,
    Telugu,
    Devanagari,
    Arabic,
    Emoji
};

struct TextStyle {
    std::string font_family{"sans-serif"};
    float font_size{14.0f};
    uint32_t font_weight{static_cast<uint32_t>(FontWeight::Normal)};
    FontStyle font_style{FontStyle::Normal};
    float letter_spacing{0.0f};
    float line_height{1.2f};
    rhi::Color color{rhi::Color::from_argb(255, 0, 0, 0)};

    bool operator==(const TextStyle&) const noexcept = default;
};

struct GlyphPosition {
    uint32_t glyph_id{0};
    float x_offset{0.0f};
    float y_offset{0.0f};
    float x_advance{0.0f};
    float y_advance{0.0f};
    uint32_t cluster{0}; // UTF-8 byte offset of grapheme cluster

    bool operator==(const GlyphPosition&) const noexcept = default;
};

struct FontMetrics {
    float ascent{0.0f};
    float descent{0.0f};
    float leading{0.0f};
    float cap_height{0.0f};
    float x_height{0.0f};
    float underline_position{0.0f};
    float underline_thickness{1.0f};

    [[nodiscard]] constexpr float line_height_total() const noexcept {
        return ascent + descent + leading;
    }
};

struct ShapedTextRun {
    std::vector<GlyphPosition> glyphs;
    std::string font_family;
    TextDirection direction{TextDirection::LTR};
    TextScript script{TextScript::Common};
    float width{0.0f};
    float ascent{0.0f};
    float descent{0.0f};
    float line_height{0.0f};
};

} // namespace pingdart::text
