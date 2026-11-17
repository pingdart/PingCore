// include/pingdart/pal/macos/macos_platform_host.h
// PKG-113: Native macOS Platform Host (Cocoa NSApplication Integration)
#pragma once
#include <pingdart/pal/i_platform_host.h>
#include <pingdart/pal/native_types.h>
#include <pingdart/pal/i_window.h>
#include <pingdart/pal/i_platform_event_pump.h>
#include <pingdart/core/result.h>
#include <memory>
#include <string_view>
#include <thread>

namespace pingdart::pal::macos {

class MacosEventPump;

class MacosPlatformHost final : public IPlatformHost {
public:
    MacosPlatformHost();
    ~MacosPlatformHost() override;

    MacosPlatformHost(const MacosPlatformHost&) = delete;
    MacosPlatformHost& operator=(const MacosPlatformHost&) = delete;

    // Initializes Cocoa NSApplication and configures activation policy
    [[nodiscard]] core::Status initialize() override;

    // Shuts down Cocoa host and closes all native windows
    void shutdown() override;

    [[nodiscard]] PlatformType platform_type() const noexcept override;
    [[nodiscard]] IPlatformEventPump& event_pump() override;

    [[nodiscard]] core::Result<std::unique_ptr<IWindow>> create_window(
        std::string_view title,
        const WindowSize& initial_size
    ) override;

    void request_quit() noexcept override;
    [[nodiscard]] bool is_quit_requested() const noexcept override;

    [[nodiscard]] bool is_on_ui_thread() const noexcept;
    [[nodiscard]] std::thread::id ui_thread_id() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace pingdart::pal::macos
