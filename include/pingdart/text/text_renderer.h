#pragma once

#include "font_face.h"
#include "glyph_cache.h"
#include <pingdart/rhi/geometry.h>
#include <pingdart/rhi/paint.h>
#include <hb.h>
#include <string>
#include <vector>
#include <memory>

namespace pingdart::text {

using Point = pingdart::rhi::Point;
using Rect = pingdart::rhi::Rect;
using Color = pingdart::rhi::Color;

struct GlyphQuad {
    Rect dst_rect;
    Rect uv_rect;
    Color color;
};

struct ShapedRun {
    core::RefPtr<FontFace> face;
    std::vector<GlyphQuad> quads;
    float total_advance_x{0.0f};
    float total_advance_y{0.0f};
};

class TextRenderer {
public:
    explicit TextRenderer(uint32_t atlas_size = 1024);
    ~TextRenderer() = default;

    ShapedRun shape_and_render(FontFace& face,
                              const std::string& text,
                              float font_size,
                              float dpi_scale,
                              Point origin,
                              Color color,
                              hb_script_t script = HB_SCRIPT_INVALID,
                              hb_direction_t direction = HB_DIRECTION_LTR,
                              uint64_t current_gpu_gen = 0,
                              uint64_t completed_gpu_gen = 0);

    [[nodiscard]] GlyphCache& cache() noexcept { return *cache_; }
    [[nodiscard]] const GlyphCache& cache() const noexcept { return *cache_; }

private:
    std::unique_ptr<GlyphCache> cache_;
};

} // namespace pingdart::text
