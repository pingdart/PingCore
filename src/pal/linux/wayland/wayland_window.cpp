// src/pal/linux/wayland/wayland_window.cpp
#include "wayland_window.h"
#include "wayland_registry.h"
#include <chrono>

#if defined(__linux__)
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

namespace pingdart::pal::linux_wayland {

namespace {

void xdg_surface_configure_handler(void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
    auto* window = static_cast<WaylandWindow*>(data);
    xdg_surface_ack_configure(xdg_surface, serial);
    window->on_xdg_surface_configure(serial);
}

const struct xdg_surface_listener kXdgSurfaceListener = {
    xdg_surface_configure_handler,
};

void xdg_toplevel_configure_handler(
    void* data,
    struct xdg_toplevel* toplevel,
    int32_t width,
    int32_t height,
    struct wl_array* states
) {
    (void)toplevel;
    (void)states;
    auto* window = static_cast<WaylandWindow*>(data);
    window->on_xdg_toplevel_configure(width, height);
}

void xdg_toplevel_close_handler(void* data, struct xdg_toplevel* toplevel) {
    (void)toplevel;
    auto* window = static_cast<WaylandWindow*>(data);
    window->on_close_requested();
}

const struct xdg_toplevel_listener kXdgToplevelListener = {
    xdg_toplevel_configure_handler,
    xdg_toplevel_close_handler,
};

uint64_t current_timestamp_us() {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

} // namespace

core::Result<std::unique_ptr<WaylandWindow>> WaylandWindow::create(
    wl_display* display,
    const WaylandGlobals& globals,
    std::string_view title,
    const WindowSize& size
) {
    if (!display || !globals.compositor || !globals.wm_base) {
        return core::Status::FailedPrecondition("Wayland globals not initialized");
    }

    wl_surface* surface = wl_compositor_create_surface(globals.compositor);
    if (!surface) {
        return core::Status::Internal("Failed to create wl_surface");
    }

    xdg_surface* xdg_surf = xdg_wm_base_get_xdg_surface(globals.wm_base, surface);
    if (!xdg_surf) {
        wl_surface_destroy(surface);
        return core::Status::Internal("Failed to obtain xdg_surface");
    }

    xdg_toplevel* toplevel = xdg_surface_get_toplevel(xdg_surf);
    if (!toplevel) {
        xdg_surface_destroy(xdg_surf);
        wl_surface_destroy(surface);
        return core::Status::Internal("Failed to obtain xdg_toplevel");
    }

    auto window = std::unique_ptr<WaylandWindow>(new WaylandWindow(
        display, surface, xdg_surf, toplevel, size
    ));

    xdg_surface_add_listener(xdg_surf, &kXdgSurfaceListener, window.get());
    xdg_toplevel_add_listener(toplevel, &kXdgToplevelListener, window.get());

    std::string str_title(title);
    xdg_toplevel_set_title(toplevel, str_title.c_str());
    xdg_toplevel_set_app_id(toplevel, "org.pingdart.app");

    // Initiate initial configure handshake by committing empty surface
    wl_surface_commit(surface);
    wl_display_roundtrip(display);

    return window;
}

#else

namespace pingdart::pal::linux_wayland {

core::Result<std::unique_ptr<WaylandWindow>> WaylandWindow::create(
    wl_display* display,
    const WaylandGlobals& globals,
    std::string_view title,
    const WindowSize& size
) {
    (void)display;
    (void)globals;
    (void)title;
    (void)size;
    return core::Status::Unimplemented("Wayland only supported on Linux targets");
}

#endif

WaylandWindow::WaylandWindow(
    wl_display* display,
    wl_surface* surface,
    xdg_surface* xdg_surf,
    xdg_toplevel* toplevel,
    WindowSize size
) : display_(display),
    surface_(surface),
    xdg_surface_(xdg_surf),
    xdg_toplevel_(toplevel),
    size_(size),
    ui_thread_id_(std::this_thread::get_id()) {}

WaylandWindow::~WaylandWindow() {
    close();
#if defined(__linux__)
    if (xdg_toplevel_) {
        xdg_toplevel_destroy(xdg_toplevel_);
        xdg_toplevel_ = nullptr;
    }
    if (xdg_surface_) {
        xdg_surface_destroy(xdg_surface_);
        xdg_surface_ = nullptr;
    }
    if (surface_) {
        wl_surface_destroy(surface_);
        surface_ = nullptr;
    }
#endif
    lifecycle_state_ = SurfaceLifecycleState::Destroyed;
}

NativeSurfaceHandle WaylandWindow::native_surface_handle() const noexcept {
    NativeSurfaceHandle handle;
    handle.kind = SurfaceKind::Wayland;
    handle.token = WaylandSurfaceDescriptor{
        reinterpret_cast<void*>(display_),
        reinterpret_cast<void*>(surface_)
    };
    handle.surface_generation_id = surface_generation_id_;
    handle.swapchain_generation_id = swapchain_generation_id_;
    handle.generation_id = surface_generation_id_;
    handle.lifecycle_state = lifecycle_state_;
    handle.is_valid = (surface_ != nullptr) && (lifecycle_state_ != SurfaceLifecycleState::Destroyed);
    return handle;
}

uint64_t WaylandWindow::surface_generation_id() const noexcept {
    return surface_generation_id_;
}

uint64_t WaylandWindow::swapchain_generation_id() const noexcept {
    return swapchain_generation_id_;
}

WindowSize WaylandWindow::physical_size() const noexcept {
    return size_;
}

float WaylandWindow::scale_factor() const noexcept {
    return scale_factor_;
}

SurfaceLifecycleState WaylandWindow::surface_lifecycle_state() const noexcept {
    return lifecycle_state_;
}

void WaylandWindow::mark_surface_retiring() noexcept {
    lifecycle_state_ = SurfaceLifecycleState::Retiring;
}

void WaylandWindow::acknowledge_surface_retirement() noexcept {
    lifecycle_state_ = SurfaceLifecycleState::Retired;
}

bool WaylandWindow::is_surface_ready_for_destruction() const noexcept {
    return lifecycle_state_ == SurfaceLifecycleState::Retired;
}

void WaylandWindow::force_surface_retirement_timeout() noexcept {
    lifecycle_state_ = SurfaceLifecycleState::Abandoned;
}

core::Status WaylandWindow::show() {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
    visible_ = true;
    return core::Status::Ok();
}

core::Status WaylandWindow::hide() {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
    visible_ = false;
    return core::Status::Ok();
}

core::Status WaylandWindow::close() {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
    mark_surface_retiring();
    if (input_dispatcher_) {
        input_dispatcher_->dispatch_lifecycle_event(WindowLifecycleEvent{
            EventType::WindowCloseRequested,
            size_.width,
            size_.height,
            scale_factor_,
            0
        });
    }
    return core::Status::Ok();
}

core::Status WaylandWindow::set_title(std::string_view title) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
#if defined(__linux__)
    if (xdg_toplevel_) {
        std::string str_title(title);
        xdg_toplevel_set_title(xdg_toplevel_, str_title.c_str());
    }
#endif
    return core::Status::Ok();
}

core::Status WaylandWindow::set_size(const WindowSize& size) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
    size_ = size;
    swapchain_generation_id_++;
    return core::Status::Ok();
}

core::Status WaylandWindow::set_position(const WindowPoint& position) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
    position_ = position; // Wayland compositors control global window placement
    return core::Status::Ok();
}

