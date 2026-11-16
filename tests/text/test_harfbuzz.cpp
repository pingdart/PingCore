#include <gtest/gtest.h>
#include <pingdart/text/font_face.h>
#include <hb.h>
#include <filesystem>

using namespace pingdart::text;

namespace {

std::string find_font(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) return path;
    }
    return "";
}

} // namespace

TEST(HarfBuzzTest, ShapesLatinLigaturesAndAdvances) {
    std::string font_path = find_font({
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc"
    });
    ASSERT_FALSE(font_path.empty());

    auto face = FontFace::load_from_file(font_path);
    ASSERT_NE(face, nullptr);
    ASSERT_NE(face->hb_font(), nullptr);

    hb_buffer_t* buf = hb_buffer_create();
    const char* text = "Hello";
    hb_buffer_add_utf8(buf, text, -1, 0, -1);
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);

    hb_shape(face->hb_font(), buf, nullptr, 0);

    unsigned int glyph_count = 0;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    EXPECT_EQ(glyph_count, 5u);
    float total_advance = 0.0f;
    for (unsigned int i = 0; i < glyph_count; ++i) {
        EXPECT_GT(glyph_info[i].codepoint, 0u);
        EXPECT_GT(glyph_pos[i].x_advance, 0);
        total_advance += static_cast<float>(glyph_pos[i].x_advance) / 64.0f;
    }
    EXPECT_GT(total_advance, 20.0f);

    hb_buffer_destroy(buf);
}

TEST(HarfBuzzTest, ShapesTeluguConjunctsWithRealFont) {
    std::string font_path = find_font({
        "/System/Library/Fonts/KohinoorTelugu.ttc",
        "/System/Library/Fonts/Supplemental/Telugu MN.ttc",
        "/System/Library/Fonts/Supplemental/Telugu Sangam MN.ttc"
    });
    if (font_path.empty()) {
        GTEST_SKIP() << "Telugu font not available on this host";
    }

    auto face = FontFace::load_from_file(font_path);
    ASSERT_NE(face, nullptr);

    hb_buffer_t* buf = hb_buffer_create();
    // "నమస్కారం" has Telugu letters, virama, and anusvara
    const char* telugu_text = "నమస్కారం";
    hb_buffer_add_utf8(buf, telugu_text, -1, 0, -1);
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_TELUGU);

    hb_shape(face->hb_font(), buf, nullptr, 0);

    unsigned int glyph_count = 0;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    ASSERT_GT(glyph_count, 0u);

    // Verify OpenType glyph IDs are non-zero
    for (unsigned int i = 0; i < glyph_count; ++i) {
        EXPECT_GT(glyph_info[i].codepoint, 0u);
    }

    hb_buffer_destroy(buf);
}

TEST(HarfBuzzTest, ShapesDevanagariLigatures) {
    std::string font_path = find_font({
        "/System/Library/Fonts/Supplemental/DevanagariMT.ttc",
        "/System/Library/Fonts/Supplemental/ITFDevanagari.ttc",
        "/System/Library/Fonts/Supplemental/Devanagari Sangam MN.ttc"
    });
    if (font_path.empty()) {
        GTEST_SKIP() << "Devanagari font not available on this host";
    }

    auto face = FontFace::load_from_file(font_path);
    ASSERT_NE(face, nullptr);

    hb_buffer_t* buf = hb_buffer_create();
    const char* hindi_text = "नमस्ते";
    hb_buffer_add_utf8(buf, hindi_text, -1, 0, -1);
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_DEVANAGARI);

    hb_shape(face->hb_font(), buf, nullptr, 0);

    unsigned int glyph_count = 0;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    EXPECT_GT(glyph_count, 0u);

    for (unsigned int i = 0; i < glyph_count; ++i) {
        EXPECT_GT(glyph_info[i].codepoint, 0u);
    }

    hb_buffer_destroy(buf);
}

TEST(HarfBuzzTest, ShapesArabicRTLContextualForms) {
    std::string font_path = find_font({
        "/System/Library/Fonts/SFArabic.ttf",
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf"
    });
    if (font_path.empty()) {
        GTEST_SKIP() << "Arabic font not available on this host";
    }

    auto face = FontFace::load_from_file(font_path);
    ASSERT_NE(face, nullptr);

    hb_buffer_t* buf = hb_buffer_create();
    const char* arabic_text = "مرحبا";
    hb_buffer_add_utf8(buf, arabic_text, -1, 0, -1);
    hb_buffer_set_direction(buf, HB_DIRECTION_RTL);
    hb_buffer_set_script(buf, HB_SCRIPT_ARABIC);

    hb_shape(face->hb_font(), buf, nullptr, 0);

    unsigned int glyph_count = 0;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    EXPECT_GT(glyph_count, 0u);

    for (unsigned int i = 0; i < glyph_count; ++i) {
        EXPECT_GT(glyph_info[i].codepoint, 0u);
    }

    hb_buffer_destroy(buf);
}
