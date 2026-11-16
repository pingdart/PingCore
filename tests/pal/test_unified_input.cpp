// tests/pal/test_unified_input.cpp
#include <gtest/gtest.h>
#include <pingdart/pal/native_types.h>
#include <pingdart/pal/input_events.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include <pingdart/pal/i_window.h>
#include <string>
#include <vector>

using namespace pingdart::pal;
using namespace pingdart::core;

namespace {

class MockInputTracker final : public IInputDispatcher, public ITextInputClient {
public:
    std::vector<KeyEvent> key_events;
    std::vector<TextInputEvent> text_events;
    std::vector<PointerEvent> pointer_events;
    std::vector<bool> focus_history;

    // IInputDispatcher
    void dispatch_pointer_event(const PointerEvent& event) override {
        pointer_events.push_back(event);
    }

    void dispatch_keyboard_event(const KeyboardEvent& event) override {
        (void)event;
    }

    void dispatch_lifecycle_event(const WindowLifecycleEvent& event) override {
        (void)event;
    }

    void dispatch_key_event(const KeyEvent& event) override {
        key_events.push_back(event);
    }

    void dispatch_text_event(const TextInputEvent& event) override {
        text_events.push_back(event);
    }

    // ITextInputClient
    void on_text_input(const TextInputEvent& event) override {
        text_events.push_back(event);
    }

    void on_focus_changed(bool focused) override {
        focus_history.push_back(focused);
    }
};

} // namespace

TEST(UnifiedInputTest, PhysicalAndLogicalKeysRemainDistinct) {
    MockInputTracker tracker;

    // Pressing 'A' on QWERTY: physical scancode 0x1E (30), logical virtual key 0x41 ('A')
    KeyEvent event_a;
    event_a.physical_key = 0x1E;
    event_a.logical_key = 0x41;
    event_a.action = KeyAction::Down;
    event_a.modifiers = static_cast<uint32_t>(ModifierKey::None);
    event_a.timestamp_ns = 1000000;

    tracker.dispatch_key_event(event_a);
    ASSERT_EQ(tracker.key_events.size(), 1u);
    EXPECT_EQ(tracker.key_events[0].physical_key, 0x1Eu);
    EXPECT_EQ(tracker.key_events[0].logical_key, 0x41u);
    EXPECT_NE(tracker.key_events[0].physical_key, tracker.key_events[0].logical_key);
}

TEST(UnifiedInputTest, KeyActionLifecycleAndRepeat) {
    MockInputTracker tracker;

    KeyEvent down{30, 65, KeyAction::Down, 0, 100};
    KeyEvent repeat1{30, 65, KeyAction::Repeat, 0, 200};
    KeyEvent repeat2{30, 65, KeyAction::Repeat, 0, 300};
    KeyEvent up{30, 65, KeyAction::Up, 0, 400};

    tracker.dispatch_key_event(down);
    tracker.dispatch_key_event(repeat1);
    tracker.dispatch_key_event(repeat2);
    tracker.dispatch_key_event(up);

    ASSERT_EQ(tracker.key_events.size(), 4u);
    EXPECT_EQ(tracker.key_events[0].action, KeyAction::Down);
    EXPECT_EQ(tracker.key_events[1].action, KeyAction::Repeat);
    EXPECT_EQ(tracker.key_events[2].action, KeyAction::Repeat);
    EXPECT_EQ(tracker.key_events[3].action, KeyAction::Up);
}

TEST(UnifiedInputTest, UnicodeCommittedTextDelivery) {
    MockInputTracker tracker;

    TextInputEvent commit_event;
    commit_event.type = TextEventType::CompositionCommit;
    commit_event.utf8_text = "PingDart 🎯 UI";
    commit_event.selection_start = static_cast<uint32_t>(commit_event.utf8_text.size());
    commit_event.selection_end = commit_event.selection_start;

    tracker.on_text_input(commit_event);
    ASSERT_EQ(tracker.text_events.size(), 1u);
    EXPECT_EQ(tracker.text_events[0].type, TextEventType::CompositionCommit);
    EXPECT_EQ(tracker.text_events[0].utf8_text, "PingDart 🎯 UI");
    EXPECT_GT(tracker.text_events[0].selection_start, 0u);
}

