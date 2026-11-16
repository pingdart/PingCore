#include <gtest/gtest.h>
#include <pingdart/rhi/i_render_device.h>
#include <pingdart/rhi/i_swapchain.h>
#include <pingdart/rhi/i_texture.h>
#include <pingdart/rhi/i_command_buffer.h>
#include "../../src/rhi/vulkan/vulkan_device.h"
#include "../../src/rhi/vulkan/vulkan_swapchain.h"
#include "../../src/rhi/vulkan/vulkan_dispatcher.h"

using namespace pingdart;
using namespace pingdart::rhi;
using namespace pingdart::rhi::vk;

TEST(VulkanSwapchainTest, DeviceInitializationAndValidationLayers) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true,
        .prefer_discrete_gpu = true
    };

    auto device_res = IRenderDevice::create(desc);
    ASSERT_TRUE(device_res.has_value());
    auto device = std::move(device_res).value();

    EXPECT_EQ(device->api_type(), GraphicsApi::Vulkan);
    EXPECT_FALSE(device->device_name().empty());
    EXPECT_FALSE(device->is_device_lost());

    const auto& caps = device->capabilities();
    EXPECT_TRUE(caps.presentation_supported);
    EXPECT_TRUE(caps.timeline_semaphores);
    EXPECT_TRUE(caps.descriptor_indexing);
    EXPECT_GE(caps.max_texture_dimension_2d, 8192u);

    auto* vk_dev = dynamic_cast<VulkanRenderDevice*>(device.get());
    ASSERT_NE(vk_dev, nullptr);
    EXPECT_EQ(vk_dev->validation_error_count(), 0u);
    EXPECT_EQ(vk_dev->validation_warning_count(), 0u);
}

TEST(VulkanSwapchainTest, Win32SurfaceCreationAndPresentation) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true
    };
    auto device = IRenderDevice::create(desc).value();
    auto* vk_dev = static_cast<VulkanRenderDevice*>(device.get());

    pal::NativeSurfaceHandle win32_surface{
        .kind = pal::SurfaceKind::Win32,
        .token = pal::Win32SurfaceDescriptor{
            .hwnd = reinterpret_cast<void*>(0x880011),
            .hinstance = reinterpret_cast<void*>(0x400000)
        },
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto sc_res = device->create_swapchain(win32_surface, 1280, 720, PresentMode::Mailbox);
    ASSERT_TRUE(sc_res.has_value());
    auto swapchain = std::move(sc_res).value();

    EXPECT_EQ(swapchain->width(), 1280u);
    EXPECT_EQ(swapchain->height(), 720u);
    EXPECT_GE(swapchain->buffer_count(), 3u); // Triple buffering
    EXPECT_TRUE(swapchain->is_valid());
    EXPECT_EQ(swapchain->swapchain_generation_id(), 1u);

    // Frame presentation loop with clear-color command buffer
    for (int frame = 0; frame < 6; ++frame) {
        auto backbuffer_res = swapchain->acquire_next_backbuffer();
        ASSERT_TRUE(backbuffer_res.has_value());
        ITexture* backbuffer = backbuffer_res.value();
        ASSERT_NE(backbuffer, nullptr);

        auto cmd = device->create_command_buffer().value();
        cmd->begin();
        cmd->begin_render_pass(backbuffer, ColorClearValue{0.1f, 0.2f, 0.3f, 1.0f});
        cmd->set_viewport(Viewport{0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f});
        cmd->set_scissor(ScissorRect{0, 0, 1280, 720});
        cmd->draw(3, 0);
        cmd->end_render_pass();
        cmd->end();

        core::Status submit_status = device->submit(cmd.get());
        EXPECT_TRUE(submit_status.ok());

        core::Status present_status = swapchain->present(PresentMode::Mailbox);
        EXPECT_TRUE(present_status.ok());
    }

    EXPECT_EQ(vk_dev->submit_count(), 6u);
    EXPECT_EQ(vk_dev->validation_error_count(), 0u);
}

TEST(VulkanSwapchainTest, WaylandSurfaceCreationAndPresentation) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true
    };
    auto device = IRenderDevice::create(desc).value();
    auto* vk_dev = static_cast<VulkanRenderDevice*>(device.get());

    pal::NativeSurfaceHandle wayland_surface{
        .kind = pal::SurfaceKind::Wayland,
        .token = pal::WaylandSurfaceDescriptor{
            .display = reinterpret_cast<void*>(0x10001),
            .surface = reinterpret_cast<void*>(0x20002)
        },
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto sc_res = device->create_swapchain(wayland_surface, 1920, 1080, PresentMode::Fifo);
    ASSERT_TRUE(sc_res.has_value());
    auto swapchain = std::move(sc_res).value();

    EXPECT_EQ(swapchain->width(), 1920u);
    EXPECT_EQ(swapchain->height(), 1080u);

    for (int frame = 0; frame < 4; ++frame) {
        auto buffer = swapchain->acquire_next_backbuffer();
        ASSERT_TRUE(buffer.has_value());
        EXPECT_TRUE(swapchain->present(PresentMode::Fifo).ok());
    }

    EXPECT_EQ(vk_dev->validation_error_count(), 0u);
}

