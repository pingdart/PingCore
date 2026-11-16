#pragma once

#include <cstdint>
#include <vector>

namespace pingdart::text {

enum class PixelFormat : uint8_t {
    Alpha8,
    RGBA32
};

struct GlyphMetrics {
    int32_t bearing_x{0};
    int32_t bearing_y{0};
    uint32_t width{0};
    uint32_t height{0};
    float advance_x{0.0f};
    float advance_y{0.0f};

    constexpr bool operator==(const GlyphMetrics&) const noexcept = default;
};

struct GlyphBitmap {
    uint32_t glyph_id{0};
    GlyphMetrics metrics{};
    PixelFormat format{PixelFormat::Alpha8};
    std::vector<uint8_t> pixels;

    [[nodiscard]] size_t pitch() const noexcept {
        return metrics.width * (format == PixelFormat::RGBA32 ? 4 : 1);
    }
};

} // namespace pingdart::text
