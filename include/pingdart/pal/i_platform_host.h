// include/pingdart/pal/i_platform_host.h
#pragma once
#include <pingdart/pal/native_types.h>
#include <pingdart/pal/i_window.h>
#include <pingdart/pal/i_platform_event_pump.h>
#include <pingdart/core/result.h>
#include <memory>
#include <string_view>

namespace pingdart::pal {

class IPlatformHost {
public:
    virtual ~IPlatformHost() = default;

    // Initializes the host adapter
    [[nodiscard]] virtual core::Status initialize() = 0;

    // Shuts down the host adapter and releases all display/window resources
    virtual void shutdown() = 0;

    // Returns the active platform type
    [[nodiscard]] virtual PlatformType platform_type() const noexcept = 0;

    // Accesses platform event pump
    [[nodiscard]] virtual IPlatformEventPump& event_pump() = 0;

    // Creates a top-level native window (Must be called on UI thread)
    [[nodiscard]] virtual core::Result<std::unique_ptr<IWindow>> create_window(
        std::string_view title,
        const WindowSize& initial_size
    ) = 0;

    // Signals host to terminate event loops
    virtual void request_quit() noexcept = 0;
    [[nodiscard]] virtual bool is_quit_requested() const noexcept = 0;
};

} // namespace pingdart::pal
