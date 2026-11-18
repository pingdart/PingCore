#include <gtest/gtest.h>

#if defined(PINGDART_HAS_FREETYPE) && PINGDART_HAS_FREETYPE
#include <pingdart/text/font_face.h>
#include <filesystem>

using namespace pingdart::text;

namespace {

std::string get_latin_font_path() {
    std::vector<std::string> candidates = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) return path;
    }
    return "";
}

} // namespace

TEST(FreeTypeTest, LoadsFontFaceSuccessfully) {
    std::string path = get_latin_font_path();
    if (path.empty()) {
        GTEST_SKIP() << "No standard Latin font found on system";
    }

    auto face = FontFace::load_from_file(path);
    ASSERT_NE(face, nullptr);

    EXPECT_FALSE(face->family_name().empty());
    EXPECT_GT(face->metrics().units_per_em, 0u);
    EXPECT_GT(face->metrics().line_height, 0.0f);
}

TEST(FreeTypeTest, MapsCodepointsToGlyphIDs) {
    std::string path = get_latin_font_path();
    if (path.empty()) {
        GTEST_SKIP() << "No standard Latin font found on system";
    }

    auto face = FontFace::load_from_file(path);
    ASSERT_NE(face, nullptr);

    uint32_t gid_a = face->glyph_index('A');
    uint32_t gid_b = face->glyph_index('B');
    EXPECT_GT(gid_a, 0u);
    EXPECT_GT(gid_b, 0u);
    EXPECT_NE(gid_a, gid_b);
}

TEST(FreeTypeTest, RasterizesGlyphToBitmap) {
    std::string path = get_latin_font_path();
    if (path.empty()) {
        GTEST_SKIP() << "No standard Latin font found on system";
    }

    auto face = FontFace::load_from_file(path);
    ASSERT_NE(face, nullptr);

    face->set_pixel_size(16.0f, 1.0f);
    uint32_t gid = face->glyph_index('g');
    ASSERT_GT(gid, 0u);

    GlyphBitmap bmp = face->rasterize_glyph(gid);
    EXPECT_GT(bmp.metrics.width, 0u);
    EXPECT_GT(bmp.metrics.height, 0u);
    EXPECT_FALSE(bmp.buffer.empty());
    EXPECT_EQ(bmp.buffer.size(), bmp.metrics.width * bmp.metrics.height);
}

TEST(FreeTypeTest, DpiScalingAdjustsPixelDimensions) {
    std::string path = get_latin_font_path();
    if (path.empty()) {
        GTEST_SKIP() << "No standard Latin font found on system";
    }

    auto face = FontFace::load_from_file(path);
    ASSERT_NE(face, nullptr);

    uint32_t gid = face->glyph_index('M');
    ASSERT_GT(gid, 0u);

    face->set_pixel_size(20.0f, 1.0f);
    GlyphBitmap bmp_1x = face->rasterize_glyph(gid);

    face->set_pixel_size(20.0f, 2.0f); // 2x Retina
    GlyphBitmap bmp_2x = face->rasterize_glyph(gid);

    EXPECT_GT(bmp_2x.metrics.width, bmp_1x.metrics.width);
    EXPECT_GT(bmp_2x.metrics.height, bmp_1x.metrics.height);
}

#else

TEST(FreeTypeTest, FreeTypeOptionalOnHost) {
    GTEST_SKIP() << "FreeType development package not installed on host; skipping native rasterization suite.";
}

#endif
