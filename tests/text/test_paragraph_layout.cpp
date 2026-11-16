#include <gtest/gtest.h>
#include <pingdart/text/paragraph.h>
#include <pingdart/rhi/display_list.h>
#include <pingdart/rhi/compositor.h>
#include <pingdart/rhi/i_render_device.h>

using namespace pingdart;
using namespace pingdart::text;
using namespace pingdart::rhi;

TEST(ParagraphLayoutTest, SingleLineLayoutAndMeasurement) {
    ParagraphStyle style{
        .default_style = TextStyle{.font_family = "Roboto", .font_size = 16.0f},
        .align = TextAlign::Left
    };

    ParagraphBuilder builder(style);
    builder.add_text("PingDart Native C++ Framework");
    auto paragraph = builder.build();

    paragraph->layout(500.0f);
    EXPECT_EQ(paragraph->line_count(), 1u);
    EXPECT_GT(paragraph->width(), 0.0f);
    EXPECT_LE(paragraph->width(), 500.0f);
    EXPECT_GT(paragraph->height(), 0.0f);
}

TEST(ParagraphLayoutTest, MultiLineWrappingRespectingWidthConstraints) {
    ParagraphStyle style{
        .default_style = TextStyle{.font_family = "Roboto", .font_size = 14.0f},
        .align = TextAlign::Left
    };

    ParagraphBuilder builder(style);
    builder.add_text("PingDart provides high-performance cross-platform native rendering without WebViews or Electron.");
    auto paragraph = builder.build();

    float constraint_width = 150.0f;
    paragraph->layout(constraint_width);

    EXPECT_GT(paragraph->line_count(), 2u);
    for (const auto& line : paragraph->lines()) {
        EXPECT_LE(line.width, constraint_width + 10.0f); // within one character tolerance
        EXPECT_GT(line.height, 0.0f);
    }
}

TEST(ParagraphLayoutTest, CaretPositioningAndHitTesting) {
    ParagraphStyle style{
        .default_style = TextStyle{.font_family = "Roboto", .font_size = 16.0f},
        .align = TextAlign::Left
    };

    ParagraphBuilder builder(style);
    builder.add_text("Login Username");
    auto paragraph = builder.build();
    paragraph->layout(400.0f);

    // Caret at offset 0 (start of string)
    auto pos0 = paragraph->get_position_for_offset(0);
    EXPECT_EQ(pos0.offset, 0u);
    EXPECT_FLOAT_EQ(pos0.caret_point.x, 0.0f);
    EXPECT_FLOAT_EQ(pos0.caret_point.y, 0.0f);

    // Caret at offset 5 ("Login")
    auto pos5 = paragraph->get_position_for_offset(5);
    EXPECT_EQ(pos5.offset, 5u);
    EXPECT_GT(pos5.caret_point.x, pos0.caret_point.x);

    // Hit-testing click at point near offset 5
    size_t click_offset = paragraph->get_offset_for_position(Point{pos5.caret_point.x + 2.0f, 5.0f});
    EXPECT_EQ(click_offset, 5u);
}

TEST(ParagraphLayoutTest, AlignmentOffsets) {
    ParagraphStyle style_left{
        .default_style = TextStyle{.font_family = "Roboto", .font_size = 16.0f},
        .align = TextAlign::Left
    };
    ParagraphBuilder builder_left(style_left);
    builder_left.add_text("Centered Text");
    auto p_left = builder_left.build();
    p_left->layout(400.0f);
    EXPECT_FLOAT_EQ(p_left->lines()[0].origin.x, 0.0f);

    ParagraphStyle style_center{
        .default_style = TextStyle{.font_family = "Roboto", .font_size = 16.0f},
        .align = TextAlign::Center
    };
    ParagraphBuilder builder_center(style_center);
    builder_center.add_text("Centered Text");
    auto p_center = builder_center.build();
    p_center->layout(400.0f);
    EXPECT_GT(p_center->lines()[0].origin.x, 0.0f);

    ParagraphStyle style_right{
        .default_style = TextStyle{.font_family = "Roboto", .font_size = 16.0f},
        .align = TextAlign::Right
    };
    ParagraphBuilder builder_right(style_right);
    builder_right.add_text("Centered Text");
    auto p_right = builder_right.build();
    p_right->layout(400.0f);
    EXPECT_GT(p_right->lines()[0].origin.x, p_center->lines()[0].origin.x);
}

TEST(ParagraphLayoutTest, DisplayListPaintIntegration) {
    ParagraphStyle style{
        .default_style = TextStyle{
            .font_family = "Roboto",
            .font_size = 18.0f,
            .color = Color::from_argb(255, 20, 20, 20)
        },
        .align = TextAlign::Left
    };

    ParagraphBuilder builder(style);
    builder.add_text("Native Login Button");
    auto paragraph = builder.build();
    paragraph->layout(300.0f);

    DisplayListBuilder dl_builder;
    paragraph->paint(dl_builder, Point{50.0f, 100.0f});
    auto dl = dl_builder.finish_recording();

    ASSERT_TRUE(dl);
    EXPECT_EQ(dl->op_count(), 1u);
    EXPECT_FALSE(dl->bounds().is_empty());

    // Verify GPU submission via Compositor
    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Vulkan};
    auto device = IRenderDevice::create(desc).value();

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Win32,
        .token = pal::Win32SurfaceDescriptor{
            .hwnd = reinterpret_cast<void*>(0x881122),
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

    Compositor compositor(device.get());
    auto stats = compositor.render_frame(dl.get(), backbuffer, ColorClearValue{1.0f, 1.0f, 1.0f, 1.0f});
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats.value().ops_drawn, 1u);

    EXPECT_TRUE(swapchain->present().ok());
}
