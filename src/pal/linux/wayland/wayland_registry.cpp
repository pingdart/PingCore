// src/pal/linux/wayland/wayland_registry.cpp
#include "wayland_registry.h"
#include <cstring>

#if defined(__linux__)
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#endif

namespace pingdart::pal::linux_wayland {

#if defined(__linux__)
namespace {

void xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) {
    (void)data;
    xdg_wm_base_pong(xdg_wm_base, serial);
}

const struct xdg_wm_base_listener kWmBaseListener = {
    xdg_wm_base_ping,
};

void registry_handle_global(
    void* data,
    struct wl_registry* registry,
    uint32_t name,
    const char* interface,
    uint32_t version
) {
    auto* self = static_cast<WaylandRegistry*>(data);
    auto& globals = const_cast<WaylandGlobals&>(self->globals());

    if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
        globals.compositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u)));
    } else if (std::strcmp(interface, wl_subcompositor_interface.name) == 0) {
        globals.subcompositor = static_cast<wl_subcompositor*>(
            wl_registry_bind(registry, name, &wl_subcompositor_interface, 1));
    } else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
        globals.shm = static_cast<wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0) {
        globals.wm_base = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
        xdg_wm_base_add_listener(globals.wm_base, &kWmBaseListener, nullptr);
    } else if (std::strcmp(interface, wl_seat_interface.name) == 0) {
        globals.seat = static_cast<wl_seat*>(
            wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 7u)));
    }
}

void registry_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}

const struct wl_registry_listener kRegistryListener = {
    registry_handle_global,
    registry_handle_global_remove,
};

} // namespace
#endif

WaylandRegistry::~WaylandRegistry() {
    unbind();
}

core::Status WaylandRegistry::bind(wl_display* display) {
#if defined(__linux__)
    if (!display) {
        return core::Status::InvalidArgument("Wayland display is null");
    }
    display_ = display;
    registry_ = wl_display_get_registry(display_);
    if (!registry_) {
        return core::Status::Internal("Failed to obtain Wayland registry");
    }

    wl_registry_add_listener(registry_, &kRegistryListener, this);

    // Initial roundtrip to discover all compositor globals
    if (wl_display_roundtrip(display_) < 0) {
        return core::Status::Internal("Failed initial Wayland registry roundtrip");
    }

    if (!is_ready()) {
        return core::Status::FailedPrecondition("Compositor does not support required Wayland interfaces (wl_compositor or xdg_wm_base missing)");
    }

    return core::Status::Ok();
#else
    (void)display;
    return core::Status::Ok();
#endif
}

void WaylandRegistry::unbind() {
#if defined(__linux__)
    if (globals_.wm_base) {
        xdg_wm_base_destroy(globals_.wm_base);
        globals_.wm_base = nullptr;
    }
    if (globals_.seat) {
        wl_seat_destroy(globals_.seat);
        globals_.seat = nullptr;
    }
    if (globals_.shm) {
        wl_shm_destroy(globals_.shm);
        globals_.shm = nullptr;
    }
    if (globals_.subcompositor) {
        wl_subcompositor_destroy(globals_.subcompositor);
        globals_.subcompositor = nullptr;
    }
    if (globals_.compositor) {
        wl_compositor_destroy(globals_.compositor);
        globals_.compositor = nullptr;
    }
    if (registry_) {
        wl_registry_destroy(registry_);
        registry_ = nullptr;
    }
#endif
}

bool WaylandRegistry::is_ready() const noexcept {
#if defined(__linux__)
    return globals_.compositor != nullptr && globals_.wm_base != nullptr;
#else
    return true;
#endif
}

} // namespace pingdart::pal::linux_wayland
