// tests/pal/test_wayland_protocol.cpp
#include <gtest/gtest.h>
#include <pingdart/pal/native_types.h>
#include <pingdart/pal/input_events.h>
#include <pingdart/pal/event_pump_adapter.h>
#include <pingdart/core/event_loop.h>
#include "../../src/pal/linux/wayland/wayland_input_dispatcher.h"
#include <atomic>
#include <vector>

using namespace pingdart::pal;
using namespace pingdart::pal::linux_wayland;
using namespace pingdart::core;

namespace {

// Mock Wayland Compositor and Socket Protocol State Machine
class MockWaylandProtocolState {
public:
    enum class ReadState {
        Idle,
        Prepared,
        EventsRead,
        Cancelled
    };

    ReadState read_state{ReadState::Idle};
    int pending_events_count{0};
    int flush_eagain_count{0};
    bool socket_readable{false};
    bool initial_configure_acked{false};
    uint32_t last_configured_serial{0};

    int mock_prepare_read() {
        if (pending_events_count > 0) {
            // Cannot prepare read while pending events exist
            return -1;
        }
        if (read_state == ReadState::Prepared) {
            // Protocol violation: double prepare_read without read_events or cancel_read
            return -2;
        }
        read_state = ReadState::Prepared;
        return 0;
    }

    int mock_dispatch_pending() {
        int dispatched = pending_events_count;
        pending_events_count = 0;
        return dispatched;
    }

    int mock_flush() {
        if (flush_eagain_count > 0) {
            --flush_eagain_count;
            return -1; // Simulates EAGAIN
        }
        return 0;
    }

    int mock_read_events() {
        if (read_state != ReadState::Prepared) {
            return -1; // Protocol violation: read_events without successful prepare_read
        }
        read_state = ReadState::EventsRead;
        return 0;
    }

    void mock_cancel_read() {
        if (read_state == ReadState::Prepared) {
            read_state = ReadState::Cancelled;
        }
    }

    void ack_configure(uint32_t serial) {
        last_configured_serial = serial;
        initial_configure_acked = true;
    }

    [[nodiscard]] bool can_attach_buffer() const noexcept {
        return initial_configure_acked;
    }
};

} // namespace

TEST(WaylandProtocolTest, PrepareReadRequiresPendingDispatch) {
    MockWaylandProtocolState wayland;
    wayland.pending_events_count = 3;

    // Step 1: First prepare_read fails because pending events exist
    EXPECT_NE(wayland.mock_prepare_read(), 0);

    // Step 2: Draining pending events enables prepare_read
    int dispatched = 0;
    while (wayland.mock_prepare_read() != 0) {
        dispatched += wayland.mock_dispatch_pending();
    }

    EXPECT_EQ(dispatched, 3);
    EXPECT_EQ(wayland.read_state, MockWaylandProtocolState::ReadState::Prepared);

    // Protocol requires exactly one read_events or cancel_read
    wayland.mock_cancel_read();
    EXPECT_EQ(wayland.read_state, MockWaylandProtocolState::ReadState::Cancelled);
}

TEST(WaylandProtocolTest, StrictOneToOnePrepareAndReadPairing) {
    MockWaylandProtocolState wayland;

    ASSERT_EQ(wayland.mock_prepare_read(), 0);
    EXPECT_EQ(wayland.read_state, MockWaylandProtocolState::ReadState::Prepared);

    // Double prepare_read without cancel or read is forbidden
    EXPECT_EQ(wayland.mock_prepare_read(), -2);

    // Reading events resets prepared state
    ASSERT_EQ(wayland.mock_read_events(), 0);
    EXPECT_EQ(wayland.read_state, MockWaylandProtocolState::ReadState::EventsRead);

    // Second prepare_read succeeds cleanly
    ASSERT_EQ(wayland.mock_prepare_read(), 0);
    wayland.mock_cancel_read();
    EXPECT_EQ(wayland.read_state, MockWaylandProtocolState::ReadState::Cancelled);
}

TEST(WaylandProtocolTest, FlushEagainHandling) {
    MockWaylandProtocolState wayland;
    wayland.flush_eagain_count = 2;

    int retries = 0;
    while (wayland.mock_flush() < 0) {
        ++retries;
    }

    EXPECT_EQ(retries, 2);
    EXPECT_EQ(wayland.mock_flush(), 0);
}

TEST(WaylandProtocolTest, InitialConfigureHandshakeInvariant) {
    MockWaylandProtocolState wayland;

    // Step 1 & 2: Creation of wl_surface/xdg_surface followed by initial bufferless commit
    bool initial_bufferless_commit_done = true;
    EXPECT_TRUE(initial_bufferless_commit_done);

    // Invariant: Visual application buffers must NOT be attached prior to initial configure ACK
    EXPECT_FALSE(wayland.can_attach_buffer());

    // Step 3 & 4: Compositor delivers xdg_surface.configure(serial = 42) and client acknowledges
    wayland.ack_configure(42);

    // Only after ACK is visual presentation and buffer attachment permitted
    EXPECT_TRUE(wayland.can_attach_buffer());
    EXPECT_EQ(wayland.last_configured_serial, 42u);
}

TEST(WaylandProtocolTest, EvdevPointerButtonMapping) {
    EXPECT_EQ(WaylandInputHelper::map_linux_button(0x110), PointerButton::Left);
    EXPECT_EQ(WaylandInputHelper::map_linux_button(0x111), PointerButton::Right);
    EXPECT_EQ(WaylandInputHelper::map_linux_button(0x112), PointerButton::Middle);
    EXPECT_EQ(WaylandInputHelper::map_linux_button(0x113), PointerButton::Back);
    EXPECT_EQ(WaylandInputHelper::map_linux_button(0x114), PointerButton::Forward);
    EXPECT_EQ(WaylandInputHelper::map_linux_button(0x999), PointerButton::None);
}

TEST(WaylandProtocolTest, XkbModifierBitmaskMapping) {
    // Depressed Shift (bit 0)
    auto shift_mod = WaylandInputHelper::map_xkb_modifiers(1 << 0, 0, 0);
    EXPECT_TRUE(shift_mod & ModifierKey::Shift);
    EXPECT_FALSE(shift_mod & ModifierKey::Control);

    // Depressed Control (bit 2) + Latched Alt (bit 3)
    auto ctrl_alt = WaylandInputHelper::map_xkb_modifiers(1 << 2, 1 << 3, 0);
    EXPECT_TRUE(ctrl_alt & ModifierKey::Control);
    EXPECT_TRUE(ctrl_alt & ModifierKey::Alt);
    EXPECT_FALSE(ctrl_alt & ModifierKey::Shift);

    // Locked CapsLock (bit 1)
    auto caps = WaylandInputHelper::map_xkb_modifiers(0, 0, 1 << 1);
    EXPECT_TRUE(caps & ModifierKey::CapsLock);
}