TEST(VulkanSwapchainTest, X11SurfaceCreationAndPresentation) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true
    };
    auto device = IRenderDevice::create(desc).value();
    auto* vk_dev = static_cast<VulkanRenderDevice*>(device.get());

    pal::NativeSurfaceHandle x11_surface{
        .kind = pal::SurfaceKind::X11,
        .token = pal::X11SurfaceDescriptor{
            .display = reinterpret_cast<void*>(0x99901),
            .window = 0x54321,
            .visual = nullptr
        },
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto sc_res = device->create_swapchain(x11_surface, 1024, 768, PresentMode::Fifo);
    ASSERT_TRUE(sc_res.has_value());
    auto swapchain = std::move(sc_res).value();

    EXPECT_EQ(swapchain->width(), 1024u);
    EXPECT_EQ(swapchain->height(), 768u);

    for (int frame = 0; frame < 3; ++frame) {
        auto buffer = swapchain->acquire_next_backbuffer();
        ASSERT_TRUE(buffer.has_value());
        EXPECT_TRUE(swapchain->present(PresentMode::Fifo).ok());
    }

    EXPECT_EQ(vk_dev->validation_error_count(), 0u);
}

TEST(VulkanSwapchainTest, SurfaceRetirementAndLifecycleEnforcement) {
    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Vulkan};
    auto device = IRenderDevice::create(desc).value();

    pal::NativeSurfaceHandle retiring_surface{
        .kind = pal::SurfaceKind::Win32,
        .token = pal::Win32SurfaceDescriptor{
            .hwnd = reinterpret_cast<void*>(0x880022),
            .hinstance = reinterpret_cast<void*>(0x400000)
        },
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Retiring,
        .is_valid = true
    };

    // Submissions and swapchain creation rejected when retiring
    auto sc_res = device->create_swapchain(retiring_surface, 800, 600);
    EXPECT_FALSE(sc_res.has_value());
    EXPECT_EQ(sc_res.status().code(), core::StatusCode::FailedPrecondition);

    // Abandoned surface also rejected
    retiring_surface.lifecycle_state = pal::SurfaceLifecycleState::Abandoned;
    auto sc_abandoned = device->create_swapchain(retiring_surface, 800, 600);
    EXPECT_FALSE(sc_abandoned.has_value());
    EXPECT_EQ(sc_abandoned.status().code(), core::StatusCode::FailedPrecondition);
}

TEST(VulkanSwapchainTest, DynamicResizeAndGenerationIdIncrement) {
    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Vulkan};
    auto device = IRenderDevice::create(desc).value();
    auto* vk_dev = static_cast<VulkanRenderDevice*>(device.get());

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Mock,
        .token = pal::MockSurfaceDescriptor{101},
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto swapchain = device->create_swapchain(surface, 800, 600).value();
    EXPECT_EQ(swapchain->swapchain_generation_id(), 1u);

    // Resize to 1920x1080
    core::Status resize_status = swapchain->resize(1920, 1080);
    EXPECT_TRUE(resize_status.ok());
    EXPECT_EQ(swapchain->width(), 1920u);
    EXPECT_EQ(swapchain->height(), 1080u);
    EXPECT_EQ(swapchain->swapchain_generation_id(), 2u);

    auto backbuffer = swapchain->acquire_next_backbuffer().value();
    EXPECT_EQ(backbuffer->width(), 1920u);
    EXPECT_EQ(backbuffer->height(), 1080u);

    EXPECT_EQ(vk_dev->validation_error_count(), 0u);
}

