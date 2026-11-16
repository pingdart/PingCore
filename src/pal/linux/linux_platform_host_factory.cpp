// src/pal/linux/linux_platform_host_factory.cpp
#include "linux_platform_host_factory.h"
#include "wayland/wayland_platform_host.h"
#include "x11/x11_platform_host.h"
#include <cstdlib>

namespace pingdart::pal::linux_pal {

core::Result<std::unique_ptr<IPlatformHost>> create_linux_platform_host() {
    const char* wayland_display = std::getenv("WAYLAND_DISPLAY");
    const char* x11_display = std::getenv("DISPLAY");

    // Tier 1: Prefer Wayland when WAYLAND_DISPLAY is present or as primary candidate
    if (wayland_display != nullptr && *wayland_display != '\0') {
        auto wayland_host = std::make_unique<linux_wayland::WaylandPlatformHost>();
        auto status = wayland_host->initialize();
        if (status.ok()) {
            return std::unique_ptr<IPlatformHost>(std::move(wayland_host));
        }
    }

    // Tier 2: Fall back to X11 when DISPLAY is present
    if (x11_display != nullptr && *x11_display != '\0') {
        auto x11_host = std::make_unique<linux_x11::X11PlatformHost>();
        auto status = x11_host->initialize();
        if (status.ok()) {
            return std::unique_ptr<IPlatformHost>(std::move(x11_host));
        }
    }

    return core::Status::Unavailable(
        "Failed to initialize Linux display backend: neither Wayland (WAYLAND_DISPLAY) nor X11 (DISPLAY) available."
    );
}

} // namespace pingdart::pal::linux_pal
