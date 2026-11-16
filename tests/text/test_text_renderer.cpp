#include <gtest/gtest.h>
#include <pingdart/text/text_renderer.h>
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

TEST(TextRendererTest, RendersLatinTextQuads) {
    std::string font_path = find_font({
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc"
    });
    ASSERT_FALSE(font_path.empty());

    auto face = FontFace::load_from_file(font_path);
    ASSERT_NE(face, nullptr);

    TextRenderer renderer(1024);
    Color white = Color::from_argb(255, 255, 255, 255);
    Point origin{10.0f, 50.0f};

    ShapedRun run = renderer.shape_and_render(*face, "PingDart Native", 16.0f, 1.0f, origin, white);

    EXPECT_GT(run.quads.size(), 5u);
    EXPECT_GT(run.total_advance_x, 50.0f);

    for (const auto& quad : run.quads) {
        EXPECT_GT(quad.dst_rect.width(), 0.0f);
        EXPECT_GT(quad.dst_rect.height(), 0.0f);
        EXPECT_GE(quad.uv_rect.left, 0.0f);
        EXPECT_LE(quad.uv_rect.right, 1.0f);
        EXPECT_EQ(quad.color, white);
    }
}

TEST(TextRendererTest, RendersTeluguComplexScriptQuads) {
    std::string font_path = find_font({
        "/System/Library/Fonts/KohinoorTelugu.ttc",
        "/System/Library/Fonts/Supplemental/Telugu MN.ttc"
    });
    if (font_path.empty()) {
        GTEST_SKIP() << "Telugu font not available on this host";
    }

    auto face = FontFace::load_from_file(font_path);
    ASSERT_NE(face, nullptr);

    TextRenderer renderer(1024);
    Color gold = Color::from_argb(255, 250, 204, 21);
    Point origin{20.0f, 60.0f};

    ShapedRun run = renderer.shape_and_render(*face, "నమస్కారం", 22.0f, 1.0f, origin, gold, HB_SCRIPT_TELUGU);

    EXPECT_GT(run.quads.size(), 0u);
    EXPECT_GT(run.total_advance_x, 30.0f);

    for (const auto& quad : run.quads) {
        EXPECT_GT(quad.dst_rect.width(), 0.0f);
        EXPECT_GT(quad.dst_rect.height(), 0.0f);
        EXPECT_EQ(quad.color, gold);
    }
}