TEST(UnifiedInputTest, ImeCompositionLifecycleSequence) {
    MockInputTracker tracker;

    // Phase 1: Composition Start
    tracker.on_text_input(TextInputEvent{TextEventType::CompositionStart, "", 0, 0});

    // Phase 2: Pre-edit text update 1 ("ni")
    tracker.on_text_input(TextInputEvent{TextEventType::CompositionUpdate, "ni", 0, 2});

    // Phase 3: Pre-edit text update 2 ("nihon")
    tracker.on_text_input(TextInputEvent{TextEventType::CompositionUpdate, "nihon", 0, 5});

    // Phase 4: Commit candidate ("日本" in UTF-8 is 6 bytes)
    tracker.on_text_input(TextInputEvent{TextEventType::CompositionCommit, "日本", 6, 6});

    // Phase 5: Composition End
    tracker.on_text_input(TextInputEvent{TextEventType::CompositionEnd, "", 0, 0});

    ASSERT_EQ(tracker.text_events.size(), 5u);
    EXPECT_EQ(tracker.text_events[0].type, TextEventType::CompositionStart);
    EXPECT_EQ(tracker.text_events[1].type, TextEventType::CompositionUpdate);
    EXPECT_EQ(tracker.text_events[1].utf8_text, "ni");
    EXPECT_EQ(tracker.text_events[2].type, TextEventType::CompositionUpdate);
    EXPECT_EQ(tracker.text_events[2].utf8_text, "nihon");
    EXPECT_EQ(tracker.text_events[3].type, TextEventType::CompositionCommit);
    EXPECT_EQ(tracker.text_events[3].utf8_text, "日本");
    EXPECT_EQ(tracker.text_events[3].selection_end, 6u);
    EXPECT_EQ(tracker.text_events[4].type, TextEventType::CompositionEnd);
}

TEST(UnifiedInputTest, TextFocusTransitions) {
    MockInputTracker tracker;

    EXPECT_TRUE(tracker.focus_history.empty());
    tracker.on_focus_changed(true);
    tracker.on_focus_changed(false);

    ASSERT_EQ(tracker.focus_history.size(), 2u);
    EXPECT_TRUE(tracker.focus_history[0]);
    EXPECT_FALSE(tracker.focus_history[1]);
}

TEST(UnifiedInputTest, PointerMotionWheelAndButtons) {
    MockInputTracker tracker;

    PointerEvent move_ev;
    move_ev.type = EventType::PointerMove;
    move_ev.x = 250.0;
    move_ev.y = 180.0;
    tracker.dispatch_pointer_event(move_ev);

    PointerEvent wheel_ev;
    wheel_ev.type = EventType::PointerWheel;
    wheel_ev.x = 250.0;
    wheel_ev.y = 180.0;
    wheel_ev.delta_y = 1.0;
    tracker.dispatch_pointer_event(wheel_ev);

    PointerEvent down_ev;
    down_ev.type = EventType::PointerDown;
    down_ev.button = PointerButton::Left;
    down_ev.buttons_down = static_cast<uint8_t>(PointerButton::Left);
    tracker.dispatch_pointer_event(down_ev);

    ASSERT_EQ(tracker.pointer_events.size(), 3u);
    EXPECT_EQ(tracker.pointer_events[0].type, EventType::PointerMove);
    EXPECT_EQ(tracker.pointer_events[1].type, EventType::PointerWheel);
    EXPECT_EQ(tracker.pointer_events[1].delta_y, 1.0);
    EXPECT_EQ(tracker.pointer_events[2].type, EventType::PointerDown);
    EXPECT_EQ(tracker.pointer_events[2].button, PointerButton::Left);
}