WindowSize WaylandWindow::size() const noexcept {
    return size_;
}

WindowPoint WaylandWindow::position() const noexcept {
    return position_;
}

bool WaylandWindow::is_visible() const noexcept {
    return visible_;
}

bool WaylandWindow::is_minimized() const noexcept {
    return false;
}

bool WaylandWindow::is_maximized() const noexcept {
    return is_maximized_;
}

void WaylandWindow::set_input_dispatcher(IInputDispatcher* dispatcher) noexcept {
    input_dispatcher_ = dispatcher;
}

IInputDispatcher* WaylandWindow::input_dispatcher() const noexcept {
    return input_dispatcher_;
}

core::Status WaylandWindow::set_cursor(CursorKind cursor) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Cursor mutations must occur on UI thread");
    }
    cursor_ = cursor;
    return core::Status::Ok();
}

CursorKind WaylandWindow::cursor() const noexcept {
    return cursor_;
}

core::Status WaylandWindow::set_cursor_visible(bool visible) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Cursor mutations must occur on UI thread");
    }
    cursor_visible_ = visible;
    return core::Status::Ok();
}

void WaylandWindow::set_text_input_client(ITextInputClient* client) noexcept {
    text_client_ = client;
}

ITextInputClient* WaylandWindow::text_input_client() const noexcept {
    return text_client_;
}

void WaylandWindow::update_ime_caret_rect(const WindowPoint& position, const WindowSize& size) {
    (void)position;
    (void)size;
}

bool WaylandWindow::is_on_ui_thread() const noexcept {
    return std::this_thread::get_id() == ui_thread_id_;
}

void WaylandWindow::on_xdg_surface_configure(uint32_t serial) {
    (void)serial;
    has_initial_configure_ = true;
#if defined(__linux__)
    if (surface_) {
        wl_surface_commit(surface_);
    }
#endif
}

void WaylandWindow::on_xdg_toplevel_configure(int32_t width, int32_t height) {
    if (width > 0 && height > 0) {
        size_ = WindowSize{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        swapchain_generation_id_++;
        if (input_dispatcher_) {
            input_dispatcher_->dispatch_lifecycle_event(WindowLifecycleEvent{
                EventType::WindowResized,
                size_.width,
                size_.height,
                scale_factor_,
                0
            });
        }
    }
}

void WaylandWindow::on_close_requested() {
    close();
}

} // namespace pingdart::pal::linux_wayland
