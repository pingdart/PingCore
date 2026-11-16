// src/pal/linux/x11/x11_platform_host.cpp
#include "x11_platform_host.h"
#include "x11_window.h"

#if defined(__linux__)
#include <X11/Xlib.h>
#endif

namespace pingdart::pal::linux_x11 {

X11PlatformHost::X11PlatformHost()
    : ui_thread_id_(std::this_thread::get_id()) {}

X11PlatformHost::~X11PlatformHost() {
    shutdown();
}

core::Status X11PlatformHost::initialize() {
    if (initialized_) {
        return core::Status::AlreadyExists("X11PlatformHost already initialized");
    }

#if defined(__linux__)
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        return core::Status::Unavailable("Could not connect to X11 display (DISPLAY environment variable not set or X server inactive)");
    }

    event_pump_ = std::make_unique<X11EventPump>(display_);
    auto status = event_pump_->initialize();
    if (!status.ok()) {
        XCloseDisplay(display_);
        display_ = nullptr;
        return status;
    }

    initialized_ = true;
    return core::Status::Ok();
#else
    return core::Status::Unimplemented("X11 platform host only supported on Linux");
#endif
}

void X11PlatformHost::shutdown() {
    if (initialized_) {
        initialized_ = false;
        quit_requested_ = true;
#if defined(__linux__)
        event_pump_.reset();
        if (display_) {
            XCloseDisplay(display_);
            display_ = nullptr;
        }
#endif
    }
}

PlatformType X11PlatformHost::platform_type() const noexcept {
    return PlatformType::LinuxX11;
}

IPlatformEventPump& X11PlatformHost::event_pump() {
    return *event_pump_;
}

core::Result<std::unique_ptr<IWindow>> X11PlatformHost::create_window(
    std::string_view title,
    const WindowSize& initial_size
) {
    if (!initialized_) {
        return core::Status::FailedPrecondition("X11PlatformHost is not initialized");
    }
    if (std::this_thread::get_id() != ui_thread_id_) {
        return core::Status::FailedPrecondition("create_window must run on the UI thread");
    }

    auto window_res = X11Window::create(display_, title, initial_size);
    if (!window_res.has_value()) {
        return window_res.status();
    }

    std::unique_ptr<IWindow> window = std::move(window_res).value();
    return window;
}

void X11PlatformHost::request_quit() noexcept {
    quit_requested_ = true;
    if (event_pump_) {
        event_pump_->set_quit_received();
        event_pump_->wake_up();
    }
}

bool X11PlatformHost::is_quit_requested() const noexcept {
    return quit_requested_ || (event_pump_ && event_pump_->has_quit_been_received());
}

} // namespace pingdart::pal::linux_x11
