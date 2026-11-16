#include <pingdart/text/glyph_cache.h>
#include <algorithm>

namespace pingdart::text {

GlyphCache::GlyphCache(uint32_t atlas_size)
    : atlas_(core::make_ref<GlyphAtlas>(atlas_size, atlas_size)) {}

const AtlasGlyph* GlyphCache::find(const GlyphKey& key) const {
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return &it->second;
    }
    return nullptr;
}

const AtlasGlyph* GlyphCache::get_or_insert(FontFace& face,
                                            uint32_t glyph_id,
                                            float font_size,
                                            float dpi_scale,
                                            uint64_t current_gpu_generation,
                                            uint64_t completed_gpu_generation) {
    registered_faces_[face.font_id()] = core::RefPtr<FontFace>(&face);
    GlyphKey key{face.font_id(), face.generation(), glyph_id, font_size, dpi_scale, false, false};

    auto it = cache_.find(key);
    if (it != cache_.end()) {
        it->second.last_used_generation = current_gpu_generation;
        return &it->second;
    }

    face.set_pixel_size(font_size, dpi_scale);
    GlyphBitmap bitmap = face.rasterize_glyph(glyph_id);

    AtlasGlyph ag{};
    if (!atlas_->add_glyph(bitmap, ag, current_gpu_generation)) {
        // Atlas full: GPU fence-safe eviction
        // Preserve in-flight glyphs referenced by active frames (last_used > completed_gpu_gen)
        std::vector<std::pair<GlyphKey, GlyphBitmap>> in_flight_glyphs;

        for (const auto& [k, existing_ag] : cache_) {
            if (existing_ag.last_used_generation > completed_gpu_generation) {
                auto face_it = registered_faces_.find(k.font_id);
                if (face_it != registered_faces_.end() && face_it->second) {
                    face_it->second->set_pixel_size(k.font_size, k.dpi_scale);
                    in_flight_glyphs.emplace_back(k, face_it->second->rasterize_glyph(k.glyph_id));
                }
            }
        }

        // Reset atlas and cache
        atlas_->clear();
        cache_.clear();
        lru_order_.clear();

        // Re-pack in-flight glyphs
        for (const auto& [k, bmp] : in_flight_glyphs) {
            AtlasGlyph restored_ag{};
            if (atlas_->add_glyph(bmp, restored_ag, current_gpu_generation)) {
                cache_[k] = restored_ag;
                lru_order_.push_back(k);
            }
        }

        // Retry allocating the requested new glyph
        if (!atlas_->add_glyph(bitmap, ag, current_gpu_generation)) {
            return nullptr;
        }
    }

    auto [inserted_it, _] = cache_.emplace(key, ag);
    lru_order_.push_back(key);
    return &inserted_it->second;
}

void GlyphCache::invalidate_dpi(float dpi_scale) {
    for (auto it = cache_.begin(); it != cache_.end();) {
        if (it->first.dpi_scale == dpi_scale) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

void GlyphCache::clear() {
    cache_.clear();
    lru_order_.clear();
    atlas_->clear();
}

} // namespace pingdart::text
