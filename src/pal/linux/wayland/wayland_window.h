// src/pal/linux/wayland/wayland_window.h
#pragma once
#include <pingdart/pal/i_window.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include <string>
#include <string_view>
#include <thread>
#include <atomic>

#if defined(__linux__)
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#else
struct wl_display;
struct wl_surface;
struct xdg_surface;
struct xdg_toplevel;
#endif

namespace pingdart::pal::linux_wayland {

struct WaylandGlobals;

class WaylandWindow final : public IWindow {
public:
    static core::Result<std::unique_ptr<WaylandWindow>> create(
        wl_display* display,
        const WaylandGlobals& globals,
        std::string_view title,
        const WindowSize& size
    );

    ~WaylandWindow() override;

    WaylandWindow(const WaylandWindow&) = delete;
    WaylandWindow& operator=(const WaylandWindow&) = delete;

    // ISurfaceProvider
    [[nodiscard]] NativeSurfaceHandle native_surface_handle() const noexcept override;
    [[nodiscard]] uint64_t surface_generation_id() const noexcept override;
    [[nodiscard]] uint64_t swapchain_generation_id() const noexcept override;
    [[nodiscard]] WindowSize physical_size() const noexcept override;
    [[nodiscard]] float scale_factor() const noexcept override;
    [[nodiscard]] SurfaceLifecycleState surface_lifecycle_state() const noexcept override;
    void mark_surface_retiring() noexcept override;
    void acknowledge_surface_retirement() noexcept override;
    [[nodiscard]] bool is_surface_ready_for_destruction() const noexcept override;
    void force_surface_retirement_timeout() noexcept override;

    // IWindow
    core::Status show() override;
    core::Status hide() override;
    core::Status close() override;

    core::Status set_title(std::string_view title) override;
    core::Status set_size(const WindowSize& size) override;
    core::Status set_position(const WindowPoint& position) override;

    [[nodiscard]] WindowSize size() const noexcept override;
    [[nodiscard]] WindowPoint position() const noexcept override;
    [[nodiscard]] bool is_visible() const noexcept override;
    [[nodiscard]] bool is_minimized() const noexcept override;
    [[nodiscard]] bool is_maximized() const noexcept override;

    void set_input_dispatcher(IInputDispatcher* dispatcher) noexcept override;
    [[nodiscard]] IInputDispatcher* input_dispatcher() const noexcept override;

    // Cursor management
    core::Status set_cursor(CursorKind cursor) override;
    [[nodiscard]] CursorKind cursor() const noexcept override;
    core::Status set_cursor_visible(bool visible) override;

    // IME & text input client
    void set_text_input_client(ITextInputClient* client) noexcept override;
    [[nodiscard]] ITextInputClient* text_input_client() const noexcept override;
    void update_ime_caret_rect(const WindowPoint& position, const WindowSize& size) override;

    [[nodiscard]] bool is_on_ui_thread() const noexcept override;

    // Wayland Protocol Callbacks
    void on_xdg_surface_configure(uint32_t serial);
    void on_xdg_toplevel_configure(int32_t width, int32_t height);
    void on_close_requested();

    [[nodiscard]] bool has_completed_initial_configure() const noexcept { return has_initial_configure_; }
    [[nodiscard]] wl_surface* wl_surface_handle() const noexcept { return surface_; }

private:
    WaylandWindow(
        wl_display* display,
        wl_surface* surface,
        xdg_surface* xdg_surf,
        xdg_toplevel* toplevel,
        WindowSize size
    );

    wl_display* display_{nullptr};
    wl_surface* surface_{nullptr};
    xdg_surface* xdg_surface_{nullptr};
    xdg_toplevel* xdg_toplevel_{nullptr};

    WindowSize size_{0, 0};
    WindowPoint position_{0, 0};
    float scale_factor_{1.0f};
    bool visible_{false};
    bool has_initial_configure_{false};
    bool is_maximized_{false};

    uint64_t surface_generation_id_{1};
    uint64_t swapchain_generation_id_{1};
    SurfaceLifecycleState lifecycle_state_{SurfaceLifecycleState::Active};
    IInputDispatcher* input_dispatcher_{nullptr};
    ITextInputClient* text_client_{nullptr};
    CursorKind cursor_{CursorKind::Arrow};
    bool cursor_visible_{true};
    std::thread::id ui_thread_id_;
};

} // namespace pingdart::pal::linux_wayland
