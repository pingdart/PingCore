// tests/pal/test_mobile_platform.cpp
#include <gtest/gtest.h>
#include <pingdart/pal/android/android_platform.h>
#include <pingdart/pal/ios/ios_platform.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include <thread>
#include <vector>

using namespace pingdart::pal;
using namespace pingdart::pal::android;
using namespace pingdart::pal::ios;

namespace {

class MockInputDispatcher final : public IInputDispatcher {
public:
    std::vector<PointerEvent> pointer_events;
    std::vector<KeyboardEvent> keyboard_events;
    std::vector<WindowLifecycleEvent> lifecycle_events;

    void dispatch_pointer_event(const PointerEvent& event) override {
        pointer_events.push_back(event);
    }
    void dispatch_keyboard_event(const KeyboardEvent& event) override {
        keyboard_events.push_back(event);
    }
    void dispatch_lifecycle_event(const WindowLifecycleEvent& event) override {
        lifecycle_events.push_back(event);
    }

    void clear() {
        pointer_events.clear();
        keyboard_events.clear();
        lifecycle_events.clear();
    }
};

} // namespace

// ============================================================================
// Android Platform Tests
// ============================================================================

TEST(MobilePlatformGoogleTest, AndroidWindowSurfaceLifecycleAndGenerations) {
    AndroidWindow window("PingDartAndroid", WindowSize{1080, 2400});

    // 1. Initial State: Unattached native window
    EXPECT_FALSE(window.native_surface_handle().is_usable_for_rendering());
    EXPECT_EQ(window.surface_generation_id(), 0u);
    EXPECT_EQ(window.swapchain_generation_id(), 0u);
    EXPECT_EQ(window.surface_lifecycle_state(), SurfaceLifecycleState::Retired);

    // 2. Attach ANativeWindow
    void* mock_anative_window = reinterpret_cast<void*>(0xDEADBEEF);
    window.on_native_window_created(mock_anative_window, 1080, 2400);

    auto handle = window.native_surface_handle();
    EXPECT_TRUE(handle.is_usable_for_rendering());
    EXPECT_EQ(handle.kind, SurfaceKind::AndroidSurface);
    EXPECT_EQ(window.surface_generation_id(), 1u);
    EXPECT_EQ(window.swapchain_generation_id(), 1u);
    EXPECT_EQ(window.surface_lifecycle_state(), SurfaceLifecycleState::Active);

    ASSERT_TRUE(std::holds_alternative<AndroidSurfaceDescriptor>(handle.token));
    EXPECT_EQ(std::get<AndroidSurfaceDescriptor>(handle.token).native_window, mock_anative_window);

    // 3. Screen / Window Resize (e.g. keyboard shown or split-screen)
    window.on_native_window_resized(1080, 1800);
    EXPECT_EQ(window.surface_generation_id(), 1u); // Surface handle unchanged
    EXPECT_EQ(window.swapchain_generation_id(), 2u); // Swapchain generation incremented
    EXPECT_EQ(window.size().width, 1080u);
    EXPECT_EQ(window.size().height, 1800u);

    // 4. 4-Step Retirement Handshake
    window.mark_surface_retiring();
    EXPECT_EQ(window.surface_lifecycle_state(), SurfaceLifecycleState::Retiring);
    EXPECT_FALSE(window.is_surface_ready_for_destruction());

    window.acknowledge_surface_retirement();
    EXPECT_EQ(window.surface_lifecycle_state(), SurfaceLifecycleState::Retired);
    EXPECT_TRUE(window.is_surface_ready_for_destruction());
    EXPECT_FALSE(window.native_surface_handle().is_usable_for_rendering());
}

