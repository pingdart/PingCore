// src/text/font_face_stub.cpp
// Stub implementation of FontFace when FreeType/HarfBuzz dev libraries are not present on build host
#include <pingdart/text/font_face.h>

namespace pingdart::text {

FontFace::FontFace(FT_Face, std::vector<uint8_t>) {}
FontFace::~FontFace() = default;

core::RefPtr<FontFace> FontFace::load_from_file(const std::string&, uint32_t) {
    return nullptr;
}

core::RefPtr<FontFace> FontFace::load_from_memory(const uint8_t*, size_t, uint32_t) {
    return nullptr;
}

void FontFace::set_pixel_size(float, float) {}

uint32_t FontFace::glyph_index(uint32_t) const {
    return 0;
}

GlyphBitmap FontFace::rasterize_glyph(uint32_t, bool) {
    return GlyphBitmap{};
}

} // namespace pingdart::text
