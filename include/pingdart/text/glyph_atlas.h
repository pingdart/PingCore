#pragma once

#include "glyph_bitmap.h"
#include <pingdart/core/ref_ptr.h>
#include <vector>
#include <cstdint>

namespace pingdart::text {

struct AtlasRect {
    uint32_t x{0};
    uint32_t y{0};
    uint32_t width{0};
    uint32_t height{0};

    constexpr bool operator==(const AtlasRect&) const noexcept = default;
};

struct AtlasGlyph {
    uint32_t glyph_id{0};
    AtlasRect rect{};
    GlyphMetrics metrics{};
    float u0{0.0f};
    float v0{0.0f};
    float u1{0.0f};
    float v1{0.0f};
    uint64_t last_used_generation{0};
};

class SkylinePacker {
public:
    SkylinePacker(uint32_t width, uint32_t height);

    bool allocate(uint32_t width, uint32_t height, uint32_t& out_x, uint32_t& out_y);
    void clear();
    [[nodiscard]] float occupancy() const noexcept;

private:
    struct Node {
        uint32_t x;
        uint32_t y;
        uint32_t width;
    };

    uint32_t width_;
    uint32_t height_;
    std::vector<Node> nodes_;
    uint64_t used_surface_{0};
};

class GlyphAtlas : public core::RefCounted<GlyphAtlas> {
public:
    explicit GlyphAtlas(uint32_t width = 1024, uint32_t height = 1024);
    ~GlyphAtlas() override = default;

    bool add_glyph(const GlyphBitmap& bitmap, AtlasGlyph& out_glyph, uint64_t generation = 0);
    void clear();

    [[nodiscard]] uint32_t width() const noexcept { return width_; }
    [[nodiscard]] uint32_t height() const noexcept { return height_; }
    [[nodiscard]] const uint8_t* pixel_data() const noexcept { return pixels_.data(); }
    [[nodiscard]] size_t pixel_bytes() const noexcept { return pixels_.size(); }

    [[nodiscard]] std::vector<AtlasRect> consume_dirty_regions();
    [[nodiscard]] bool has_dirty_regions() const noexcept { return !dirty_regions_.empty(); }

private:
    uint32_t width_;
    uint32_t height_;
    std::vector<uint8_t> pixels_;
    SkylinePacker packer_;
    std::vector<AtlasRect> dirty_regions_;
};

} // namespace pingdart::text