TEST(MobilePlatformGoogleTest, AndroidMultiTouchMotionTranslation) {
    MockInputDispatcher dispatcher;
    AndroidWindow window("PingDartAndroid", WindowSize{1080, 2400});
    window.set_input_dispatcher(&dispatcher);

    // 1. Single Touch Down
    TouchPoint t0{.touch_id = 10, .x = 250.0, .y = 500.0, .pressure = 0.75f, .phase = TouchPhase::Began, .timestamp_us = 1000};
    window.dispatch_motion_event(AndroidMotionEventAction::Down, 0, {t0});

    ASSERT_EQ(dispatcher.pointer_events.size(), 1u);
    EXPECT_EQ(dispatcher.pointer_events[0].type, EventType::PointerDown);
    EXPECT_EQ(dispatcher.pointer_events[0].pointer_id, 10u);
    EXPECT_DOUBLE_EQ(dispatcher.pointer_events[0].x, 250.0);
    EXPECT_DOUBLE_EQ(dispatcher.pointer_events[0].y, 500.0);
    EXPECT_FLOAT_EQ(dispatcher.pointer_events[0].pressure, 0.75f);
    EXPECT_EQ(dispatcher.pointer_events[0].button, PointerButton::Left);

    dispatcher.clear();

    // 2. Multi-Touch Move (Pinch Gesture)
    TouchPoint t0_move{.touch_id = 10, .x = 260.0, .y = 510.0, .pressure = 0.8f, .phase = TouchPhase::Moved, .timestamp_us = 2000};
    TouchPoint t1_move{.touch_id = 11, .x = 600.0, .y = 800.0, .pressure = 0.9f, .phase = TouchPhase::Moved, .timestamp_us = 2000};
    window.dispatch_motion_event(AndroidMotionEventAction::Move, 0, {t0_move, t1_move});

    ASSERT_EQ(dispatcher.pointer_events.size(), 2u);
    EXPECT_EQ(dispatcher.pointer_events[0].type, EventType::PointerMove);
    EXPECT_EQ(dispatcher.pointer_events[0].pointer_id, 10u);
    EXPECT_EQ(dispatcher.pointer_events[1].type, EventType::PointerMove);
    EXPECT_EQ(dispatcher.pointer_events[1].pointer_id, 11u);

    dispatcher.clear();

    // 3. Pointer Up
    window.dispatch_motion_event(AndroidMotionEventAction::PointerUp, 1, {t0_move, t1_move});
    ASSERT_EQ(dispatcher.pointer_events.size(), 1u);
    EXPECT_EQ(dispatcher.pointer_events[0].type, EventType::PointerUp);
    EXPECT_EQ(dispatcher.pointer_events[0].pointer_id, 11u);

    window.set_input_dispatcher(nullptr);
}

