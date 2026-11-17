// include/pingdart/pal/macos/macos_event_pump.h
// PKG-113: Native macOS Cocoa Event Pump (NSApplication Event Loop Integration)
#pragma once
#include <pingdart/pal/i_platform_event_pump.h>
#include <chrono>
#include <memory>

namespace pingdart::pal::macos {

class MacosEventPump final : public IPlatformEventPump {
public:
    MacosEventPump();
    ~MacosEventPump() override;

    MacosEventPump(const MacosEventPump&) = delete;
    MacosEventPump& operator=(const MacosEventPump&) = delete;

    // Drains pending Cocoa NSEvent queue non-blockingly
    void poll_events() override;

    // Waits cooperatively for NSEvents up to timeout or until cross-thread wake_up()
    bool wait_for_events(std::chrono::milliseconds timeout) override;

    // Posts an asynchronous wake-up NSEvent from background threads to wake the main loop
    void wake_up() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace pingdart::pal::macos
