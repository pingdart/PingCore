// tests/pal/test_macos_platform.mm
// PKG-113: Native macOS Platform Adapter Unit Tests
#include <gtest/gtest.h>
#include <pingdart/pal/macos/macos_platform_host.h>
#include <pingdart/pal/macos/macos_window.h>
#include <pingdart/pal/macos/macos_event_pump.h>
#include <pingdart/pal/macos/macos_input_dispatcher.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include <pingdart/pal/input_events.h>
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <QuartzCore/CAMetalLayer.h>
#include <thread>
#include <vector>
#include <atomic>

using namespace pingdart::pal;
using namespace pingdart::pal::macos;

namespace {

class MockMacosDispatcher final : public IInputDispatcher {
public:
    std::vector<PointerEvent> pointer_events;
    std::vector<KeyboardEvent> keyboard_events;
    std::vector<WindowLifecycleEvent> lifecycle_events;
    std::vector<TextInputEvent> text_events;

    void dispatch_pointer_event(const PointerEvent& event) override {
        pointer_events.push_back(event);
    }
    void dispatch_keyboard_event(const KeyboardEvent& event) override {
        keyboard_events.push_back(event);
    }
    void dispatch_lifecycle_event(const WindowLifecycleEvent& event) override {
        lifecycle_events.push_back(event);
    }
    void dispatch_text_event(const TextInputEvent& event) override {
        text_events.push_back(event);
    }
};

class MockTextInputReceiver final : public ITextInputClient {
public:
    std::vector<TextInputEvent> received_events;
    bool is_focused{false};

    void on_text_input(const TextInputEvent& event) override {
        received_events.push_back(event);
    }
    void on_focus_changed(bool focused) override {
        is_focused = focused;
    }
};

} // namespace

// ============================================================================
// 1. Cocoa Host Lifecycle & Main Thread Enforcement
// ============================================================================

TEST(MacosPlatformGoogleTest, HostLifecycleAndMainThreadEnforcement) {
    MacosPlatformHost host;
    auto init_status = host.initialize();
    EXPECT_TRUE(init_status.ok());
    EXPECT_EQ(host.platform_type(), PlatformType::MacOS);
    EXPECT_TRUE(host.is_on_ui_thread());

    // Creating window on main thread succeeds
    auto win_res = host.create_window("Test Main Thread Window", WindowSize{800, 600});
    ASSERT_TRUE(win_res.has_value());
    EXPECT_NE(win_res.value(), nullptr);

    // Threading violation: Calling create_window from background thread must fail
    std::atomic<bool> worker_executed{false};
    std::thread background_worker([&host, &worker_executed]() {
        auto bg_win = host.create_window("Illegal Background Window", WindowSize{800, 600});
        EXPECT_FALSE(bg_win.has_value());
        EXPECT_EQ(bg_win.status().code(), pingdart::core::StatusCode::FailedPrecondition);
        worker_executed = true;
    });
    background_worker.join();
    EXPECT_TRUE(worker_executed.load());

    // Window closure and host quit request
    host.request_quit();
    EXPECT_TRUE(host.is_quit_requested());
    host.shutdown();
}

// ============================================================================
// 2. Cocoa NSWindow, CAMetalLayer & Surface Lifecycle
// ============================================================================

TEST(MacosPlatformGoogleTest, WindowMetalSurfaceAndCooperativeRetirement) {
    MacosPlatformHost host;
    ASSERT_TRUE(host.initialize().ok());

    auto win_res = host.create_window("Metal Surface Test", WindowSize{1024, 768});
    ASSERT_TRUE(win_res.has_value());
    auto window = std::move(win_res).value();

    // Verify NativeSurfaceHandle metadata
    auto surface = window->native_surface_handle();
    EXPECT_EQ(surface.kind, SurfaceKind::Metal);
    EXPECT_TRUE(surface.is_valid);
    EXPECT_EQ(surface.lifecycle_state, SurfaceLifecycleState::Active);

    // Verify Metal Layer backing
    auto metal_desc = std::get<MetalSurfaceDescriptor>(surface.token);
    EXPECT_NE(metal_desc.layer, nullptr);
    CAMetalLayer* layer = (__bridge CAMetalLayer*)metal_desc.layer;
    EXPECT_TRUE([layer isKindOfClass:[CAMetalLayer class]]);
    EXPECT_TRUE(layer.framebufferOnly);

    // Geometry and Presentation
    EXPECT_EQ(window->size().width, 1024u);
    EXPECT_EQ(window->size().height, 768u);
    EXPECT_GT(window->scale_factor(), 0.0f);

    EXPECT_TRUE(window->show().ok());
    EXPECT_TRUE(window->set_title("Updated Title").ok());
    EXPECT_TRUE(window->set_size(WindowSize{1280, 720}).ok());
    EXPECT_EQ(window->size().width, 1280u);
    EXPECT_EQ(window->size().height, 720u);

    // Cursor Controls
    EXPECT_TRUE(window->set_cursor(CursorKind::IBeam).ok());
    EXPECT_EQ(window->cursor(), CursorKind::IBeam);
    EXPECT_TRUE(window->set_cursor_visible(false).ok());
    EXPECT_TRUE(window->set_cursor_visible(true).ok());

    // Surface Retirement 4-Step Protocol
    EXPECT_FALSE(window->is_surface_ready_for_destruction());
    window->mark_surface_retiring();
    EXPECT_EQ(window->surface_lifecycle_state(), SurfaceLifecycleState::Retiring);
    EXPECT_FALSE(window->is_surface_ready_for_destruction());

    window->acknowledge_surface_retirement();
    EXPECT_EQ(window->surface_lifecycle_state(), SurfaceLifecycleState::Retired);
    EXPECT_TRUE(window->is_surface_ready_for_destruction());

    EXPECT_TRUE(window->close().ok());
}

