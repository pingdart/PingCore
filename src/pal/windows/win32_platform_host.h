// src/pal/windows/win32_platform_host.h
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <pingdart/pal/i_platform_host.h>
#include "win32_event_pump.h"
#include <memory>
#include <thread>

namespace pingdart::pal::windows {

class Win32PlatformHost final : public IPlatformHost {
public:
    Win32PlatformHost();
    ~Win32PlatformHost() override;

    Win32PlatformHost(const Win32PlatformHost&) = delete;
    Win32PlatformHost& operator=(const Win32PlatformHost&) = delete;

    core::Status initialize() override;
    void shutdown() override;

    [[nodiscard]] PlatformType platform_type() const noexcept override;
    [[nodiscard]] IPlatformEventPump& event_pump() override;

    [[nodiscard]] core::Result<std::unique_ptr<IWindow>> create_window(
        std::string_view title,
        const WindowSize& initial_size
    ) override;

    void request_quit() noexcept override;
    [[nodiscard]] bool is_quit_requested() const noexcept override;

private:
    HINSTANCE hinstance_{nullptr};
    Win32EventPump event_pump_;
    std::thread::id ui_thread_id_;
    bool initialized_{false};
    bool quit_requested_{false};
};

} // namespace pingdart::pal::windows
