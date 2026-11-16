#include <gtest/gtest.h>
#include <pingdart/rhi/i_render_device.h>
#include <pingdart/rhi/i_swapchain.h>
#include <pingdart/rhi/i_texture.h>
#include <pingdart/rhi/i_command_buffer.h>
#include "../../src/rhi/mock/mock_render_device.h"

using namespace pingdart;
using namespace pingdart::rhi;

TEST(RhiLifecycleTest, DeviceCreationAndCapabilities) {
    RenderDeviceDescriptor desc{
        .preferred_api = GraphicsApi::Mock,
        .enable_validation_layers = true,
        .prefer_discrete_gpu = true
    };

    auto device_result = IRenderDevice::create(desc);
    ASSERT_TRUE(device_result.has_value());

    auto device = std::move(device_result).value();
    EXPECT_EQ(device->api_type(), GraphicsApi::Mock);
    EXPECT_FALSE(device->device_name().empty());
    EXPECT_FALSE(device->is_device_lost());

    const auto& caps = device->capabilities();
    EXPECT_TRUE(caps.presentation_supported);
    EXPECT_TRUE(caps.timeline_semaphores);
    EXPECT_GE(caps.max_texture_dimension_2d, 8192u);
}

TEST(RhiLifecycleTest, SwapchainCreationAndAcquirePresent) {
    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Mock};
    auto device = IRenderDevice::create(desc).value();

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Mock,
        .token = pal::MockSurfaceDescriptor{101},
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto swapchain_res = device->create_swapchain(surface, 800, 600, PresentMode::Mailbox);
    ASSERT_TRUE(swapchain_res.has_value());
    auto swapchain = std::move(swapchain_res).value();

    EXPECT_EQ(swapchain->width(), 800u);
    EXPECT_EQ(swapchain->height(), 600u);
    EXPECT_EQ(swapchain->buffer_count(), 3u);
    EXPECT_TRUE(swapchain->is_valid());
    EXPECT_EQ(swapchain->swapchain_generation_id(), 1u);

    // Acquire and present multiple frames
    for (int frame = 0; frame < 6; ++frame) {
        auto backbuffer_res = swapchain->acquire_next_backbuffer();
        ASSERT_TRUE(backbuffer_res.has_value());
        ITexture* backbuffer = backbuffer_res.value();
        ASSERT_NE(backbuffer, nullptr);
        EXPECT_EQ(backbuffer->width(), 800u);
        EXPECT_EQ(backbuffer->height(), 600u);

        core::Status present_status = swapchain->present(PresentMode::Mailbox);
        EXPECT_TRUE(present_status.ok());
    }

    auto* mock_sc = static_cast<MockSwapchain*>(swapchain.get());
    EXPECT_EQ(mock_sc->present_count(), 6u);
}

TEST(RhiLifecycleTest, SwapchainResizeAndGenerationId) {
    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Mock};
    auto device = IRenderDevice::create(desc).value();

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Mock,
        .token = pal::MockSurfaceDescriptor{102},
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

    // Acquire new backbuffer matches new size
    auto backbuffer = swapchain->acquire_next_backbuffer().value();
    EXPECT_EQ(backbuffer->width(), 1920u);
    EXPECT_EQ(backbuffer->height(), 1080u);
}

TEST(RhiLifecycleTest, SwapchainZeroExtentHandling) {
    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Mock};
    auto device = IRenderDevice::create(desc).value();

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Mock,
        .token = pal::MockSurfaceDescriptor{103},
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto swapchain = device->create_swapchain(surface, 800, 600).value();

    // Minimize window -> Extent {0, 0}
    core::Status min_status = swapchain->resize(0, 0);
    EXPECT_TRUE(min_status.ok());
    EXPECT_FALSE(swapchain->is_valid());

    // Acquiring during zero extent safely returns Unavailable without crash
    auto acquire_res = swapchain->acquire_next_backbuffer();
    EXPECT_FALSE(acquire_res.has_value());
    EXPECT_EQ(acquire_res.status().code(), core::StatusCode::Unavailable);

    // Restore window extent
    core::Status restore_status = swapchain->resize(800, 600);
    EXPECT_TRUE(restore_status.ok());
    EXPECT_TRUE(swapchain->is_valid());

    auto restored_buffer = swapchain->acquire_next_backbuffer();
    EXPECT_TRUE(restored_buffer.has_value());
}