TEST(VulkanSwapchainTest, ZeroExtentMinimizationHandling) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true
    };
    auto device = IRenderDevice::create(desc).value();
    auto* vk_dev = static_cast<VulkanRenderDevice*>(device.get());

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Win32,
        .token = pal::Win32SurfaceDescriptor{
            .hwnd = reinterpret_cast<void*>(0x880033),
            .hinstance = reinterpret_cast<void*>(0x400000)
        },
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto swapchain = device->create_swapchain(surface, 800, 600).value();

    // Minimize window -> Extent {0, 0}
    // Must NOT call vkCreateSwapchainKHR with zero extent (which triggers VUID-01274)
    core::Status min_status = swapchain->resize(0, 0);
    EXPECT_TRUE(min_status.ok());
    EXPECT_FALSE(swapchain->is_valid());

    // Acquiring during zero extent safely returns Unavailable without crash
    auto acquire_res = swapchain->acquire_next_backbuffer();
    EXPECT_FALSE(acquire_res.has_value());
    EXPECT_EQ(acquire_res.status().code(), core::StatusCode::Unavailable);

    // Presenting during zero extent also safely rejected
    core::Status present_status = swapchain->present();
    EXPECT_FALSE(present_status.ok());
    EXPECT_EQ(present_status.code(), core::StatusCode::Unavailable);

    // ZERO validation errors must have been triggered
    EXPECT_EQ(vk_dev->validation_error_count(), 0u);

    // Restore window extent to 1024x768
    core::Status restore_status = swapchain->resize(1024, 768);
    EXPECT_TRUE(restore_status.ok());
    EXPECT_TRUE(swapchain->is_valid());
    EXPECT_EQ(swapchain->width(), 1024u);
    EXPECT_EQ(swapchain->height(), 768u);

    auto restored_buffer = swapchain->acquire_next_backbuffer();
    EXPECT_TRUE(restored_buffer.has_value());
    EXPECT_TRUE(swapchain->present().ok());

    EXPECT_EQ(vk_dev->validation_error_count(), 0u);
}

TEST(VulkanSwapchainTest, OutOfDateDynamicRecreation) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true
    };
    auto device = IRenderDevice::create(desc).value();
    auto* vk_dev = static_cast<VulkanRenderDevice*>(device.get());
    auto* mock_dispatcher = static_cast<MockVulkanDispatcher*>(vk_dev->dispatcher());

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Wayland,
        .token = pal::WaylandSurfaceDescriptor{
            .display = reinterpret_cast<void*>(0x10002),
            .surface = reinterpret_cast<void*>(0x20003)
        },
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto swapchain = device->create_swapchain(surface, 800, 600).value();
    auto* vk_sc = static_cast<VulkanSwapchain*>(swapchain.get());

    // Simulate driver signaling VK_ERROR_OUT_OF_DATE_KHR on acquire
    mock_dispatcher->simulate_out_of_date(true);

    auto acquire_res = swapchain->acquire_next_backbuffer();
    EXPECT_FALSE(acquire_res.has_value());
    EXPECT_EQ(acquire_res.status().code(), core::StatusCode::Unavailable);
    EXPECT_TRUE(vk_sc->is_out_of_date());

    // Clear simulation flag and resize / recreate
    mock_dispatcher->simulate_out_of_date(false);
    core::Status resize_status = swapchain->resize(800, 600);
    EXPECT_TRUE(resize_status.ok());
    EXPECT_FALSE(vk_sc->is_out_of_date());

    // Normal acquire and present succeeds
    auto recovered_buffer = swapchain->acquire_next_backbuffer();
    EXPECT_TRUE(recovered_buffer.has_value());
    EXPECT_TRUE(swapchain->present().ok());

    EXPECT_EQ(vk_dev->validation_error_count(), 0u);
}

TEST(VulkanSwapchainTest, DeviceLossDetectionAndRecoveryPipeline) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true
    };
    auto device = IRenderDevice::create(desc).value();
    auto* vk_dev = static_cast<VulkanRenderDevice*>(device.get());
    auto* mock_dispatcher = static_cast<MockVulkanDispatcher*>(vk_dev->dispatcher());

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Win32,
        .token = pal::Win32SurfaceDescriptor{
            .hwnd = reinterpret_cast<void*>(0x880044),
            .hinstance = reinterpret_cast<void*>(0x400000)
        },
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto swapchain = device->create_swapchain(surface, 800, 600).value();

    // Trigger simulated device loss
    mock_dispatcher->simulate_device_lost(true);
    device->wait_idle();
    EXPECT_TRUE(device->is_device_lost());

    // Operations fail safely with Unavailable
    auto cmd = device->create_command_buffer();
    EXPECT_FALSE(cmd.has_value());
    EXPECT_EQ(cmd.status().code(), core::StatusCode::Unavailable);

    auto tex = device->create_texture(100, 100, PixelFormat::RGBA8_UNORM, TextureUsage::Sampled);
    EXPECT_FALSE(tex.has_value());
    EXPECT_EQ(tex.status().code(), core::StatusCode::Unavailable);

    // Resilient Recovery Pipeline:
    // 1. Release resources
    swapchain.reset();
    device.reset();

    // 2. Re-instantiate fresh Vulkan device
    auto recovered_device_res = IRenderDevice::create(desc);
    ASSERT_TRUE(recovered_device_res.has_value());
    auto recovered_device = std::move(recovered_device_res).value();
    EXPECT_FALSE(recovered_device->is_device_lost());

    // 3. Recreate swapchain on existing surface token
    auto recovered_swapchain = recovered_device->create_swapchain(surface, 800, 600);
    ASSERT_TRUE(recovered_swapchain.has_value());
    EXPECT_TRUE(recovered_swapchain.value()->is_valid());
}
