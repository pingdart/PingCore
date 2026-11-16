// include/pingdart/pal/i_platform_event_pump.h
#pragma once
#include <chrono>

namespace pingdart::pal {

class IPlatformEventPump {
public:
    virtual ~IPlatformEventPump() = default;

    // Non-blocking drain of OS queue
    virtual void poll_events() = 0;

    // Waits until OS events arrive or specified timeout expires
    virtual bool wait_for_events(std::chrono::milliseconds timeout) = 0;

    // Wakes up event pump from arbitrary worker/render threads
    virtual void wake_up() = 0;
};

} // namespace pingdart::pal
