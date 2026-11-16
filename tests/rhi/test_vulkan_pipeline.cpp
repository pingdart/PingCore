#include <gtest/gtest.h>
#include <pingdart/rhi/i_render_device.h>
#include <pingdart/rhi/i_swapchain.h>
#include "../../src/rhi/vulkan/vulkan_device.h"
#include "../../src/rhi/vulkan/vulkan_dispatcher_dynamic.h"
#include "../../src/rhi/vulkan/vulkan_glyph_pipeline.h"

using namespace pingdart;
using namespace pingdart::rhi;
using namespace pingdart::rhi::vk;

TEST(VulkanPipelineTest, DynamicDispatcherDetectsDriverOrFallback) {
    DynamicVulkanDispatcher dispatcher;
    EXPECT_FALSE(dispatcher.driver_name().empty());
}

TEST(VulkanPipelineTest, EnumeratesPhysicalDevicesAndQueueFamilies) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true,
        .prefer_discrete_gpu = true
    };

    auto dev_res = IRenderDevice::create(desc);
    ASSERT_TRUE(dev_res.has_value());

    auto device = std::move(dev_res).value();
    EXPECT_FALSE(device->device_name().empty());
    EXPECT_TRUE(device->capabilities().presentation_supported);
    EXPECT_GE(device->capabilities().max_texture_dimension_2d, 4096u);
}

TEST(VulkanPipelineTest, SwapchainCreationAndMultiBuffering) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true
    };

    auto dev_res = IRenderDevice::create(desc);
    ASSERT_TRUE(dev_res.has_value());
    auto device = std::move(dev_res).value();

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Win32,
        .token = pal::Win32SurfaceDescriptor{
            .hwnd = reinterpret_cast<void*>(0x12345),
            .hinstance = reinterpret_cast<void*>(0x67890)
        },
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto sc_res = device->create_swapchain(surface, 1280, 720, PresentMode::Mailbox);
    ASSERT_TRUE(sc_res.has_value());

    auto swapchain = std::move(sc_res).value();
    EXPECT_EQ(swapchain->width(), 1280u);
    EXPECT_EQ(swapchain->height(), 720u);
    EXPECT_GE(swapchain->buffer_count(), 2u);

    auto backbuffer = swapchain->acquire_next_backbuffer();
    ASSERT_TRUE(backbuffer.has_value());

    auto present_status = swapchain->present(PresentMode::Mailbox);
    EXPECT_TRUE(present_status.ok());
}

TEST(VulkanPipelineTest, GlyphAtlasStagingUploadAndQuadRecording) {
    auto dispatcher = std::make_unique<MockVulkanDispatcher>();
    VkDevice mock_device = reinterpret_cast<VkDevice>(uintptr_t(101));
    VkCommandBuffer mock_cmd = reinterpret_cast<VkCommandBuffer>(uintptr_t(202));

    VulkanGlyphPipeline pipeline(dispatcher.get(), mock_device, 1024, GlyphAtlasFormat::Rgba8);
    EXPECT_EQ(pipeline.atlas_size(), 1024u);

    // Prepare a mock 64x64 RGBA bitmap dirty region
    std::vector<uint8_t> mock_pixels(64 * 64 * 4, 255);
    bool uploaded = pipeline.upload_dirty_region(mock_cmd, 10, 10, 64, 64, mock_pixels.data());
    EXPECT_TRUE(uploaded);
    EXPECT_EQ(pipeline.uploaded_bytes(), 64u * 64u * 4u);

    // Record 12 glyph quads (representing a shaped word)
    pipeline.record_glyph_quads(mock_cmd, 12);
    EXPECT_EQ(pipeline.draw_quad_count(), 12u);

    // Verify zero validation errors reported
    EXPECT_EQ(dispatcher->validation_error_count(), 0u);
}

TEST(VulkanPipelineTest, GlyphAtlasAlpha8FormatUpload) {
    auto dispatcher = std::make_unique<MockVulkanDispatcher>();
    VkDevice mock_device = reinterpret_cast<VkDevice>(uintptr_t(101));
    VkCommandBuffer mock_cmd = reinterpret_cast<VkCommandBuffer>(uintptr_t(202));

    VulkanGlyphPipeline pipeline(dispatcher.get(), mock_device, 1024, GlyphAtlasFormat::Alpha8);
    EXPECT_EQ(pipeline.format(), GlyphAtlasFormat::Alpha8);

    // Single-channel 8-bit FreeType bitmap (64x64 = 4096 bytes)
    std::vector<uint8_t> mock_alpha(64 * 64, 180);
    bool uploaded = pipeline.upload_dirty_region(mock_cmd, 0, 0, 64, 64, mock_alpha.data());
    EXPECT_TRUE(uploaded);
    EXPECT_EQ(pipeline.uploaded_bytes(), 64u * 64u * 1u);
}

TEST(VulkanPipelineTest, MetalSurfaceCreationOnMacOS) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan
    };

    auto dev_res = IRenderDevice::create(desc);
    ASSERT_TRUE(dev_res.has_value());
    auto device = std::move(dev_res).value();

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Metal,
        .token = pal::MetalSurfaceDescriptor{
            .layer = reinterpret_cast<void*>(0xCAFEF00D)
        },
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto sc_res = device->create_swapchain(surface, 800, 600);
    ASSERT_TRUE(sc_res.has_value());
    auto sc = std::move(sc_res).value();
    EXPECT_EQ(sc->width(), 800u);
    EXPECT_EQ(sc->height(), 600u);

    // Test Framebuffer Screenshot Capture
    auto screenshot = sc->capture_screenshot();
    ASSERT_TRUE(screenshot.has_value());
    EXPECT_EQ(screenshot.value().size(), 800u * 600u * 4u);
}

TEST(VulkanPipelineTest, StrictHardwareModeRejectsMissingDriverWhenRequested) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan,
        .require_hardware_adapter = true
    };

    auto dev_res = IRenderDevice::create(desc);
    // On systems without a real physical Vulkan ICD/library loaded,
    // strict hardware mode must return StatusCode::Unavailable rather than silently falling back to mock.
    DynamicVulkanDispatcher dynamic_check;
    if (!dynamic_check.is_loaded()) {
        EXPECT_FALSE(dev_res.has_value());
        EXPECT_EQ(dev_res.status().code(), core::StatusCode::Unavailable);
    } else {
        EXPECT_TRUE(dev_res.has_value());
    }
}
