// src/pal/linux/x11/x11_event_pump.cpp
#include "x11_event_pump.h"

#if defined(__linux__)
#include <X11/Xlib.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace pingdart::pal::linux_x11 {

X11EventPump::X11EventPump(Display* display)
    : display_(display) {}

X11EventPump::~X11EventPump() {
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

core::Status X11EventPump::initialize() {
#if defined(__linux__)
    if (!display_) {
        return core::Status::InvalidArgument("X11 Display pointer is null");
    }

    x11_fd_ = ConnectionNumber(display_);
    if (x11_fd_ < 0) {
        return core::Status::Internal("Failed to retrieve X11 ConnectionNumber fd");
    }

    wakeup_eventfd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeup_eventfd_ < 0) {
        return core::Status::Internal("Failed to create eventfd for X11 event pump");
    }

    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ < 0) {
        close(wakeup_eventfd_);
        wakeup_eventfd_ = -1;
        return core::Status::Internal("Failed to create epoll instance for X11");
    }

    // Monitor X11 connection socket for readability
    epoll_event ev_x11{};
    ev_x11.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    ev_x11.data.fd = x11_fd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, x11_fd_, &ev_x11) < 0) {
        return core::Status::Internal("Failed to register X11 fd with epoll");
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

void X11EventPump::poll_events() {
#if defined(__linux__)
    if (!display_) return;

    while (XPending(display_) > 0) {
        XEvent event;
        XNextEvent(display_, &event);
        // Note: Individual X11Window instances register their Window IDs and dispatch here
    }
#endif
}

bool X11EventPump::wait_for_events(std::chrono::milliseconds timeout) {
    if (quit_received_.load()) {
        return false;
    }

#if defined(__linux__)
    if (!display_ || epoll_fd_ < 0) {
        return false;
    }

    // CRITICAL: Drain already-buffered Xlib client-side events before epoll_wait.
    // If events are buffered in user-space Xlib queues, epoll_wait on the kernel
    // socket will NOT wake up and would cause indefinite hangs!
    if (XPending(display_) > 0) {
        poll_events();
        return true;
    }

    // Flush any pending outgoing requests
    XFlush(display_);

    int timeout_ms = (timeout.count() < 0) ? -1 : static_cast<int>(timeout.count());
    epoll_event events[4];
    int nfds = epoll_wait(epoll_fd_, events, 4, timeout_ms);

    if (nfds < 0) {
        return (errno == EINTR);
    }

    bool x11_readable = false;
    for (int i = 0; i < nfds; ++i) {
        if (events[i].data.fd == x11_fd_) {
            x11_readable = true;
        } else if (events[i].data.fd == wakeup_eventfd_) {
            uint64_t val = 0;
            read(wakeup_eventfd_, &val, sizeof(val));
        }
    }

    if (x11_readable || XPending(display_) > 0) {
        poll_events();
        return true;
    }

    return (nfds > 0);
#else
    return false;
#endif
}

void X11EventPump::wake_up() {
#if defined(__linux__)
    if (wakeup_eventfd_ >= 0) {
        uint64_t val = 1;
        write(wakeup_eventfd_, &val, sizeof(val));
    }
#endif
}

} // namespace pingdart::pal::linux_x11
