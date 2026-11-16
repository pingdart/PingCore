#pragma once

#include "glyph_atlas.h"
#include "font_face.h"
#include <unordered_map>
#include <list>
#include <functional>

namespace pingdart::text {

struct GlyphKey {
    uint64_t font_id{0};
    uint32_t font_generation{0};
    uint32_t glyph_id{0};
    float font_size{14.0f};
    float dpi_scale{1.0f};
    bool bold{false};
    bool italic{false};

    bool operator==(const GlyphKey& o) const noexcept {
        return font_id == o.font_id &&
               font_generation == o.font_generation &&
               glyph_id == o.glyph_id &&
               font_size == o.font_size &&
               dpi_scale == o.dpi_scale &&
               bold == o.bold &&
               italic == o.italic;
    }
};

struct GlyphKeyHash {
    size_t operator()(const GlyphKey& k) const noexcept {
        size_t h1 = std::hash<uint64_t>{}(k.font_id);
        size_t h2 = std::hash<uint32_t>{}(k.font_generation);
        size_t h3 = std::hash<uint32_t>{}(k.glyph_id);
        size_t h4 = std::hash<float>{}(k.font_size);
        size_t h5 = std::hash<float>{}(k.dpi_scale);
        size_t h6 = (k.bold ? 1 : 0) | (k.italic ? 2 : 0);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5);
    }
};

class GlyphCache {
public:
    explicit GlyphCache(uint32_t atlas_size = 1024);

    const AtlasGlyph* get_or_insert(FontFace& face,
                                    uint32_t glyph_id,
                                    float font_size,
                                    float dpi_scale,
                                    uint64_t current_gpu_generation,
                                    uint64_t completed_gpu_generation);

    [[nodiscard]] const AtlasGlyph* find(const GlyphKey& key) const;

    void invalidate_dpi(float dpi_scale);
    void clear();

    [[nodiscard]] core::RefPtr<GlyphAtlas> atlas() const noexcept { return atlas_; }
    [[nodiscard]] size_t cached_glyph_count() const noexcept { return cache_.size(); }

private:
    core::RefPtr<GlyphAtlas> atlas_;
    std::unordered_map<GlyphKey, AtlasGlyph, GlyphKeyHash> cache_;
    std::list<GlyphKey> lru_order_;
    std::unordered_map<uint64_t, core::RefPtr<FontFace>> registered_faces_;
};

} // namespace pingdart::text