// ============================================================================
// 3. Event Pump & Cross-Thread Wake-Up
// ============================================================================

TEST(MacosPlatformGoogleTest, EventPumpPollAndThreadWakeUp) {
    MacosPlatformHost host;
    ASSERT_TRUE(host.initialize().ok());

    auto& pump = host.event_pump();
    // Non-blocking poll returns immediately
    pump.poll_events();

    // Cross-thread wakeup interrupts timed wait
    auto start = std::chrono::steady_clock::now();
    std::thread background_poker([&pump]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        pump.wake_up();
    });

    pump.wait_for_events(std::chrono::milliseconds(500));
    auto elapsed = std::chrono::steady_clock::now() - start;
    background_poker.join();

    // Should have woken up well before the 500ms timeout
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 400);
}

// ============================================================================
// 4. Input Translation (Modifiers, Virtual Keys, Mouse Buttons)
// ============================================================================

TEST(MacosPlatformGoogleTest, InputTranslationMappings) {
    // Modifier mappings
    auto mods = MacosInputTranslator::translate_modifiers(
        NSEventModifierFlagShift | NSEventModifierFlagCommand | NSEventModifierFlagOption
    );
    EXPECT_TRUE(mods & ModifierKey::Shift);
    EXPECT_TRUE(mods & ModifierKey::Meta);
    EXPECT_TRUE(mods & ModifierKey::Alt);
    EXPECT_FALSE(mods & ModifierKey::Control);

    // Key mappings
    EXPECT_EQ(MacosInputTranslator::translate_virtual_key(kVK_ANSI_A), 'A');
    EXPECT_EQ(MacosInputTranslator::translate_virtual_key(kVK_Return), 13u);
    EXPECT_EQ(MacosInputTranslator::translate_virtual_key(kVK_Space), 32u);
    EXPECT_EQ(MacosInputTranslator::translate_virtual_key(kVK_Escape), 27u);

    // Mouse button mappings
    EXPECT_EQ(MacosInputTranslator::translate_button(0), PointerButton::Left);
    EXPECT_EQ(MacosInputTranslator::translate_button(1), PointerButton::Right);
    EXPECT_EQ(MacosInputTranslator::translate_button(2), PointerButton::Middle);
}

// ============================================================================
// 5. NSTextInputClient & Unicode Composition / Dead Keys
// ============================================================================

TEST(MacosPlatformGoogleTest, NSTextInputClientUnicodeAndIME) {
    auto win_res = MacosWindow::create("IME Test Window", WindowSize{640, 480});
    ASSERT_TRUE(win_res.has_value());
    auto window = std::move(win_res).value();

    MockMacosDispatcher dispatcher;
    MockTextInputReceiver text_client;
    window->set_input_dispatcher(&dispatcher);
    window->set_text_input_client(&text_client);

    NSView* view = (__bridge NSView*)window->native_nsview();
    ASSERT_TRUE([view conformsToProtocol:@protocol(NSTextInputClient)]);
    id<NSTextInputClient> text_view = (id<NSTextInputClient>)view;

    // Simulate multi-character Unicode text insertion (e.g. Japanese / Telugu / Emoji)
    NSString* unicode_sample = @"నమస్కారం PingCore 🚀";
    [text_view insertText:unicode_sample replacementRange:NSMakeRange(NSNotFound, 0)];

    ASSERT_FALSE(text_client.received_events.empty());
    const auto& last_event = text_client.received_events.back();
    EXPECT_EQ(last_event.type, TextEventType::CompositionCommit);
    EXPECT_EQ(last_event.utf8_text, [unicode_sample UTF8String]);

    // Simulate dead key / accented composition (e.g. Option+E then 'e' for 'é')
    [text_view setMarkedText:@"´" selectedRange:NSMakeRange(1, 0) replacementRange:NSMakeRange(NSNotFound, 0)];
    EXPECT_TRUE([text_view hasMarkedText]);
    EXPECT_EQ([text_view markedRange].length, 1u);

    // Complete composition
    [text_view insertText:@"é" replacementRange:NSMakeRange(NSNotFound, 0)];
    EXPECT_FALSE([text_view hasMarkedText]);
    EXPECT_EQ(text_client.received_events.back().utf8_text, "é");

    EXPECT_TRUE(window->close().ok());
}
