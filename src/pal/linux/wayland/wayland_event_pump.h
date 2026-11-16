// src/pal/linux/wayland/wayland_event_pump.h
#pragma once
#include <pingdart/pal/i_platform_event_pump.h>
#include <pingdart/core/status.h>
#include <chrono>
#include <atomic>

struct wl_display;

namespace pingdart::pal::linux_wayland {

class WaylandEventPump final : public IPlatformEventPump {
public:
    explicit WaylandEventPump(wl_display* display);
    ~WaylandEventPump() override;

    WaylandEventPump(const WaylandEventPump&) = delete;
    WaylandEventPump& operator=(const WaylandEventPump&) = delete;

    core::Status initialize();

    // Drains any currently pending Wayland events
    void poll_events() override;

    // Full 5-step Wayland prepare_read/flush/poll/read_events/dispatch protocol
    bool wait_for_events(std::chrono::milliseconds timeout) override;

    // Signals eventfd to unblock epoll_wait from arbitrary threads
    void wake_up() override;

    [[nodiscard]] int display_fd() const noexcept { return display_fd_; }
    [[nodiscard]] int eventfd() const noexcept { return wakeup_eventfd_; }
    [[nodiscard]] bool has_quit_been_received() const noexcept { return quit_received_.load(); }
    void set_quit_received() noexcept { quit_received_.store(true); }

private:
    wl_display* display_{nullptr};
    int display_fd_{-1};
    int epoll_fd_{-1};
    int wakeup_eventfd_{-1};
    std::atomic<bool> quit_received_{false};
};

} // namespace pingdart::pal::linux_wayland