TEST(MobilePlatformGoogleTest, AndroidHostLifecycleTransitions) {
    AndroidPlatformHost host;
    EXPECT_EQ(host.platform_type(), PlatformType::Android);
    EXPECT_FALSE(host.is_quit_requested());

    auto init_status = host.initialize();
    EXPECT_TRUE(init_status.ok());
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

// ============================================================================
// iOS Platform Tests
// ============================================================================

TEST(MobilePlatformGoogleTest, IOSWindowMetalSurfaceLifecycleAndPhysicalScaling) {
    IOSWindow window("PingDartIOS", WindowSize{393, 852});

    EXPECT_FALSE(window.native_surface_handle().is_usable_for_rendering());
    EXPECT_EQ(window.surface_generation_id(), 0u);

    // Attach CAMetalLayer with 3.0x Retina scaling
    void* mock_uiview = reinterpret_cast<void*>(0x112233);
    void* mock_cametallayer = reinterpret_cast<void*>(0x445566);
    window.setup_metal_surface(mock_uiview, mock_cametallayer, 393, 852, 3.0f);

    auto handle = window.native_surface_handle();
    EXPECT_TRUE(handle.is_usable_for_rendering());
    EXPECT_EQ(handle.kind, SurfaceKind::IOSMetal);
    EXPECT_EQ(window.surface_generation_id(), 1u);
    EXPECT_FLOAT_EQ(window.scale_factor(), 3.0f);

    // Verify physical drawable dimensions
    auto phys_size = window.physical_size();
    EXPECT_EQ(phys_size.width, 393u * 3u);   // 1179
    EXPECT_EQ(phys_size.height, 852u * 3u);  // 2556

    ASSERT_TRUE(std::holds_alternative<IOSSurfaceDescriptor>(handle.token));
    const auto& desc = std::get<IOSSurfaceDescriptor>(handle.token);
    EXPECT_EQ(desc.ui_view, mock_uiview);
    EXPECT_EQ(desc.metal_layer, mock_cametallayer);

    // Orientation change to Landscape
    window.update_drawable_size(852, 393);
    EXPECT_EQ(window.surface_generation_id(), 1u);
    EXPECT_EQ(window.swapchain_generation_id(), 2u);
    EXPECT_EQ(window.physical_size().width, 852u * 3u);
    EXPECT_EQ(window.physical_size().height, 393u * 3u);

    // Surface detachment & retirement
    window.detach_metal_surface();
    EXPECT_EQ(window.surface_lifecycle_state(), SurfaceLifecycleState::Retired);
    EXPECT_TRUE(window.is_surface_ready_for_destruction());
    EXPECT_FALSE(window.native_surface_handle().is_usable_for_rendering());
}

TEST(MobilePlatformGoogleTest, IOSMultiTouchPhaseTranslation) {
    MockInputDispatcher dispatcher;
    IOSWindow window("PingDartIOS", WindowSize{393, 852});
    window.set_input_dispatcher(&dispatcher);

    // 1. Began -> PointerDown
    TouchPoint t0{.touch_id = 1, .x = 100.0, .y = 200.0, .pressure = 1.0f, .phase = TouchPhase::Began, .timestamp_us = 100};
    window.handle_ios_touches(TouchPhase::Began, {t0});
    ASSERT_EQ(dispatcher.pointer_events.size(), 1u);
    EXPECT_EQ(dispatcher.pointer_events[0].type, EventType::PointerDown);
    EXPECT_EQ(dispatcher.pointer_events[0].button, PointerButton::Left);

    dispatcher.clear();

    // 2. Moved -> PointerMove
    t0.phase = TouchPhase::Moved;
    t0.x = 105.0;
    t0.y = 205.0;
    window.handle_ios_touches(TouchPhase::Moved, {t0});
    ASSERT_EQ(dispatcher.pointer_events.size(), 1u);
    EXPECT_EQ(dispatcher.pointer_events[0].type, EventType::PointerMove);
    EXPECT_DOUBLE_EQ(dispatcher.pointer_events[0].x, 105.0);

    dispatcher.clear();

    // 3. Ended -> PointerUp
    t0.phase = TouchPhase::Ended;
    window.handle_ios_touches(TouchPhase::Ended, {t0});
    ASSERT_EQ(dispatcher.pointer_events.size(), 1u);
    EXPECT_EQ(dispatcher.pointer_events[0].type, EventType::PointerUp);

    dispatcher.clear();

    // 4. Cancelled -> PointerLeave
    t0.phase = TouchPhase::Cancelled;
    window.handle_ios_touches(TouchPhase::Cancelled, {t0});
    ASSERT_EQ(dispatcher.pointer_events.size(), 1u);
    EXPECT_EQ(dispatcher.pointer_events[0].type, EventType::PointerLeave);

    window.set_input_dispatcher(nullptr);
}

TEST(MobilePlatformGoogleTest, IOSSafeAreaInsetsHandling) {
    IOSWindow window("PingDartIOS", WindowSize{393, 852});

    // iPhone Dynamic Island / Notch Insets
    IOSSafeAreaInsets insets{.top = 59.0f, .bottom = 34.0f, .left = 0.0f, .right = 0.0f};
    window.set_safe_area_insets(insets);

    const auto& res = window.safe_area_insets();
    EXPECT_FLOAT_EQ(res.top, 59.0f);
    EXPECT_FLOAT_EQ(res.bottom, 34.0f);
    EXPECT_FLOAT_EQ(res.left, 0.0f);
    EXPECT_FLOAT_EQ(res.right, 0.0f);
}

TEST(MobilePlatformGoogleTest, IOSHostLifecycleTransitions) {
    IOSPlatformHost host;
    EXPECT_EQ(host.platform_type(), PlatformType::IOS);

    auto init_status = host.initialize();
    EXPECT_TRUE(init_status.ok());
    EXPECT_EQ(host.lifecycle_state(), IOSLifecycleState::Active);

    host.on_application_will_resign_active();
    EXPECT_EQ(host.lifecycle_state(), IOSLifecycleState::Inactive);

    host.on_application_did_enter_background();
    EXPECT_EQ(host.lifecycle_state(), IOSLifecycleState::Background);

    host.on_application_will_enter_foreground();
    EXPECT_EQ(host.lifecycle_state(), IOSLifecycleState::Inactive);

    host.on_application_did_become_active();
    EXPECT_EQ(host.lifecycle_state(), IOSLifecycleState::Active);
}

TEST(MobilePlatformGoogleTest, AndroidSurfaceRetirementRacePrevention) {
    AndroidWindow window("PingDartAndroid", WindowSize{1080, 2400});
    void* mock_window_1 = reinterpret_cast<void*>(0xABC111);
    window.on_native_window_created(mock_window_1, 1080, 2400);

    uint64_t gen1 = window.surface_generation_id();
    EXPECT_EQ(gen1, 1u);
    EXPECT_TRUE(window.can_render_frame(gen1));

    // Rapid background transition / native window destroyed by Android OS
    window.on_native_window_destroyed();

    // Render thread attempts to submit frame with old generation or during retirement
    EXPECT_FALSE(window.can_render_frame(gen1));
    EXPECT_FALSE(window.can_render_frame(window.surface_generation_id()));
    EXPECT_FALSE(window.native_surface_handle().is_usable_for_rendering());

    // Foreground resume: OS creates a new surface with a new pointer
    void* mock_window_2 = reinterpret_cast<void*>(0xABC222);
    window.on_native_window_created(mock_window_2, 1080, 2400);
    uint64_t gen2 = window.surface_generation_id();
    EXPECT_NE(gen2, gen1);
    EXPECT_FALSE(window.can_render_frame(gen1)); // Old generation rejected
    EXPECT_TRUE(window.can_render_frame(gen2));  // New generation accepted
}

TEST(MobilePlatformGoogleTest, IOSDynamicSafeAreaNotification) {
    IOSWindow window("PingDartIOS", WindowSize{393, 852});

    // Default safe area is 0 until dynamically provided by UIKit
    EXPECT_FLOAT_EQ(window.safe_area_insets().top, 0.0f);
    EXPECT_FLOAT_EQ(window.safe_area_insets().bottom, 0.0f);

    bool callback_called = false;
    IOSSafeAreaInsets notified_insets{};
    window.set_on_safe_area_changed([&](const IOSSafeAreaInsets& insets) {
        callback_called = true;
        notified_insets = insets;
    });

    // Dynamic rotation to portrait with Dynamic Island
    IOSSafeAreaInsets portrait_insets{.top = 59.0f, .bottom = 34.0f, .left = 0.0f, .right = 0.0f};
    window.update_safe_area_insets(portrait_insets);

    EXPECT_TRUE(callback_called);
    EXPECT_FLOAT_EQ(notified_insets.top, 59.0f);
    EXPECT_FLOAT_EQ(notified_insets.bottom, 34.0f);
    EXPECT_FLOAT_EQ(window.safe_area_insets().top, 59.0f);

    // Re-applying identical insets does not trigger spurious callback
    callback_called = false;
    window.update_safe_area_insets(portrait_insets);
    EXPECT_FALSE(callback_called);

    // Dynamic rotation to landscape (notch at left or right)
    IOSSafeAreaInsets landscape_insets{.top = 0.0f, .bottom = 21.0f, .left = 59.0f, .right = 0.0f};
    window.update_safe_area_insets(landscape_insets);
    EXPECT_TRUE(callback_called);
    EXPECT_FLOAT_EQ(notified_insets.left, 59.0f);
    EXPECT_FLOAT_EQ(notified_insets.bottom, 21.0f);
}
