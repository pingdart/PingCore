// src/pal/windows/win32_event_pump.h
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <pingdart/pal/i_platform_event_pump.h>
#include <atomic>
#include <chrono>

namespace pingdart::pal::windows {

class Win32EventPump final : public IPlatformEventPump {
public:
    Win32EventPump();
    ~Win32EventPump() override;

    Win32EventPump(const Win32EventPump&) = delete;
    Win32EventPump& operator=(const Win32EventPump&) = delete;

    // Non-blocking drain of Win32 message queue
    void poll_events() override;

    // Blocking wait on Win32 messages OR runtime wakeup handle
    bool wait_for_events(std::chrono::milliseconds timeout) override;

    // Wakes up MsgWaitForMultipleObjectsEx from runtime/worker threads
    void wake_up() override;

    [[nodiscard]] HANDLE wakeup_handle() const noexcept { return wakeup_event_; }
    [[nodiscard]] bool has_quit_been_received() const noexcept { return quit_received_.load(); }

private:
    HANDLE wakeup_event_{nullptr};
    std::atomic<bool> quit_received_{false};
};

} // namespace pingdart::pal::windows
