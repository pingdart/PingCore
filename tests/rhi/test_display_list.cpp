#include <gtest/gtest.h>
#include <pingdart/rhi/geometry.h>
#include <pingdart/rhi/paint.h>
#include <pingdart/rhi/display_list.h>
#include <pingdart/rhi/display_list_mailbox.h>
#include <pingdart/rhi/compositor.h>
#include <pingdart/rhi/i_render_device.h>
#include "../../src/rhi/vulkan/vulkan_device.h"

using namespace pingdart;
using namespace pingdart::rhi;

class TestDisplayListReceiver final : public IDisplayListReceiver {
public:
    void on_save() override { save_count++; }
    void on_restore() override { restore_count++; }
    void on_push_clip_rect(const Rect&, ClipOp) override { clip_rect_count++; }
    void on_push_clip_rrect(const RRect&, ClipOp) override { clip_rrect_count++; }
    void on_pop_clip() override { pop_clip_count++; }
    void on_push_transform(const Matrix2D&) override { transform_count++; }
    void on_pop_transform() override { pop_transform_count++; }
    void on_push_opacity(float) override { opacity_count++; }
    void on_pop_opacity() override { pop_opacity_count++; }

    void on_draw_rect(const Rect& r, const Paint&) override {
        rects.push_back(r);
    }
    void on_draw_rrect(const RRect& rr, const Paint&) override {
        rrects.push_back(rr);
    }
    void on_draw_circle(Point p, float, const Paint&) override {
        circles.push_back(p);
    }
    void on_draw_line(Point p0, Point p1, const Paint&) override {
        lines.push_back({p0, p1});
    }
    void on_draw_text_blob(const TextBlob& blob, Point, const Paint&) override {
        text_blobs.push_back(blob.text);
    }
    void on_draw_image(ITexture*, const Rect&, const Rect& dst, const Paint&) override {
        images.push_back(dst);
    }
    void on_draw_shadow(const RRect&, Color, float, float) override {
        shadow_count++;
    }

    uint32_t save_count{0};
    uint32_t restore_count{0};
    uint32_t clip_rect_count{0};
    uint32_t clip_rrect_count{0};
    uint32_t pop_clip_count{0};
    uint32_t transform_count{0};
    uint32_t pop_transform_count{0};
    uint32_t opacity_count{0};
    uint32_t pop_opacity_count{0};
    uint32_t shadow_count{0};

    std::vector<Rect> rects;
    std::vector<RRect> rrects;
    std::vector<Point> circles;
    std::vector<std::pair<Point, Point>> lines;
    std::vector<std::string> text_blobs;
    std::vector<Rect> images;
};

TEST(DisplayListTest, GeometryAndAffineTransforms) {
    Rect r1 = Rect::from_xywh(10.0f, 20.0f, 100.0f, 50.0f);
    EXPECT_EQ(r1.left, 10.0f);
    EXPECT_EQ(r1.top, 20.0f);
    EXPECT_EQ(r1.right, 110.0f);
    EXPECT_EQ(r1.bottom, 70.0f);
    EXPECT_EQ(r1.width(), 100.0f);
    EXPECT_EQ(r1.height(), 50.0f);
    EXPECT_TRUE(r1.contains(Point{50.0f, 40.0f}));
    EXPECT_FALSE(r1.contains(Point{0.0f, 0.0f}));

    Rect r2 = Rect::from_xywh(80.0f, 40.0f, 60.0f, 60.0f);
    EXPECT_TRUE(r1.intersects(r2));

    Rect u = r1.union_with(r2);
    EXPECT_EQ(u.left, 10.0f);
    EXPECT_EQ(u.top, 20.0f);
    EXPECT_EQ(u.right, 140.0f);
    EXPECT_EQ(u.bottom, 100.0f);

    Rect isect = r1.intersect_with(r2);
    EXPECT_EQ(isect.left, 80.0f);
    EXPECT_EQ(isect.top, 40.0f);
    EXPECT_EQ(isect.right, 110.0f);
    EXPECT_EQ(isect.bottom, 70.0f);

    // Matrix translation and scaling
    Matrix2D m_trans = Matrix2D::translation(100.0f, 50.0f);
    Point p_mapped = m_trans.map_point(Point{10.0f, 20.0f});
    EXPECT_FLOAT_EQ(p_mapped.x, 110.0f);
    EXPECT_FLOAT_EQ(p_mapped.y, 70.0f);

    Matrix2D m_scale = Matrix2D::scaling(2.0f, 3.0f);
    Matrix2D m_combined = m_trans * m_scale;
    Point p_scaled_trans = m_combined.map_point(Point{5.0f, 5.0f});
    EXPECT_FLOAT_EQ(p_scaled_trans.x, 110.0f);
    EXPECT_FLOAT_EQ(p_scaled_trans.y, 65.0f);

    Rect r_mapped = m_combined.map_rect(Rect::from_xywh(0.0f, 0.0f, 10.0f, 10.0f));
    EXPECT_FLOAT_EQ(r_mapped.left, 100.0f);
    EXPECT_FLOAT_EQ(r_mapped.top, 50.0f);
    EXPECT_FLOAT_EQ(r_mapped.right, 120.0f);
    EXPECT_FLOAT_EQ(r_mapped.bottom, 80.0f);
}

