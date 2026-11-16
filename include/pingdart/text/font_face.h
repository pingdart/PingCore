#pragma once

#include "glyph_bitmap.h"
#include <pingdart/core/ref_ptr.h>
#include <string>
#include <memory>
#include <cstdint>

typedef struct FT_FaceRec_* FT_Face;
typedef struct hb_font_t hb_font_t;

namespace pingdart::text {

struct FontFaceMetrics {
    float ascender{0.0f};
    float descender{0.0f};
    float line_height{0.0f};
    uint32_t units_per_em{0};

    constexpr bool operator==(const FontFaceMetrics&) const noexcept = default;
};

class FontFace : public core::RefCounted<FontFace> {
public:
    static core::RefPtr<FontFace> load_from_file(const std::string& path, uint32_t face_index = 0);
    static core::RefPtr<FontFace> load_from_memory(const uint8_t* data, size_t size, uint32_t face_index = 0);

    ~FontFace() override;

    void set_pixel_size(float size, float dpi_scale = 1.0f);

    [[nodiscard]] const std::string& family_name() const noexcept { return family_name_; }
    [[nodiscard]] const std::string& style_name() const noexcept { return style_name_; }
    [[nodiscard]] const FontFaceMetrics& metrics() const noexcept { return metrics_; }
    [[nodiscard]] float current_pixel_size() const noexcept { return current_pixel_size_; }
    [[nodiscard]] float current_dpi_scale() const noexcept { return current_dpi_scale_; }

    [[nodiscard]] uint32_t glyph_index(uint32_t codepoint) const;
    [[nodiscard]] GlyphBitmap rasterize_glyph(uint32_t glyph_id, bool subpixel = false);

    [[nodiscard]] FT_Face ft_face() const noexcept { return ft_face_; }
    [[nodiscard]] hb_font_t* hb_font() const noexcept { return hb_font_; }
    [[nodiscard]] uint64_t font_id() const noexcept { return font_id_; }
    [[nodiscard]] uint32_t generation() const noexcept { return generation_; }

private:
    explicit FontFace(FT_Face face, std::vector<uint8_t> memory_buffer = {});

    uint64_t font_id_{0};
    uint32_t generation_{1};
    FT_Face ft_face_{nullptr};
    hb_font_t* hb_font_{nullptr};
    std::vector<uint8_t> memory_buffer_;

    std::string family_name_;
    std::string style_name_;
    FontFaceMetrics metrics_{};
    float current_pixel_size_{16.0f};
    float current_dpi_scale_{1.0f};

    void update_metrics();
};

} // namespace pingdart::text
