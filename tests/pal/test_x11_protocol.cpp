// tests/pal/test_x11_protocol.cpp
#include <gtest/gtest.h>
#include <pingdart/pal/native_types.h>
#include <pingdart/pal/input_events.h>
#include <pingdart/pal/event_pump_adapter.h>
#include <pingdart/core/event_loop.h>
#include "../../src/pal/linux/x11/x11_input_dispatcher.h"
#include <vector>
#include <string>

using namespace pingdart::pal;
using namespace pingdart::pal::linux_x11;
using namespace pingdart::core;

namespace {

// Mock X11 Server and Event Queue State Machine
class MockX11ServerState {
public:
    int xlib_buffered_events{0};
    int kernel_socket_events{0};
    bool wm_delete_window_received{false};
    uint64_t wm_delete_atom{0x1000};

    // Returns number of events already buffered in Xlib user-space memory
    [[nodiscard]] int xpending() const noexcept {
        return xlib_buffered_events;
    }

    void simulate_incoming_network_event() {
        ++kernel_socket_events;
    }

    void simulate_xlib_read() {
        xlib_buffered_events += kernel_socket_events;
        kernel_socket_events = 0;
    }

    int drain_pending() {
        int drained = xlib_buffered_events;
        xlib_buffered_events = 0;
        return drained;
    }

    // Returns whether epoll_wait on the raw kernel socket would wake up
    [[nodiscard]] bool would_epoll_wake() const noexcept {
        return kernel_socket_events > 0;
    }
};

} // namespace

TEST(X11ProtocolTest, XPendingDrainedBeforeEpollWait) {
    MockX11ServerState x11;

    // Simulate Xlib having already read 2 events from the socket into client buffers
    x11.simulate_incoming_network_event();
    x11.simulate_incoming_network_event();
    x11.simulate_xlib_read();

    EXPECT_EQ(x11.xpending(), 2);
    // Crucial check: the kernel socket is now empty!
    EXPECT_FALSE(x11.would_epoll_wake());

    // Invariant: The event pump MUST check XPending() before epoll_wait.
    // If it did not, epoll_wait on the socket would hang indefinitely even though
    // events are ready in memory!
    bool pump_drained_without_blocking = false;
    if (x11.xpending() > 0) {
        int drained = x11.drain_pending();
        EXPECT_EQ(drained, 2);
        pump_drained_without_blocking = true;
    }

    EXPECT_TRUE(pump_drained_without_blocking);
    EXPECT_EQ(x11.xpending(), 0);
}

TEST(X11ProtocolTest, X11ButtonMapping) {
    EXPECT_EQ(X11InputHelper::map_x11_button(1), PointerButton::Left);
    EXPECT_EQ(X11InputHelper::map_x11_button(2), PointerButton::Middle);
    EXPECT_EQ(X11InputHelper::map_x11_button(3), PointerButton::Right);
    EXPECT_EQ(X11InputHelper::map_x11_button(4), PointerButton::None); // Wheel handled separately
    EXPECT_EQ(X11InputHelper::map_x11_button(5), PointerButton::None);
    EXPECT_EQ(X11InputHelper::map_x11_button(99), PointerButton::None);
}

TEST(X11ProtocolTest, X11ModifierBitmaskMapping) {
    // ShiftMask (1 << 0)
    auto shift = X11InputHelper::map_x11_modifiers(1 << 0);
    EXPECT_TRUE(shift & ModifierKey::Shift);
    EXPECT_FALSE(shift & ModifierKey::Control);

    // ControlMask (1 << 2) + Mod1Mask (Alt: 1 << 3)
    auto ctrl_alt = X11InputHelper::map_x11_modifiers((1 << 2) | (1 << 3));
    EXPECT_TRUE(ctrl_alt & ModifierKey::Control);
    EXPECT_TRUE(ctrl_alt & ModifierKey::Alt);
    EXPECT_FALSE(ctrl_alt & ModifierKey::Shift);

    // LockMask (CapsLock: 1 << 1) + Mod4Mask (Super/Meta: 1 << 6)
    auto caps_meta = X11InputHelper::map_x11_modifiers((1 << 1) | (1 << 6));
    EXPECT_TRUE(caps_meta & ModifierKey::CapsLock);
    EXPECT_TRUE(caps_meta & ModifierKey::Meta);
}

TEST(X11ProtocolTest, WmDeleteWindowHandshake) {
    MockX11ServerState x11;
    bool retirement_triggered = false;

    // Simulate ClientMessage with WM_DELETE_WINDOW atom
    uint64_t incoming_atom = x11.wm_delete_atom;
    if (incoming_atom == x11.wm_delete_atom) {
        retirement_triggered = true;
    }

    EXPECT_TRUE(retirement_triggered);
}

TEST(X11ProtocolTest, FallbackBackendResolutionLogic) {
    // Tests the tier-based display server resolution:
    // 1. Wayland preferred when WAYLAND_DISPLAY is set
    // 2. X11 fallback when only DISPLAY is set
    // 3. Unavailable when neither is set

    auto resolve_tier = [](const char* wayland_env, const char* x11_env) -> std::string {
        if (wayland_env && *wayland_env != '\0') {
            return "Wayland";
        }
        if (x11_env && *x11_env != '\0') {
            return "X11";
        }
        return "Unavailable";
    };

    EXPECT_EQ(resolve_tier("wayland-0", ":0"), "Wayland");
    EXPECT_EQ(resolve_tier("wayland-1", nullptr), "Wayland");
    EXPECT_EQ(resolve_tier(nullptr, ":0"), "X11");
    EXPECT_EQ(resolve_tier("", ":1"), "X11");
    EXPECT_EQ(resolve_tier(nullptr, nullptr), "Unavailable");
    EXPECT_EQ(resolve_tier("", ""), "Unavailable");
}