TEST(DisplayListTest, DisplayListRecordingAndReplay) {
    DisplayListBuilder builder;
    Paint paint;
    paint.color = Color::from_argb(255, 255, 0, 0);

    builder.draw_rect(Rect::from_xywh(10, 10, 100, 100), paint);

    RRect rrect = RRect::from_rect_and_corners(Rect::from_xywh(200, 50, 120, 60), 8.0f, 8.0f, 8.0f, 8.0f);
    builder.draw_rounded_rect(rrect, paint);

    builder.draw_circle(Point{400, 100}, 25.0f, paint);
    builder.draw_line(Point{0, 0}, Point{100, 100}, paint);

    TextBlob blob;
    blob.text = "PingDart Native UI";
    blob.font_size = 18.0f;
    builder.draw_text_blob(blob, Point{50, 250}, paint);

    builder.draw_shadow(rrect, Color::from_argb(128, 0, 0, 0), 4.0f, 2.0f);

    core::RefPtr<DisplayList> dl = builder.finish_recording();
    ASSERT_TRUE(dl);
    EXPECT_EQ(dl->op_count(), 6u);
    EXPECT_FALSE(dl->bounds().is_empty());

    TestDisplayListReceiver receiver;
    dl->replay(receiver);

    EXPECT_EQ(receiver.rects.size(), 1u);
    EXPECT_EQ(receiver.rrects.size(), 1u);
    EXPECT_EQ(receiver.circles.size(), 1u);
    EXPECT_EQ(receiver.lines.size(), 1u);
    EXPECT_EQ(receiver.text_blobs.size(), 1u);
    EXPECT_EQ(receiver.shadow_count, 1u);
    EXPECT_EQ(receiver.text_blobs[0], "PingDart Native UI");
}

TEST(DisplayListTest, CanvasStateStackNesting) {
    DisplayListBuilder builder;
    Paint paint;

    builder.save();
    builder.push_transform(Matrix2D::translation(50, 50));
    builder.push_clip_rect(Rect::from_xywh(0, 0, 200, 200));
    builder.push_opacity(0.5f);

    builder.draw_rect(Rect::from_xywh(10, 10, 50, 50), paint);

    builder.pop_opacity();
    builder.pop_clip();
    builder.pop_transform();
    builder.restore();

    core::RefPtr<DisplayList> dl = builder.finish_recording();
    ASSERT_TRUE(dl);

    TestDisplayListReceiver receiver;
    dl->replay(receiver);

    EXPECT_EQ(receiver.save_count, 1u);
    EXPECT_EQ(receiver.transform_count, 1u);
    EXPECT_EQ(receiver.clip_rect_count, 1u);
    EXPECT_EQ(receiver.opacity_count, 1u);
    EXPECT_EQ(receiver.pop_opacity_count, 1u);
    EXPECT_EQ(receiver.pop_clip_count, 1u);
    EXPECT_EQ(receiver.pop_transform_count, 1u);
    EXPECT_EQ(receiver.restore_count, 1u);
    EXPECT_EQ(receiver.rects.size(), 1u);
}

