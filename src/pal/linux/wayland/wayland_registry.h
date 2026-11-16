// src/pal/linux/wayland/wayland_registry.h
#pragma once
#include <pingdart/pal/native_types.h>
#include <pingdart/core/status.h>
#include <cstdint>
#include <string>

#if defined(__linux__)
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#else
// Declarations for non-Linux compilation/mocking
struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_subcompositor;
struct wl_shm;
struct wl_seat;
struct xdg_wm_base;
#endif

namespace pingdart::pal::linux_wayland {

struct WaylandGlobals {
    wl_compositor* compositor{nullptr};
    wl_subcompositor* subcompositor{nullptr};
    wl_shm* shm{nullptr};
    xdg_wm_base* wm_base{nullptr};
    wl_seat* seat{nullptr};
};

class WaylandRegistry {
public:
    WaylandRegistry() = default;
    ~WaylandRegistry();

    WaylandRegistry(const WaylandRegistry&) = delete;
    WaylandRegistry& operator=(const WaylandRegistry&) = delete;

    core::Status bind(wl_display* display);
    void unbind();

    [[nodiscard]] const WaylandGlobals& globals() const noexcept { return globals_; }
    [[nodiscard]] bool is_ready() const noexcept;

private:
    wl_display* display_{nullptr};
    wl_registry* registry_{nullptr};
    WaylandGlobals globals_;
};

} // namespace pingdart::pal::linux_wayland
