// src/pal/windows/win32_event_pump.cpp
#include "win32_event_pump.h"
#include <algorithm>

namespace pingdart::pal::windows {

Win32EventPump::Win32EventPump() {
    // Create an auto-reset event for inter-thread wakeups
    wakeup_event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
}

Win32EventPump::~Win32EventPump() {
    if (wakeup_event_ != nullptr) {
        CloseHandle(wakeup_event_);
        wakeup_event_ = nullptr;
    }
}

void Win32EventPump::poll_events() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            quit_received_.store(true);
            return;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

bool Win32EventPump::wait_for_events(std::chrono::milliseconds timeout) {
    if (quit_received_.load()) {
        return false;
    }

    // Step 1: Drain any already-pending messages before sleeping to avoid lost wakeups
    poll_events();
    if (quit_received_.load()) {
        return false;
    }

    // Step 2: Compute wait timeout
    DWORD timeout_ms = (timeout == std::chrono::milliseconds::zero()) ? 0 :
                       (timeout.count() < 0) ? INFINITE : static_cast<DWORD>(timeout.count());

    // Step 3: Cooperative sleep on wakeup handle OR Windows message queue
    HANDLE handles[] = { wakeup_event_ };
    DWORD wait_result = MsgWaitForMultipleObjectsEx(
        1,
        handles,
        timeout_ms,
        QS_ALLINPUT,
        MWMO_INPUTAVAILABLE
    );

    if (wait_result == WAIT_OBJECT_0) {
        // Runtime task or shutdown signaled wakeup_event_
        return true;
    } else if (wait_result == WAIT_OBJECT_0 + 1) {
        // Windows message arrived; drain queue
        poll_events();
        return true;
    } else if (wait_result == WAIT_TIMEOUT) {
        return false;
    }

    // WAIT_FAILED
    return false;
}

void Win32EventPump::wake_up() {
    if (wakeup_event_ != nullptr) {
        SetEvent(wakeup_event_);
    }
}

} // namespace pingdart::pal::windows