TEST(DisplayListTest, DamageRegionCalculationAndPartialRedrawCulling) {
    DisplayListBuilder builder;
    Paint paint;

    // Item 1: Top-left area [10, 10, 60, 60]
    builder.draw_rect(Rect::from_xywh(10, 10, 50, 50), paint);

    // Item 2: Bottom-right area [500, 500, 550, 550]
    builder.draw_rect(Rect::from_xywh(500, 500, 50, 50), paint);

    // Item 3: Center area [250, 250, 300, 300]
    builder.draw_rect(Rect::from_xywh(250, 250, 50, 50), paint);

    core::RefPtr<DisplayList> dl = builder.finish_recording();
    ASSERT_TRUE(dl);
    EXPECT_EQ(dl->op_count(), 3u);

    // Full replay (no damage culling)
    TestDisplayListReceiver full_receiver;
    dl->replay(full_receiver);
    EXPECT_EQ(full_receiver.rects.size(), 3u);

    // Partial replay with damage localized strictly to top-left [0, 0, 100, 100]
    DamageRegion damage_top_left(Rect::from_xywh(0, 0, 100, 100));
    TestDisplayListReceiver culled_receiver;
    dl->replay(culled_receiver, &damage_top_left);

    // Only Item 1 should have been drawn; Items 2 & 3 must be culled!
    ASSERT_EQ(culled_receiver.rects.size(), 1u);
    EXPECT_FLOAT_EQ(culled_receiver.rects[0].left, 10.0f);
    EXPECT_FLOAT_EQ(culled_receiver.rects[0].top, 10.0f);
}

TEST(DisplayListTest, TripleBufferedMailboxThreadSafety) {
    DisplayListMailbox mailbox;

    DisplayListBuilder builder1;
    builder1.draw_rect(Rect::from_xywh(0, 0, 10, 10), Paint{});
    auto dl1 = builder1.finish_recording();

    DisplayListBuilder builder2;
    builder2.draw_rect(Rect::from_xywh(0, 0, 20, 20), Paint{});
    auto dl2 = builder2.finish_recording();

    DisplayListBuilder builder3;
    builder3.draw_rect(Rect::from_xywh(0, 0, 30, 30), Paint{});
    auto dl3 = builder3.finish_recording();

    // UI Thread publishes 3 frames
    uint64_t seq1 = mailbox.publish(dl1);
    uint64_t seq2 = mailbox.publish(dl2);
    uint64_t seq3 = mailbox.publish(dl3);

    EXPECT_LT(seq1, seq2);
    EXPECT_LT(seq2, seq3);

    // Render Thread acquires: should retrieve latest frame (dl3) directly, dropping dl1 and dl2
    uint64_t acquired_seq = 0;
    auto acquired_dl = mailbox.acquire_latest(&acquired_seq);
    ASSERT_TRUE(acquired_dl);
    EXPECT_EQ(acquired_seq, seq3);

    // Release rendering slot
    mailbox.release_rendering();

    // Acquiring again when no new publish returns nullptr
    auto empty_dl = mailbox.acquire_latest();
    EXPECT_FALSE(empty_dl);
}

TEST(DisplayListTest, CompositorGPUFrameRendering) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true
    };
    auto device = IRenderDevice::create(desc).value();

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Win32,
        .token = pal::Win32SurfaceDescriptor{
            .hwnd = reinterpret_cast<void*>(0x770011),
            .hinstance = reinterpret_cast<void*>(0x400000)
        },
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto swapchain = device->create_swapchain(surface, 800, 600).value();
    auto* backbuffer = swapchain->acquire_next_backbuffer().value();

    DisplayListBuilder builder;
    Paint paint;
    paint.color = Color::from_argb(255, 30, 144, 255); // Dodger Blue

    // Draw background and card
    builder.draw_rect(Rect::from_xywh(0, 0, 800, 600), paint);

    RRect card = RRect::from_rect_and_corners(Rect::from_xywh(200, 150, 400, 300), 12.0f, 12.0f, 12.0f, 12.0f);
    Paint card_paint;
    card_paint.color = Color::from_argb(255, 255, 255, 255);
    builder.draw_rounded_rect(card, card_paint);

    TextBlob title;
    title.text = "PingDart Login Screen";
    title.font_size = 20.0f;
    builder.draw_text_blob(title, Point{250, 200}, Paint{});

    auto dl = builder.finish_recording();

    Compositor compositor(device.get());
    auto stats_res = compositor.render_frame(dl.get(), backbuffer, ColorClearValue{0.05f, 0.05f, 0.05f, 1.0f});
    ASSERT_TRUE(stats_res.has_value());

    const auto& stats = stats_res.value();
    EXPECT_EQ(stats.total_ops_evaluated, 3u);
    EXPECT_EQ(stats.ops_drawn, 3u);
    EXPECT_EQ(stats.ops_culled_by_damage, 0u);

    // Present the composited backbuffer to Vulkan swapchain
    EXPECT_TRUE(swapchain->present().ok());
}
