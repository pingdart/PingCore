// tests/mobile/test_mobile_touch.cpp
#include <gtest/gtest.h>
#include <pingdart/pal/android/android_platform.h>
#include <pingdart/pal/ios/ios_platform.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include "../../examples/shared_login/login_app.h"
#include <vector>

using namespace pingdart::pal;
using namespace pingdart::pal::android;
using namespace pingdart::pal::ios;
using namespace pingdart::examples;

namespace {

class MobileTouchCollector final : public IInputDispatcher {
public:
    std::vector<PointerEvent> events;

    void dispatch_pointer_event(const PointerEvent& event) override {
        events.push_back(event);
    }
    void dispatch_keyboard_event(const KeyboardEvent&) override {}
    void dispatch_lifecycle_event(const WindowLifecycleEvent&) override {}

    void clear() { events.clear(); }
};

} // namespace

// ============================================================================
// Android Multi-Touch Tests
// ============================================================================

TEST(MobileTouchGoogleTest, AndroidSimultaneousMultiTouchDispatch) {
    MobileTouchCollector collector;
    AndroidWindow window("PingDartAndroid", WindowSize{1080, 2400});
    window.set_input_dispatcher(&collector);

    // Primary Finger Down (ID 0)
    TouchPoint t0{.touch_id = 0, .x = 300.0, .y = 600.0, .pressure = 0.8f, .phase = TouchPhase::Began, .timestamp_us = 100};
    window.dispatch_motion_event(AndroidMotionEventAction::Down, 0, {t0});

    ASSERT_EQ(collector.events.size(), 1u);
    EXPECT_EQ(collector.events[0].type, EventType::PointerDown);
    EXPECT_EQ(collector.events[0].pointer_id, 0u);
    EXPECT_DOUBLE_EQ(collector.events[0].x, 300.0);
    EXPECT_DOUBLE_EQ(collector.events[0].y, 600.0);

    collector.clear();

    // Secondary Finger Down (ID 1)
    TouchPoint t1{.touch_id = 1, .x = 700.0, .y = 800.0, .pressure = 0.9f, .phase = TouchPhase::Began, .timestamp_us = 110};
    window.dispatch_motion_event(AndroidMotionEventAction::PointerDown, 1, {t0, t1});

    ASSERT_EQ(collector.events.size(), 1u);
    EXPECT_EQ(collector.events[0].type, EventType::PointerDown);
    EXPECT_EQ(collector.events[0].pointer_id, 1u);
    EXPECT_DOUBLE_EQ(collector.events[0].x, 700.0);

    collector.clear();

    // Both Fingers Move (Pinch Gesture)
    t0.x = 320.0;
    t1.x = 680.0;
    window.dispatch_motion_event(AndroidMotionEventAction::Move, 0, {t0, t1});

    ASSERT_EQ(collector.events.size(), 2u);
    EXPECT_EQ(collector.events[0].type, EventType::PointerMove);
    EXPECT_EQ(collector.events[0].pointer_id, 0u);
    EXPECT_EQ(collector.events[1].type, EventType::PointerMove);
    EXPECT_EQ(collector.events[1].pointer_id, 1u);

    collector.clear();

    // Secondary Finger Up
    window.dispatch_motion_event(AndroidMotionEventAction::PointerUp, 1, {t0, t1});
    ASSERT_EQ(collector.events.size(), 1u);
    EXPECT_EQ(collector.events[0].type, EventType::PointerUp);
    EXPECT_EQ(collector.events[0].pointer_id, 1u);

    window.set_input_dispatcher(nullptr);
}

// ============================================================================
// iOS UIKit Multi-Touch Translation Tests
// ============================================================================

TEST(MobileTouchGoogleTest, IOSTouchPhaseMappingAndCancellation) {
    MobileTouchCollector collector;
    IOSWindow window("PingDartIOS", WindowSize{393, 852});
    window.set_input_dispatcher(&collector);

    TouchPoint t0{.touch_id = 42, .x = 150.0, .y = 300.0, .pressure = 1.0f, .phase = TouchPhase::Began, .timestamp_us = 200};
    window.handle_ios_touches(TouchPhase::Began, {t0});
    ASSERT_EQ(collector.events.size(), 1u);
    EXPECT_EQ(collector.events[0].type, EventType::PointerDown);
    EXPECT_EQ(collector.events[0].pointer_id, 42u);

    collector.clear();

    // Touch Moved
    t0.phase = TouchPhase::Moved;
    t0.x = 160.0;
    window.handle_ios_touches(TouchPhase::Moved, {t0});
    ASSERT_EQ(collector.events.size(), 1u);
    EXPECT_EQ(collector.events[0].type, EventType::PointerMove);
    EXPECT_DOUBLE_EQ(collector.events[0].x, 160.0);

    collector.clear();

    // System Gesture Interruption (e.g., iOS Control Center swipe) triggers Cancelled
    t0.phase = TouchPhase::Cancelled;
    window.handle_ios_touches(TouchPhase::Cancelled, {t0});
    ASSERT_EQ(collector.events.size(), 1u);
    EXPECT_EQ(collector.events[0].type, EventType::PointerLeave);

    window.set_input_dispatcher(nullptr);
}

// ============================================================================
// SharedLoginApp Interactive Touch & State Update Tests
// ============================================================================

TEST(MobileTouchGoogleTest, SharedLoginAppInteractiveTouchWorkflow) {
    SharedLoginApp app;
    app.set_screen_geometry(393.0f, 852.0f, {59.0f, 34.0f, 0.0f, 0.0f});

    EXPECT_FALSE(app.is_logged_in());
    EXPECT_EQ(app.username(), "developer@pingdart.dev");

    // Touch focus on username field
    app.handle_pointer_down(196.0, 310.0);
    app.handle_pointer_up(196.0, 310.0);

    // Enter additional text
    app.handle_text_input(".mobile");

    // Navigate to submit button via Tab keys
    app.handle_key_down(9); // Tab to password field
    app.handle_key_down(9); // Tab to submit button
    app.handle_key_down(13); // Enter to activate button
    EXPECT_TRUE(app.is_logged_in());
}
