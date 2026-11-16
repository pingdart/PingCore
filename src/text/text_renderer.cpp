#include <pingdart/text/text_renderer.h>
#include <hb.h>

namespace pingdart::text {

TextRenderer::TextRenderer(uint32_t atlas_size)
    : cache_(std::make_unique<GlyphCache>(atlas_size)) {}

ShapedRun TextRenderer::shape_and_render(FontFace& face,
                                        const std::string& text,
                                        float font_size,
                                        float dpi_scale,
                                        Point origin,
                                        Color color,
                                        hb_script_t script,
                                        hb_direction_t direction,
                                        uint64_t current_gpu_gen,
                                        uint64_t completed_gpu_gen) {
    ShapedRun run;
    run.face = core::RefPtr<FontFace>(&face);
    if (text.empty() || !face.hb_font()) {
        return run;
    }

    face.set_pixel_size(font_size, dpi_scale);

    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, text.c_str(), -1, 0, -1);
    hb_buffer_set_direction(buf, direction);
    if (script != HB_SCRIPT_INVALID) {
        hb_buffer_set_script(buf, script);
    } else {
        hb_buffer_guess_segment_properties(buf);
    }

    hb_shape(face.hb_font(), buf, nullptr, 0);

    unsigned int glyph_count = 0;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    float pen_x = origin.x;
    float pen_y = origin.y;

    run.quads.reserve(glyph_count);

    for (unsigned int i = 0; i < glyph_count; ++i) {
        uint32_t gid = glyph_info[i].codepoint;
        float x_offset = static_cast<float>(glyph_pos[i].x_offset) / 64.0f;
        float y_offset = static_cast<float>(glyph_pos[i].y_offset) / 64.0f;
        float x_advance = static_cast<float>(glyph_pos[i].x_advance) / 64.0f;
        float y_advance = static_cast<float>(glyph_pos[i].y_advance) / 64.0f;

        const AtlasGlyph* ag = cache_->get_or_insert(face, gid, font_size, dpi_scale,
                                                    current_gpu_gen, completed_gpu_gen);
        if (ag && ag->rect.width > 0 && ag->rect.height > 0) {
            float gx = pen_x + x_offset + static_cast<float>(ag->metrics.bearing_x);
            float gy = pen_y - y_offset - static_cast<float>(ag->metrics.bearing_y);
            float gw = static_cast<float>(ag->metrics.width);
            float gh = static_cast<float>(ag->metrics.height);

            GlyphQuad quad{
                Rect::from_xywh(gx, gy, gw, gh),
                Rect::from_ltrb(ag->u0, ag->v0, ag->u1, ag->v1),
                color
            };
            run.quads.push_back(quad);
        }

        pen_x += x_advance;
        pen_y += y_advance;
    }

    run.total_advance_x = pen_x - origin.x;
    run.total_advance_y = pen_y - origin.y;

    hb_buffer_destroy(buf);
    return run;
}

} // namespace pingdart::text
