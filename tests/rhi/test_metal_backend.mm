// tests/rhi/test_metal_backend.mm
// PKG-114: PingCore Native Metal RHI Backend Tests
#include <gtest/gtest.h>
#include <pingdart/rhi/i_render_device.h>
#include <pingdart/rhi/i_swapchain.h>
#include <pingdart/rhi/i_texture.h>
#include <pingdart/rhi/i_command_buffer.h>
#include <pingdart/rhi/compositor.h>
#include <pingdart/rhi/display_list.h>
#include <pingdart/rhi/metal/metal_device.h>
#include <pingdart/rhi/metal/metal_swapchain.h>
#include <pingdart/rhi/metal/metal_texture.h>
#include <pingdart/pal/macos/macos_platform_host.h>
#include <pingdart/pal/macos/macos_window.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

using namespace pingdart::rhi;
using namespace pingdart::rhi::metal;
using namespace pingdart::pal;
using namespace pingdart::pal::macos;

// ============================================================================
// 1. Metal Device Discovery & Capabilities
// ============================================================================

TEST(MetalBackendGoogleTest, DeviceCreationAndHardwareCapabilities) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Metal,
        .enable_validation_layers = true,
        .prefer_discrete_gpu = true
    };

    auto dev_res = IRenderDevice::create(desc);
    if (!dev_res.has_value()) {
        GTEST_SKIP() << "IRenderDevice::create failed (likely sandboxed or headless): " << dev_res.status().message();
    }
    ASSERT_TRUE(dev_res.has_value());
    auto device = std::move(dev_res).value();

    EXPECT_EQ(device->api_type(), GraphicsApi::Metal);
    EXPECT_FALSE(device->device_name().empty());

    const auto& caps = device->capabilities();
    EXPECT_TRUE(caps.presentation_supported);
    EXPECT_TRUE(caps.timeline_semaphores);
    EXPECT_GE(caps.max_texture_dimension_2d, 8192u);

    auto p_info = device->physical_device_info();
    EXPECT_EQ(p_info.vendor_id, 0x106Bu); // Apple Inc.
    EXPECT_FALSE(device->is_device_lost());
}

// ============================================================================
// 2. Texture Allocation & Raw Pixel Uploads (Glyph Atlas Integration)
// ============================================================================

TEST(MetalBackendGoogleTest, TextureAllocationAndPixelUpload) {
    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Metal};
    auto dev_res = IRenderDevice::create(desc);
    if (!dev_res.has_value()) {
        GTEST_SKIP() << "IRenderDevice::create failed (likely sandboxed or headless): " << dev_res.status().message();
    }
    ASSERT_TRUE(dev_res.has_value());
    auto device = std::move(dev_res).value();

    uint32_t width = 128;
    uint32_t height = 128;
    auto tex_res = device->create_texture(
        width,
        height,
        PixelFormat::RGBA8_UNORM,
        TextureUsage::Sampled | TextureUsage::TransferDst
    );
    ASSERT_TRUE(tex_res.has_value());
    auto texture = std::move(tex_res).value();

    EXPECT_EQ(texture->width(), width);
    EXPECT_EQ(texture->height(), height);
    EXPECT_EQ(texture->format(), PixelFormat::RGBA8_UNORM);

    // Upload gradient / glyph bitmap pixels
    std::vector<uint32_t> test_pixels(width * height, 0xFF4488CC);
    auto* metal_tex = static_cast<MetalTexture*>(texture.get());
    metal_tex->upload_pixels(test_pixels.data(), width * 4, 0, 0, width, height);

    EXPECT_NE(metal_tex->native_texture_handle(), nullptr);
}

// ============================================================================
// 3. CAMetalLayer Surface Connection & Swapchain Lifecycle
// ============================================================================

TEST(MetalBackendGoogleTest, SurfaceAndSwapchainLifecycle) {
    MacosPlatformHost host;
    ASSERT_TRUE(host.initialize().ok());

    auto win_res = host.create_window("Metal Swapchain Test", WindowSize{800, 600});
    ASSERT_TRUE(win_res.has_value());
    auto window = std::move(win_res).value();

    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Metal};
    auto dev_res = IRenderDevice::create(desc);
    if (!dev_res.has_value()) {
        GTEST_SKIP() << "IRenderDevice::create failed (likely sandboxed or headless): " << dev_res.status().message();
    }
    ASSERT_TRUE(dev_res.has_value());
    auto device = std::move(dev_res).value();

    auto sc_res = device->create_swapchain(
        window->native_surface_handle(),
        800,
        600,
        PresentMode::Fifo
    );
    ASSERT_TRUE(sc_res.has_value());
    auto swapchain = std::move(sc_res).value();

    EXPECT_TRUE(swapchain->is_valid());
    EXPECT_EQ(swapchain->width(), 800u);
    EXPECT_EQ(swapchain->height(), 600u);
    EXPECT_EQ(swapchain->format(), PixelFormat::BGRA8_UNORM);
    EXPECT_EQ(swapchain->buffer_count(), 3u);

    // Resize handling: Minimized (0x0)
    EXPECT_TRUE(swapchain->resize(0, 0).ok());
    EXPECT_FALSE(swapchain->is_valid());

    // Backbuffer acquisition fails gracefully when minimized
    auto min_buf = swapchain->acquire_next_backbuffer();
    EXPECT_FALSE(min_buf.has_value());

    // Restore size (1024x768)
    EXPECT_TRUE(swapchain->resize(1024, 768).ok());
    EXPECT_TRUE(swapchain->is_valid());
    EXPECT_EQ(swapchain->width(), 1024u);
    EXPECT_EQ(swapchain->height(), 768u);
    EXPECT_EQ(swapchain->swapchain_generation_id(), 3u);

    EXPECT_TRUE(window->close().ok());
}

