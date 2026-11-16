#include <gtest/gtest.h>
#include <pingdart/text/glyph_atlas.h>

using namespace pingdart::text;

TEST(GlyphAtlasTest, SkylinePackerAllocatesDisjointRects) {
    SkylinePacker packer(256, 256);
    std::vector<AtlasRect> allocations;

    for (int i = 0; i < 20; ++i) {
        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t w = 24;
        uint32_t h = 24;
        ASSERT_TRUE(packer.allocate(w, h, x, y));
        allocations.push_back(AtlasRect{x, y, w, h});
    }

    // Check that no two allocations overlap
    for (size_t i = 0; i < allocations.size(); ++i) {
        for (size_t j = i + 1; j < allocations.size(); ++j) {
            const auto& a = allocations[i];
            const auto& b = allocations[j];

            bool separate = (a.x + a.width <= b.x) ||
                            (b.x + b.width <= a.x) ||
                            (a.y + a.height <= b.y) ||
                            (b.y + b.height <= a.y);
            EXPECT_TRUE(separate) << "Overlap between rect " << i << " and rect " << j;
        }
    }
}

TEST(GlyphAtlasTest, PacksGlyphBitmapsAndCalculatesUVs) {
    GlyphAtlas atlas(512, 512);

    GlyphBitmap bmp;
    bmp.glyph_id = 42;
    bmp.metrics.width = 32;
    bmp.metrics.height = 32;
    bmp.metrics.bearing_x = 2;
    bmp.metrics.bearing_y = 28;
    bmp.metrics.advance_x = 34.0f;
    bmp.pixels.assign(32 * 32, 255); // Solid white glyph

    AtlasGlyph ag;
    ASSERT_TRUE(atlas.add_glyph(bmp, ag, 1));

    EXPECT_EQ(ag.glyph_id, 42u);
    EXPECT_EQ(ag.rect.width, 32u);
    EXPECT_EQ(ag.rect.height, 32u);

    // UVs must be normalized in [0, 1]
    EXPECT_GE(ag.u0, 0.0f);
    EXPECT_LE(ag.u1, 1.0f);
    EXPECT_GE(ag.v0, 0.0f);
    EXPECT_LE(ag.v1, 1.0f);
    EXPECT_LT(ag.u0, ag.u1);
    EXPECT_LT(ag.v0, ag.v1);

    // Verify dirty region was recorded
    EXPECT_TRUE(atlas.has_dirty_regions());
    auto dirty = atlas.consume_dirty_regions();
    EXPECT_EQ(dirty.size(), 1u);
    EXPECT_EQ(dirty[0], ag.rect);
    EXPECT_FALSE(atlas.has_dirty_regions());
}

TEST(GlyphAtlasTest, HandlesZeroDimensionGlyphsGracefully) {
    GlyphAtlas atlas(256, 256);

    GlyphBitmap space_bmp;
    space_bmp.glyph_id = 3;
    space_bmp.metrics.width = 0;
    space_bmp.metrics.height = 0;
    space_bmp.metrics.advance_x = 10.0f;

    AtlasGlyph ag;
    EXPECT_TRUE(atlas.add_glyph(space_bmp, ag, 1));
    EXPECT_EQ(ag.rect.width, 0u);
    EXPECT_EQ(ag.rect.height, 0u);
    EXPECT_EQ(ag.metrics.advance_x, 10.0f);
}