TEST(RhiLifecycleTest, SwapchainOutOfDateRecovery) {
    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Mock};
    auto device = IRenderDevice::create(desc).value();

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Mock,
        .token = pal::MockSurfaceDescriptor{104},
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    auto swapchain = device->create_swapchain(surface, 800, 600).value();
    auto* mock_sc = static_cast<MockSwapchain*>(swapchain.get());

    // Simulate driver signaling VK_ERROR_OUT_OF_DATE_KHR
    mock_sc->simulate_out_of_date(true);

    auto acquire_res = swapchain->acquire_next_backbuffer();
    EXPECT_FALSE(acquire_res.has_value());
    EXPECT_EQ(acquire_res.status().code(), core::StatusCode::Unavailable);
    EXPECT_NE(acquire_res.status().message().find("out of date"), std::string_view::npos);

    // Recreate / resize clears out of date
    core::Status resize_status = swapchain->resize(800, 600);
    EXPECT_TRUE(resize_status.ok());

    auto recovered_buffer = swapchain->acquire_next_backbuffer();
    EXPECT_TRUE(recovered_buffer.has_value());
}

TEST(RhiLifecycleTest, CommandBufferRecordingAndSubmission) {
    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Mock};
    auto device = IRenderDevice::create(desc).value();

    auto cmd_res = device->create_command_buffer();
    ASSERT_TRUE(cmd_res.has_value());
    auto cmd = std::move(cmd_res).value();

    EXPECT_EQ(cmd->state(), CommandBufferState::Initial);

    // Attempting to submit initial command buffer fails
    core::Status early_submit = device->submit(cmd.get());
    EXPECT_FALSE(early_submit.ok());
    EXPECT_EQ(early_submit.code(), core::StatusCode::FailedPrecondition);

    // Begin recording
    cmd->begin();
    EXPECT_EQ(cmd->state(), CommandBufferState::Recording);

    auto texture_res = device->create_texture(
        800, 600, PixelFormat::BGRA8_UNORM, TextureUsage::RenderTarget
    );
    ASSERT_TRUE(texture_res.has_value());
    auto texture = std::move(texture_res).value();

    cmd->begin_render_pass(texture.get(), ColorClearValue{0.0f, 0.5f, 1.0f, 1.0f});
    cmd->set_viewport(Viewport{0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f});
    cmd->set_scissor(ScissorRect{0, 0, 800, 600});
    cmd->draw(3, 0);
    cmd->end_render_pass();

    // End recording
    cmd->end();
    EXPECT_EQ(cmd->state(), CommandBufferState::Executable);

    // Submit to queue
    core::Status submit_status = device->submit(cmd.get());
    EXPECT_TRUE(submit_status.ok());
    EXPECT_EQ(cmd->state(), CommandBufferState::Submitted);

    auto* mock_device = static_cast<MockRenderDevice*>(device.get());
    EXPECT_EQ(mock_device->submit_count(), 1u);

    device->wait_idle();
    EXPECT_EQ(mock_device->idle_wait_count(), 1u);
}

TEST(RhiLifecycleTest, DeviceLossDetectionAndRecovery) {
    RenderDeviceDescriptor desc{.preferred_api = GraphicsApi::Mock};
    auto device = IRenderDevice::create(desc).value();
    auto* mock_device = static_cast<MockRenderDevice*>(device.get());

    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Mock,
        .token = pal::MockSurfaceDescriptor{105},
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };
    auto swapchain = device->create_swapchain(surface, 800, 600).value();

    // Trigger simulated device loss
    mock_device->simulate_device_lost(true);
    EXPECT_TRUE(device->is_device_lost());

    // Operations fail with Unavailable
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

    // 2. Re-instantiate fresh device
    auto recovered_device_res = IRenderDevice::create(desc);
    ASSERT_TRUE(recovered_device_res.has_value());
    auto recovered_device = std::move(recovered_device_res).value();
    EXPECT_FALSE(recovered_device->is_device_lost());

    // 3. Recreate swapchain on existing surface token
    auto recovered_swapchain = recovered_device->create_swapchain(surface, 800, 600);
    ASSERT_TRUE(recovered_swapchain.has_value());
    EXPECT_TRUE(recovered_swapchain.value()->is_valid());
}
