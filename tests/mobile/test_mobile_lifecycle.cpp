// tests/mobile/test_mobile_lifecycle.cpp
#include <gtest/gtest.h>
#include <pingdart/pal/android/android_platform.h>
#include <pingdart/pal/ios/ios_platform.h>
#include <thread>
#include <vector>

using namespace pingdart::pal;
using namespace pingdart::pal::android;
using namespace pingdart::pal::ios;

// ============================================================================
// Android Mobile Lifecycle & Surface Retirement Tests
// ============================================================================

TEST(MobileLifecycleGoogleTest, AndroidStateTransitions) {
    AndroidPlatformHost host;
    EXPECT_EQ(host.platform_type(), PlatformType::Android);

    EXPECT_TRUE(host.initialize().ok());
    EXPECT_EQ(host.lifecycle_state(), AndroidLifecycleState::Created);

    host.handle_app_cmd(AndroidAppCmd::Start);
    EXPECT_EQ(host.lifecycle_state(), AndroidLifecycleState::Started);

    host.handle_app_cmd(AndroidAppCmd::Resume);
    EXPECT_EQ(host.lifecycle_state(), AndroidLifecycleState::Resumed);

    host.handle_app_cmd(AndroidAppCmd::Pause);
    EXPECT_EQ(host.lifecycle_state(), AndroidLifecycleState::Paused);

    host.handle_app_cmd(AndroidAppCmd::Stop);
    EXPECT_EQ(host.lifecycle_state(), AndroidLifecycleState::Stopped);

    host.handle_app_cmd(AndroidAppCmd::Destroy);
    EXPECT_EQ(host.lifecycle_state(), AndroidLifecycleState::Destroyed);
    EXPECT_TRUE(host.is_quit_requested());
}

TEST(MobileLifecycleGoogleTest, AndroidSurfaceRetirementHandshake) {
    AndroidWindow window("PingDartAndroid", WindowSize{1080, 2400});

    void* mock_native_window = reinterpret_cast<void*>(0x11223344);
    window.on_native_window_created(mock_native_window, 1080, 2400);

    uint64_t gen1 = window.surface_generation_id();
    EXPECT_EQ(gen1, 1u);
    EXPECT_EQ(window.surface_lifecycle_state(), SurfaceLifecycleState::Active);
    EXPECT_TRUE(window.can_render_frame(gen1));

    // Step 1: OS signals surface is retiring
    window.mark_surface_retiring();
    EXPECT_EQ(window.surface_lifecycle_state(), SurfaceLifecycleState::Retiring);
    EXPECT_FALSE(window.is_surface_ready_for_destruction());

    // Step 2: Render thread acknowledges retirement
    window.acknowledge_surface_retirement();
    EXPECT_EQ(window.surface_lifecycle_state(), SurfaceLifecycleState::Retired);
    EXPECT_TRUE(window.is_surface_ready_for_destruction());
    EXPECT_FALSE(window.can_render_frame(gen1));

    // Step 3: Native window destroyed by OS
    window.on_native_window_destroyed();
    EXPECT_FALSE(window.native_surface_handle().is_usable_for_rendering());
}

// ============================================================================
// iOS Mobile Lifecycle & Surface Retiring Tests
// ============================================================================

TEST(MobileLifecycleGoogleTest, IOSStateTransitions) {
    IOSPlatformHost host;
    EXPECT_EQ(host.platform_type(), PlatformType::IOS);

    EXPECT_TRUE(host.initialize().ok());
    EXPECT_EQ(host.lifecycle_state(), IOSLifecycleState::Active);

    host.on_application_will_resign_active();
    EXPECT_EQ(host.lifecycle_state(), IOSLifecycleState::Inactive);

    host.on_application_did_enter_background();
    EXPECT_EQ(host.lifecycle_state(), IOSLifecycleState::Background);

    host.on_application_will_enter_foreground();
    EXPECT_EQ(host.lifecycle_state(), IOSLifecycleState::Inactive);

    host.on_application_did_become_active();
    EXPECT_EQ(host.lifecycle_state(), IOSLifecycleState::Active);

    host.shutdown();
    EXPECT_EQ(host.lifecycle_state(), IOSLifecycleState::NotRunning);
}

TEST(MobileLifecycleGoogleTest, IOSSurfaceLifecycleAndRetirement) {
    IOSWindow window("PingDartIOS", WindowSize{393, 852});

    void* mock_view = reinterpret_cast<void*>(0xAAAA1111);
    void* mock_layer = reinterpret_cast<void*>(0xBBBB2222);

    window.setup_metal_surface(mock_view, mock_layer, 393, 852, 3.0f);
    EXPECT_EQ(window.surface_lifecycle_state(), SurfaceLifecycleState::Active);
    EXPECT_TRUE(window.native_surface_handle().is_usable_for_rendering());
    EXPECT_EQ(window.surface_generation_id(), 1u);

    // Orientation change to Landscape
    window.update_drawable_size(852, 393);
    EXPECT_EQ(window.surface_generation_id(), 1u);
    EXPECT_EQ(window.swapchain_generation_id(), 2u);

    // Surface retirement
    window.mark_surface_retiring();
    EXPECT_EQ(window.surface_lifecycle_state(), SurfaceLifecycleState::Retiring);

    window.acknowledge_surface_retirement();
    EXPECT_EQ(window.surface_lifecycle_state(), SurfaceLifecycleState::Retired);
    EXPECT_TRUE(window.is_surface_ready_for_destruction());

    window.detach_metal_surface();
    EXPECT_FALSE(window.native_surface_handle().is_usable_for_rendering());
}
