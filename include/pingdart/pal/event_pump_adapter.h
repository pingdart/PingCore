// include/pingdart/pal/event_pump_adapter.h
#pragma once
#include <pingdart/core/event_loop.h>
#include <pingdart/pal/i_platform_event_pump.h>

namespace pingdart::pal {

/// Bridges Core EventLoop IWakeupHandle to PAL IPlatformEventPump::wake_up
class EventPumpWakeupAdapter final : public core::IWakeupHandle {
public:
    explicit EventPumpWakeupAdapter(IPlatformEventPump& pump) noexcept : pump_(pump) {}

    void wake() override {
        pump_.wake_up();
    }

private:
    IPlatformEventPump& pump_;
};

} // namespace pingdart::pal
