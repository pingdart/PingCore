#include <gtest/gtest.h>
#include <pingdart/text/text_types.h>
#include <pingdart/text/font_manager.h>
#include <pingdart/text/unicode_analyzer.h>
#include <pingdart/text/text_shaper.h>

using namespace pingdart::text;

TEST(TextShapingTest, EnglishTextShaping) {
    TextShaper shaper;
    TextStyle style{
        .font_family = "Roboto",
        .font_size = 16.0f,
        .font_weight = 400
    };

    auto res = shaper.shape_text("Hello World", style);
    ASSERT_TRUE(res.has_value());
    const auto& runs = res.value();

    ASSERT_EQ(runs.size(), 1u);
    const auto& run = runs[0];

    EXPECT_EQ(run.font_family, "Roboto");
    EXPECT_EQ(run.direction, TextDirection::LTR);
    EXPECT_EQ(run.script, TextScript::Latin);
    EXPECT_EQ(run.glyphs.size(), 11u);
    EXPECT_GT(run.width, 0.0f);
    EXPECT_GT(run.ascent, 0.0f);
    EXPECT_GT(run.descent, 0.0f);

    // Verify monotonic cluster offsets for Latin
    for (size_t i = 0; i < run.glyphs.size(); ++i) {
        EXPECT_EQ(run.glyphs[i].cluster, i);
        EXPECT_GT(run.glyphs[i].x_advance, 0.0f);
    }
}

TEST(TextShapingTest, TeluguTextShapingAndComplexConjuncts) {
    TextShaper shaper;
    TextStyle style{
        .font_family = "Roboto", // Request Latin font; Telugu must trigger automatic fallback
        .font_size = 18.0f
    };

    // "నమస్కారం" ("Namaskaram" in Telugu)
    std::string_view telugu_text = "నమస్కారం";
    auto res = shaper.shape_text(telugu_text, style);
    ASSERT_TRUE(res.has_value());
    const auto& runs = res.value();

    ASSERT_EQ(runs.size(), 1u);
    const auto& run = runs[0];

    EXPECT_EQ(run.font_family, "Noto Sans Telugu"); // Automatic font fallback verified
    EXPECT_EQ(run.script, TextScript::Telugu);
    EXPECT_EQ(run.direction, TextDirection::LTR);
    EXPECT_GT(run.width, 0.0f);

    // Verify grapheme cluster integrity: combining marks are never separated into isolated glyphs
    auto clusters = UnicodeAnalyzer::segment_grapheme_clusters(telugu_text);
    // "న" (na), "మ" (ma), "స్కా" (ska), "రం" (ram) -> 4 distinct grapheme clusters
    EXPECT_EQ(clusters.size(), 4u);

    // First cluster "న" is at offset 0
    EXPECT_EQ(clusters[0].byte_start, 0u);
    // Cluster 2 "స్కా" contains virama and vowel sign and is flagged as a combining sequence
    EXPECT_TRUE(clusters[2].is_combining_sequence);
}

TEST(TextShapingTest, HindiDevanagariShaping) {
    TextShaper shaper;
    TextStyle style{
        .font_family = "Roboto",
        .font_size = 16.0f
    };

    // "नमस्ते" ("Namaste" in Hindi / Devanagari)
    std::string_view hindi_text = "नमस्ते";
    auto res = shaper.shape_text(hindi_text, style);
    ASSERT_TRUE(res.has_value());
    const auto& runs = res.value();

    ASSERT_EQ(runs.size(), 1u);
    const auto& run = runs[0];

    EXPECT_EQ(run.font_family, "Noto Sans Devanagari");
    EXPECT_EQ(run.script, TextScript::Devanagari);
    EXPECT_EQ(run.direction, TextDirection::LTR);

    auto clusters = UnicodeAnalyzer::segment_grapheme_clusters(hindi_text);
    // "न" (na), "म" (ma), "स्ते" (ste) -> 3 clusters
    EXPECT_EQ(clusters.size(), 3u);
    EXPECT_TRUE(clusters[2].is_combining_sequence);
}

TEST(TextShapingTest, ArabicRTLBiDiShaping) {
    TextShaper shaper;
    TextStyle style{
        .font_family = "Roboto",
        .font_size = 16.0f
    };

    // "مرحبا" ("Marhaban" in Arabic)
    std::string_view arabic_text = "مرحبا";
    auto res = shaper.shape_text(arabic_text, style);
    ASSERT_TRUE(res.has_value());
    const auto& runs = res.value();

    ASSERT_EQ(runs.size(), 1u);
    const auto& run = runs[0];

    EXPECT_EQ(run.font_family, "Noto Sans Arabic");
    EXPECT_EQ(run.script, TextScript::Arabic);
    EXPECT_EQ(run.direction, TextDirection::RTL);
    EXPECT_GT(run.width, 0.0f);

    // RTL visual positions: verify right-to-left layout order
    EXPECT_GE(run.glyphs[0].x_offset, 0.0f);
    EXPECT_LE(run.glyphs[0].x_offset, run.width);
}

TEST(TextShapingTest, MixedDirectionAndEmojiSequences) {
    TextShaper shaper;
    TextStyle style{
        .font_family = "Roboto",
        .font_size = 16.0f
    };

    // Mixed sentence: English + Telugu + Emoji
    std::string_view mixed = "Welcome నమస్కారం 🚀";
    auto res = shaper.shape_text(mixed, style);
    ASSERT_TRUE(res.has_value());
    const auto& runs = res.value();

    // Must split cleanly into 3 distinct font runs: Latin, Telugu, Emoji
    ASSERT_EQ(runs.size(), 3u);

    EXPECT_EQ(runs[0].script, TextScript::Latin);
    EXPECT_EQ(runs[0].font_family, "Roboto");

    EXPECT_EQ(runs[1].script, TextScript::Telugu);
    EXPECT_EQ(runs[1].font_family, "Noto Sans Telugu");

    EXPECT_EQ(runs[2].script, TextScript::Emoji);
    EXPECT_EQ(runs[2].font_family, "Noto Color Emoji");
}

TEST(TextShapingTest, FontManagerRegistrationAndMetrics) {
    FontManager fm;
    TextStyle style{
        .font_family = "Roboto",
        .font_size = 20.0f,
        .font_weight = 700
    };

    auto face = fm.resolve_font(style);
    ASSERT_NE(face, nullptr);
    EXPECT_EQ(face->family(), "Roboto");
    EXPECT_EQ(face->weight(), FontWeight::Bold);

    FontMetrics m = face->metrics_for_size(20.0f);
    EXPECT_FLOAT_EQ(m.ascent, 20.0f * 0.82f);
    EXPECT_FLOAT_EQ(m.descent, 20.0f * 0.22f);
    EXPECT_GT(m.line_height_total(), 0.0f);
}
