// src/pal/linux/wayland/wayland_platform_host.h
#pragma once
#include <pingdart/pal/i_platform_host.h>
#include "wayland_registry.h"
#include "wayland_event_pump.h"
#include <memory>
#include <thread>

struct wl_display;

namespace pingdart::pal::linux_wayland {

class WaylandPlatformHost final : public IPlatformHost {
public:
    WaylandPlatformHost();
    ~WaylandPlatformHost() override;

    WaylandPlatformHost(const WaylandPlatformHost&) = delete;
    WaylandPlatformHost& operator=(const WaylandPlatformHost&) = delete;

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
    wl_display* display_{nullptr};
    WaylandRegistry registry_;
    std::unique_ptr<WaylandEventPump> event_pump_;
    std::thread::id ui_thread_id_;
    bool initialized_{false};
    bool quit_requested_{false};
};

} // namespace pingdart::pal::linux_wayland
