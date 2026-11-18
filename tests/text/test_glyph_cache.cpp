#include <gtest/gtest.h>
#include <pingdart/text/glyph_cache.h>
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

TEST(GlyphCacheTest, CachesRasterizedGlyphsWithoutDuplicateWork) {
    std::string font_path = get_latin_font_path();
    if (font_path.empty()) {
        GTEST_SKIP() << "Standard font not found on host";
    }

    auto face = FontFace::load_from_file(font_path);
    if (!face) {
        GTEST_SKIP() << "FontFace loading requires FreeType on this host";
    }

    GlyphCache cache(512);
    uint32_t gid = face->glyph_index('A');

    // First lookup: Cache miss, rasterizes and inserts
    const AtlasGlyph* ag1 = cache.get_or_insert(*face, gid, 18.0f, 1.0f, 1, 0);
    ASSERT_NE(ag1, nullptr);
    EXPECT_EQ(cache.cached_glyph_count(), 1u);

    // Second lookup: Cache hit
    const AtlasGlyph* ag2 = cache.get_or_insert(*face, gid, 18.0f, 1.0f, 2, 1);
    ASSERT_NE(ag2, nullptr);
    EXPECT_EQ(ag1, ag2);
    EXPECT_EQ(cache.cached_glyph_count(), 1u);
    EXPECT_EQ(ag2->last_used_generation, 2u);
}

TEST(GlyphCacheTest, InvalidatesCacheOnDpiScaleChange) {
    std::string font_path = get_latin_font_path();
    if (font_path.empty()) {
        GTEST_SKIP() << "Standard font not found on host";
    }

    auto face = FontFace::load_from_file(font_path);
    if (!face) {
        GTEST_SKIP() << "FontFace loading requires FreeType on this host";
    }

    GlyphCache cache(512);
    uint32_t gid = face->glyph_index('B');

    cache.get_or_insert(*face, gid, 16.0f, 1.0f, 1, 0);
    cache.get_or_insert(*face, gid, 16.0f, 2.0f, 1, 0);
    EXPECT_EQ(cache.cached_glyph_count(), 2u);

    // Invalidate 1x scale entries
    cache.invalidate_dpi(1.0f);
    EXPECT_EQ(cache.cached_glyph_count(), 1u);

    GlyphKey key_2x{face->font_id(), face->generation(), gid, 16.0f, 2.0f, false, false};
    EXPECT_NE(cache.find(key_2x), nullptr);
}

TEST(GlyphCacheTest, EvictionProtectsInFlightGpuFrames) {
    std::string font_path = get_latin_font_path();
    if (font_path.empty()) {
        GTEST_SKIP() << "Standard font not found on host";
    }

    auto face = FontFace::load_from_file(font_path);
    if (!face) {
        GTEST_SKIP() << "FontFace loading requires FreeType on this host";
    }

    // Create a very small atlas (64x64) to quickly trigger eviction
    GlyphCache cache(64);

    // Fill atlas with completed glyphs at gen 1 (completed gen = 5)
    uint32_t gid_x = face->glyph_index('X');
    uint32_t gid_y = face->glyph_index('Y');

    cache.get_or_insert(*face, gid_x, 24.0f, 1.0f, 1, 0);
    cache.get_or_insert(*face, gid_y, 24.0f, 1.0f, 1, 0);

    // Add an in-flight glyph at gen 10 (completed gen = 5)
    uint32_t gid_z = face->glyph_index('Z');
    cache.get_or_insert(*face, gid_z, 24.0f, 1.0f, 10, 5);

    // Now insert a new glyph causing eviction, passing completed_gpu_gen = 5
    uint32_t gid_w = face->glyph_index('W');
    const AtlasGlyph* ag_w = cache.get_or_insert(*face, gid_w, 24.0f, 1.0f, 11, 5);
    ASSERT_NE(ag_w, nullptr);

    // In-flight glyph Z (gen 10 > completed 5) must be preserved!
    GlyphKey key_z{face->font_id(), face->generation(), gid_z, 24.0f, 1.0f, false, false};
    EXPECT_NE(cache.find(key_z), nullptr);
}