// ============================================================================
// 4. Real Metal Render Pass, Clear Screen & Screenshot Readback
// ============================================================================

TEST(MetalBackendGoogleTest, ClearScreenRenderPassAndReadback) {
    MacosPlatformHost host;
    ASSERT_TRUE(host.initialize().ok());

    auto win_res = host.create_window("Metal Clear Test", WindowSize{640, 480});
    ASSERT_TRUE(win_res.has_value());
    auto window = std::move(win_res).value();

    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Metal};
    auto dev_res = IRenderDevice::create(desc);
    if (!dev_res.has_value()) {
        GTEST_SKIP() << "IRenderDevice::create failed (likely sandboxed or headless): " << dev_res.status().message();
    }
    ASSERT_TRUE(dev_res.has_value());
    auto device = std::move(dev_res).value();

    auto sc_res = device->create_swapchain(window->native_surface_handle(), 640, 480);
    ASSERT_TRUE(sc_res.has_value());
    auto swapchain = std::move(sc_res).value();

    // Acquire CAMetalDrawable backbuffer
    auto backbuffer_res = swapchain->acquire_next_backbuffer();
    ASSERT_TRUE(backbuffer_res.has_value());
    auto* backbuffer = backbuffer_res.value();
    ASSERT_NE(backbuffer, nullptr);

    // Record clear pass
    auto cmd_res = device->create_command_buffer();
    ASSERT_TRUE(cmd_res.has_value());
    auto cmd = std::move(cmd_res).value();

    ColorClearValue clear_color{0.1f, 0.5f, 0.9f, 1.0f}; // Cornflower blue
    cmd->begin();
    cmd->begin_render_pass(backbuffer, clear_color);
    cmd->set_viewport(Viewport{0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f});
    cmd->set_scissor(ScissorRect{0, 0, 640, 480});
    cmd->end_render_pass();
    cmd->end();

    EXPECT_TRUE(device->submit(cmd.get()).ok());
    device->wait_idle();

    // Present frame to CAMetalLayer
    EXPECT_TRUE(swapchain->present(PresentMode::Immediate).ok());

    // Read back pixel data
    auto screenshot_res = swapchain->capture_screenshot();
    ASSERT_TRUE(screenshot_res.has_value());
    const auto& pixels = screenshot_res.value();
    EXPECT_EQ(pixels.size(), 640u * 480u * 4u);

    // Verify clear color applied (R around 0.1 * 255 = ~25, G around 0.5 * 255 = ~127, B around 0.9 * 255 = ~229)
    uint8_t r = pixels[0];
    uint8_t g = pixels[1];
    uint8_t b = pixels[2];
    uint8_t a = pixels[3];

    EXPECT_NEAR(r, 25, 5);
    EXPECT_NEAR(g, 127, 5);
    EXPECT_NEAR(b, 229, 5);
    EXPECT_EQ(a, 255);

    EXPECT_TRUE(window->close().ok());
}

// ============================================================================
// 5. Compositor 2D Display List Presentation on Metal
// ============================================================================

TEST(MetalBackendGoogleTest, CompositorDisplayListRendering) {
    MacosPlatformHost host;
    ASSERT_TRUE(host.initialize().ok());

    auto win_res = host.create_window("Metal 2D DisplayList Test", WindowSize{800, 600});
    ASSERT_TRUE(win_res.has_value());
    auto window = std::move(win_res).value();

    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Metal};
    auto dev_res = IRenderDevice::create(desc);
    if (!dev_res.has_value()) {
        GTEST_SKIP() << "IRenderDevice::create failed (likely sandboxed or headless): " << dev_res.status().message();
    }
    ASSERT_TRUE(dev_res.has_value());
    auto device = std::move(dev_res).value();

    auto sc_res = device->create_swapchain(window->native_surface_handle(), 800, 600);
    ASSERT_TRUE(sc_res.has_value());
    auto swapchain = std::move(sc_res).value();

    // Build 2D display list with UI shapes
    DisplayListBuilder builder;
    builder.draw_rect(
        pingdart::rhi::Rect::from_xywh(50.0f, 50.0f, 200.0f, 150.0f),
        Paint{.color = Color::from_argb(255, 240, 80, 80)}
    );
    builder.draw_rounded_rect(
        RRect::from_rect_and_radius(pingdart::rhi::Rect::from_xywh(300.0f, 50.0f, 200.0f, 150.0f), 12.0f),
        Paint{.color = Color::from_argb(255, 80, 200, 120)}
    );
    builder.draw_circle(
        pingdart::rhi::Point{150.0f, 350.0f},
        60.0f,
        Paint{.color = Color::from_argb(255, 80, 140, 240)}
    );
    auto dl = builder.finish_recording();

    // Render using PingCore's shared Compositor
    Compositor compositor(device.get());
    auto backbuffer = swapchain->acquire_next_backbuffer().value();
    ColorClearValue bg{0.08f, 0.12f, 0.18f, 1.0f};

    auto stats_res = compositor.render_frame(dl.get(), backbuffer, bg);
    ASSERT_TRUE(stats_res.has_value());
    const auto& stats = stats_res.value();

    EXPECT_EQ(stats.total_ops_evaluated, 3u);
    EXPECT_EQ(stats.ops_drawn, 3u);

    // Present to Metal display
    EXPECT_TRUE(swapchain->present(PresentMode::Mailbox).ok());

    EXPECT_TRUE(window->close().ok());
}
