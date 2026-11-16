// src/pal/linux/wayland/wayland_event_pump.cpp
#include "wayland_event_pump.h"

#if defined(__linux__)
#include <wayland-client.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace pingdart::pal::linux_wayland {

WaylandEventPump::WaylandEventPump(wl_display* display)
    : display_(display) {}

WaylandEventPump::~WaylandEventPump() {
#if defined(__linux__)
    if (wakeup_eventfd_ >= 0) {
        close(wakeup_eventfd_);
        wakeup_eventfd_ = -1;
    }
    if (epoll_fd_ >= 0) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }
#endif
}

core::Status WaylandEventPump::initialize() {
#if defined(__linux__)
    if (!display_) {
        return core::Status::InvalidArgument("Wayland display pointer is null");
    }

    display_fd_ = wl_display_get_fd(display_);
    if (display_fd_ < 0) {
        return core::Status::Internal("Failed to retrieve Wayland display fd");
    }

    wakeup_eventfd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeup_eventfd_ < 0) {
        return core::Status::Internal("Failed to create eventfd for Wayland event pump");
    }

    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ < 0) {
        close(wakeup_eventfd_);
        wakeup_eventfd_ = -1;
        return core::Status::Internal("Failed to create epoll instance");
    }

    // Monitor Wayland socket for readability
    epoll_event ev_display{};
    ev_display.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    ev_display.data.fd = display_fd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, display_fd_, &ev_display) < 0) {
        return core::Status::Internal("Failed to register Wayland display fd with epoll");
    }

    // Monitor wakeup eventfd for readability
    epoll_event ev_wakeup{};
    ev_wakeup.events = EPOLLIN;
    ev_wakeup.data.fd = wakeup_eventfd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wakeup_eventfd_, &ev_wakeup) < 0) {
        return core::Status::Internal("Failed to register wakeup eventfd with epoll");
    }

    return core::Status::Ok();
#else
    return core::Status::Ok();
#endif
}

void WaylandEventPump::poll_events() {
#if defined(__linux__)
    if (display_) {
        wl_display_dispatch_pending(display_);
    }
#endif
}

bool WaylandEventPump::wait_for_events(std::chrono::milliseconds timeout) {
    if (quit_received_.load()) {
        return false;
    }

#if defined(__linux__)
    if (!display_ || epoll_fd_ < 0) {
        return false;
    }

    // Step 1: Dispatch pending events until prepare_read succeeds
    while (wl_display_prepare_read(display_) != 0) {
        if (wl_display_dispatch_pending(display_) < 0) {
            return false;
        }
    }

    // Step 2: Flush outgoing Wayland requests (handle EAGAIN via POLLOUT)
    while (wl_display_flush(display_) < 0) {
        if (errno == EAGAIN) {
            pollfd pfd{};
            pfd.fd = display_fd_;
            pfd.events = POLLOUT;
            poll(&pfd, 1, -1);
        } else if (errno == EPIPE || errno == ECONNRESET) {
            wl_display_cancel_read(display_);
            quit_received_.store(true);
            return false;
        } else {
            wl_display_cancel_read(display_);
            return false;
        }
    }

    // Step 3: Wait via epoll for Wayland socket, runtime eventfd, or deadline
    int timeout_ms = (timeout.count() < 0) ? -1 : static_cast<int>(timeout.count());
    epoll_event events[4];
    int nfds = epoll_wait(epoll_fd_, events, 4, timeout_ms);

    if (nfds < 0) {
        // Interrupted by signal or error
        wl_display_cancel_read(display_);
        return (errno == EINTR);
    }

    bool display_readable = false;
    for (int i = 0; i < nfds; ++i) {
        if (events[i].data.fd == display_fd_) {
            display_readable = true;
        } else if (events[i].data.fd == wakeup_eventfd_) {
            // Drain wakeup eventfd counter
            uint64_t val = 0;
            read(wakeup_eventfd_, &val, sizeof(val));
        }
    }

    // Step 4: Pair prepare_read with exactly one read_events or cancel_read
    if (display_readable) {
        if (wl_display_read_events(display_) < 0) {
            quit_received_.store(true);
            return false;
        }
    } else {
        wl_display_cancel_read(display_);
    }

    // Step 5: Dispatch newly read pending Wayland events
    wl_display_dispatch_pending(display_);
    return true;
#else
    return false;
#endif
}

void WaylandEventPump::wake_up() {
#if defined(__linux__)
    if (wakeup_eventfd_ >= 0) {
        uint64_t val = 1;
        write(wakeup_eventfd_, &val, sizeof(val));
    }
#endif
}

} // namespace pingdart::pal::linux_wayland
