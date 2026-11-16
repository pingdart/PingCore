// src/pal/windows/win32_platform_host.cpp
#include "win32_platform_host.h"
#include "win32_window.h"

namespace pingdart::pal::windows {

Win32PlatformHost::Win32PlatformHost()
    : hinstance_(GetModuleHandleW(nullptr)),
      ui_thread_id_(std::this_thread::get_id()) {}

Win32PlatformHost::~Win32PlatformHost() {
    shutdown();
}

core::Status Win32PlatformHost::initialize() {
    if (initialized_) {
        return core::Status::AlreadyExists("Win32 platform host already initialized");
    }

    // Set process DPI awareness context to Per-Monitor V2
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    initialized_ = true;
    return core::Status::Ok();
}

void Win32PlatformHost::shutdown() {
    if (initialized_) {
        initialized_ = false;
        quit_requested_ = true;
    }
}

PlatformType Win32PlatformHost::platform_type() const noexcept {
    return PlatformType::Windows;
}

IPlatformEventPump& Win32PlatformHost::event_pump() {
    return event_pump_;
}

core::Result<std::unique_ptr<IWindow>> Win32PlatformHost::create_window(
    std::string_view title,
    const WindowSize& initial_size
) {
    if (!initialized_) {
        return core::Status::FailedPrecondition("Win32PlatformHost is not initialized");
    }
    if (std::this_thread::get_id() != ui_thread_id_) {
        return core::Status::FailedPrecondition("create_window must run on the UI thread");
    }

    auto window_res = Win32Window::create(hinstance_, title, initial_size);
    if (!window_res.has_value()) {
        return window_res.status();
    }

    std::unique_ptr<IWindow> window = std::move(window_res).value();
    return window;
}

void Win32PlatformHost::request_quit() noexcept {
    quit_requested_ = true;
    PostQuitMessage(0);
    event_pump_.wake_up();
}

bool Win32PlatformHost::is_quit_requested() const noexcept {
    return quit_requested_ || event_pump_.has_quit_been_received();
}

} // namespace pingdart::pal::windows
