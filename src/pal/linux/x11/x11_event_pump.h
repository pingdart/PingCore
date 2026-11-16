// src/pal/linux/x11/x11_event_pump.h
#pragma once
#include <pingdart/pal/i_platform_event_pump.h>
#include <pingdart/core/status.h>
#include <atomic>
#include <chrono>

#if defined(__linux__)
#include <X11/Xlib.h>
#else
struct _XDisplay;
typedef struct _XDisplay Display;
#endif

namespace pingdart::pal::linux_x11 {

class X11EventPump final : public IPlatformEventPump {
public:
    explicit X11EventPump(Display* display);
    ~X11EventPump() override;

    X11EventPump(const X11EventPump&) = delete;
    X11EventPump& operator=(const X11EventPump&) = delete;

    core::Status initialize();

    // Drains already-queued Xlib events non-blocking
    void poll_events() override;

    // Blocking wait on X11 connection fd OR runtime eventfd wakeup
    bool wait_for_events(std::chrono::milliseconds timeout) override;

    // Signals eventfd to interrupt wait_for_events from worker/render threads
    void wake_up() override;

    [[nodiscard]] int connection_fd() const noexcept { return x11_fd_; }
    [[nodiscard]] int eventfd() const noexcept { return wakeup_eventfd_; }
    [[nodiscard]] bool has_quit_been_received() const noexcept { return quit_received_.load(); }
    void set_quit_received() noexcept { quit_received_.store(true); }

private:
    Display* display_{nullptr};
    int x11_fd_{-1};
    int epoll_fd_{-1};
    int wakeup_eventfd_{-1};
    std::atomic<bool> quit_received_{false};
};

} // namespace pingdart::pal::linux_x11
