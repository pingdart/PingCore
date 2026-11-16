#include <pingdart/text/glyph_atlas.h>
#include <algorithm>
#include <cstring>

namespace pingdart::text {

GlyphAtlas::GlyphAtlas(uint32_t width, uint32_t height)
    : width_(width), height_(height), packer_(width, height) {
    pixels_.resize(static_cast<size_t>(width_) * static_cast<size_t>(height_), 0);
}

void GlyphAtlas::clear() {
    std::fill(pixels_.begin(), pixels_.end(), 0);
    packer_.clear();
    dirty_regions_.clear();
    dirty_regions_.push_back(AtlasRect{0, 0, width_, height_});
}

bool GlyphAtlas::add_glyph(const GlyphBitmap& bitmap, AtlasGlyph& out_glyph, uint64_t generation) {
    out_glyph.glyph_id = bitmap.glyph_id;
    out_glyph.metrics = bitmap.metrics;
    out_glyph.last_used_generation = generation;

    uint32_t w = bitmap.metrics.width;
    uint32_t h = bitmap.metrics.height;

    if (w == 0 || h == 0 || bitmap.pixels.empty()) {
        out_glyph.rect = AtlasRect{0, 0, 0, 0};
        out_glyph.u0 = 0.0f;
        out_glyph.v0 = 0.0f;
        out_glyph.u1 = 0.0f;
        out_glyph.v1 = 0.0f;
        return true;
    }

    uint32_t out_x = 0;
    uint32_t out_y = 0;
    if (!packer_.allocate(w, h, out_x, out_y)) {
        return false;
    }

    // Copy pixels into atlas texture buffer
    for (uint32_t row = 0; row < h; ++row) {
        const uint8_t* src = bitmap.pixels.data() + (row * w);
        uint8_t* dst = pixels_.data() + ((out_y + row) * width_ + out_x);
        std::memcpy(dst, src, w);
    }

    out_glyph.rect = AtlasRect{out_x, out_y, w, h};
    out_glyph.u0 = static_cast<float>(out_x) / static_cast<float>(width_);
    out_glyph.v0 = static_cast<float>(out_y) / static_cast<float>(height_);
    out_glyph.u1 = static_cast<float>(out_x + w) / static_cast<float>(width_);
    out_glyph.v1 = static_cast<float>(out_y + h) / static_cast<float>(height_);

    dirty_regions_.push_back(out_glyph.rect);
    return true;
}

std::vector<AtlasRect> GlyphAtlas::consume_dirty_regions() {
    auto regions = std::move(dirty_regions_);
    dirty_regions_.clear();
    return regions;
}

} // namespace pingdart::text
