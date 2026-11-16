// src/pal/linux/wayland/wayland_platform_host.cpp
#include "wayland_platform_host.h"
#include "wayland_window.h"

#if defined(__linux__)
#include <wayland-client.h>
#endif

namespace pingdart::pal::linux_wayland {

WaylandPlatformHost::WaylandPlatformHost()
    : ui_thread_id_(std::this_thread::get_id()) {}

WaylandPlatformHost::~WaylandPlatformHost() {
    shutdown();
}

core::Status WaylandPlatformHost::initialize() {
    if (initialized_) {
        return core::Status::AlreadyExists("WaylandPlatformHost already initialized");
    }

#if defined(__linux__)
    display_ = wl_display_connect(nullptr);
    if (!display_) {
        return core::Status::Unavailable("Could not connect to Wayland display socket (WAYLAND_DISPLAY not set or compositor inactive)");
    }

    auto status = registry_.bind(display_);
    if (!status.ok()) {
        wl_display_disconnect(display_);
        display_ = nullptr;
        return status;
    }

    event_pump_ = std::make_unique<WaylandEventPump>(display_);
    status = event_pump_->initialize();
    if (!status.ok()) {
        registry_.unbind();
        wl_display_disconnect(display_);
        display_ = nullptr;
        return status;
    }

    initialized_ = true;
    return core::Status::Ok();
#else
    return core::Status::Unimplemented("Wayland platform host only supported on Linux");
#endif
}

void WaylandPlatformHost::shutdown() {
    if (initialized_) {
        initialized_ = false;
        quit_requested_ = true;
#if defined(__linux__)
        event_pump_.reset();
        registry_.unbind();
        if (display_) {
            wl_display_disconnect(display_);
            display_ = nullptr;
        }
#endif
    }
}

PlatformType WaylandPlatformHost::platform_type() const noexcept {
    return PlatformType::LinuxWayland;
}

IPlatformEventPump& WaylandPlatformHost::event_pump() {
    return *event_pump_;
}

core::Result<std::unique_ptr<IWindow>> WaylandPlatformHost::create_window(
    std::string_view title,
    const WindowSize& initial_size
) {
    if (!initialized_) {
        return core::Status::FailedPrecondition("WaylandPlatformHost is not initialized");
    }
    if (std::this_thread::get_id() != ui_thread_id_) {
        return core::Status::FailedPrecondition("create_window must run on the UI thread");
    }

    auto window_res = WaylandWindow::create(display_, registry_.globals(), title, initial_size);
    if (!window_res.has_value()) {
        return window_res.status();
    }

    std::unique_ptr<IWindow> window = std::move(window_res).value();
    return window;
}

void WaylandPlatformHost::request_quit() noexcept {
    quit_requested_ = true;
    if (event_pump_) {
        event_pump_->set_quit_received();
        event_pump_->wake_up();
    }
}

bool WaylandPlatformHost::is_quit_requested() const noexcept {
    return quit_requested_ || (event_pump_ && event_pump_->has_quit_been_received());
}

} // namespace pingdart::pal::linux_wayland
