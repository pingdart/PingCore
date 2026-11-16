#include <pingdart/text/font_face.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#include <stdexcept>
#include <cmath>
#include <mutex>

namespace pingdart::text {

namespace {

FT_Library get_ft_library() {
    static FT_Library s_library = nullptr;
    static std::once_flag s_once;
    std::call_once(s_once, []() {
        if (FT_Init_FreeType(&s_library)) {
            throw std::runtime_error("Failed to initialize FreeType library");
        }
    });
    return s_library;
}

} // namespace

core::RefPtr<FontFace> FontFace::load_from_file(const std::string& path, uint32_t face_index) {
    FT_Library ft = get_ft_library();
    FT_Face face = nullptr;
    FT_Error err = FT_New_Face(ft, path.c_str(), static_cast<FT_Long>(face_index), &face);
    if (err) {
        return nullptr;
    }
    return core::adopt_ref(new FontFace(face));
}

core::RefPtr<FontFace> FontFace::load_from_memory(const uint8_t* data, size_t size, uint32_t face_index) {
    if (!data || size == 0) return nullptr;
    FT_Library ft = get_ft_library();
    std::vector<uint8_t> buffer(data, data + size);

    FT_Face face = nullptr;
    FT_Error err = FT_New_Memory_Face(ft, buffer.data(), static_cast<FT_Long>(buffer.size()),
                                      static_cast<FT_Long>(face_index), &face);
    if (err) {
        return nullptr;
    }
    return core::adopt_ref(new FontFace(face, std::move(buffer)));
}

static std::atomic<uint64_t> s_next_font_id{1};

FontFace::FontFace(FT_Face face, std::vector<uint8_t> memory_buffer)
    : font_id_(s_next_font_id.fetch_add(1, std::memory_order_relaxed)),
      generation_(1),
      ft_face_(face),
      memory_buffer_(std::move(memory_buffer)) {
    if (ft_face_) {
        hb_font_ = hb_ft_font_create(ft_face_, nullptr);
        if (ft_face_->family_name) {
            family_name_ = ft_face_->family_name;
        }
        if (ft_face_->style_name) {
            style_name_ = ft_face_->style_name;
        }
        set_pixel_size(16.0f, 1.0f);
    }
}

FontFace::~FontFace() {
    if (hb_font_) {
        hb_font_destroy(hb_font_);
        hb_font_ = nullptr;
    }
    if (ft_face_) {
        FT_Done_Face(ft_face_);
        ft_face_ = nullptr;
    }
}

void FontFace::set_pixel_size(float size, float dpi_scale) {
    current_pixel_size_ = size;
    current_dpi_scale_ = dpi_scale;
    float effective = std::max(1.0f, size * dpi_scale);

    if (ft_face_) {
        FT_Set_Pixel_Sizes(ft_face_, 0, static_cast<FT_UInt>(std::round(effective)));
        if (hb_font_) {
            hb_ft_font_changed(hb_font_);
        }
        update_metrics();
    }
}

void FontFace::update_metrics() {
    if (!ft_face_) return;

    metrics_.units_per_em = ft_face_->units_per_EM;
    if (ft_face_->size && ft_face_->units_per_EM > 0) {
        float scale = static_cast<float>(ft_face_->size->metrics.y_ppem) / static_cast<float>(ft_face_->units_per_EM);
        metrics_.ascender = static_cast<float>(ft_face_->ascender) * scale;
        metrics_.descender = static_cast<float>(ft_face_->descender) * scale;
        metrics_.line_height = static_cast<float>(ft_face_->height) * scale;
    } else {
        metrics_.ascender = current_pixel_size_ * 0.8f;
        metrics_.descender = -current_pixel_size_ * 0.2f;
        metrics_.line_height = current_pixel_size_ * 1.2f;
    }
}

uint32_t FontFace::glyph_index(uint32_t codepoint) const {
    if (!ft_face_) return 0;
    return FT_Get_Char_Index(ft_face_, codepoint);
}

GlyphBitmap FontFace::rasterize_glyph(uint32_t glyph_id, bool subpixel) {
    GlyphBitmap result;
    result.glyph_id = glyph_id;
    if (!ft_face_) return result;

    FT_Int32 flags = FT_LOAD_DEFAULT;
    if (FT_Load_Glyph(ft_face_, glyph_id, flags)) {
        return result;
    }

    FT_Render_Mode mode = subpixel ? FT_RENDER_MODE_LCD : FT_RENDER_MODE_NORMAL;
    if (FT_Render_Glyph(ft_face_->glyph, mode)) {
        return result;
    }

    FT_GlyphSlot slot = ft_face_->glyph;
    result.metrics.bearing_x = slot->bitmap_left;
    result.metrics.bearing_y = slot->bitmap_top;
    result.metrics.width = slot->bitmap.width;
    result.metrics.height = slot->bitmap.rows;
    result.metrics.advance_x = static_cast<float>(slot->advance.x) / 64.0f;
    result.metrics.advance_y = static_cast<float>(slot->advance.y) / 64.0f;
    result.format = PixelFormat::Alpha8;

    size_t total_pixels = static_cast<size_t>(slot->bitmap.width) * static_cast<size_t>(slot->bitmap.rows);
    result.pixels.resize(total_pixels);

    if (slot->bitmap.pitch > 0) {
        for (uint32_t y = 0; y < slot->bitmap.rows; ++y) {
            const uint8_t* src_row = slot->bitmap.buffer + y * static_cast<size_t>(slot->bitmap.pitch);
            uint8_t* dst_row = result.pixels.data() + y * slot->bitmap.width;
            std::copy(src_row, src_row + slot->bitmap.width, dst_row);
        }
    } else if (slot->bitmap.pitch < 0) {
        for (uint32_t y = 0; y < slot->bitmap.rows; ++y) {
            const uint8_t* src_row = slot->bitmap.buffer + (slot->bitmap.rows - 1 - y) * static_cast<size_t>(-slot->bitmap.pitch);
            uint8_t* dst_row = result.pixels.data() + y * slot->bitmap.width;
            std::copy(src_row, src_row + slot->bitmap.width, dst_row);
        }
    }

    return result;
}

} // namespace pingdart::text
