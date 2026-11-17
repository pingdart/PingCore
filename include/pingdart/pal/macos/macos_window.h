// include/pingdart/pal/macos/macos_window.h
// PKG-113: Native macOS Cocoa Window & CAMetalLayer Surface Integration
#pragma once
#include <pingdart/pal/i_window.h>
#include <pingdart/pal/native_types.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include <pingdart/pal/input_events.h>
#include <pingdart/core/result.h>
#include <memory>
#include <string>
#include <string_view>

namespace pingdart::pal::macos {

class MacosWindow final : public IWindow {
public:
    static core::Result<std::unique_ptr<MacosWindow>> create(
        std::string_view title,
        const WindowSize& initial_size
    );

    ~MacosWindow() override;

    MacosWindow(const MacosWindow&) = delete;
    MacosWindow& operator=(const MacosWindow&) = delete;

    // ISurfaceProvider interface
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

    // IWindow Lifecycle interface
    core::Status show() override;
    core::Status hide() override;
    core::Status close() override;

    // Geometry and presentation
    core::Status set_title(std::string_view title) override;
    core::Status set_size(const WindowSize& size) override;
    core::Status set_position(const WindowPoint& position) override;
    [[nodiscard]] WindowSize size() const noexcept override;
    [[nodiscard]] WindowPoint position() const noexcept override;
    [[nodiscard]] bool is_visible() const noexcept override;
    [[nodiscard]] bool is_minimized() const noexcept override;
    [[nodiscard]] bool is_maximized() const noexcept override;

    // Input event routing
    void set_input_dispatcher(IInputDispatcher* dispatcher) noexcept override;
    [[nodiscard]] IInputDispatcher* input_dispatcher() const noexcept override;

    // Cursor management
    core::Status set_cursor(CursorKind cursor) override;
    [[nodiscard]] CursorKind cursor() const noexcept override;
    core::Status set_cursor_visible(bool visible) override;

    // Text Input & IME integration
    void set_text_input_client(ITextInputClient* client) noexcept override;
    [[nodiscard]] ITextInputClient* text_input_client() const noexcept override;
    void update_ime_caret_rect(const WindowPoint& position, const WindowSize& size) override;

    [[nodiscard]] bool is_on_ui_thread() const noexcept override;

    // Native handle accessors for tests and RHI
    [[nodiscard]] void* native_nswindow() const noexcept;
    [[nodiscard]] void* native_nsview() const noexcept;
    [[nodiscard]] void* native_cametal_layer() const noexcept;

    // Internal notification callbacks from Cocoa
    void on_native_resized(uint32_t width, uint32_t height, float scale);
    void on_native_moved(int32_t x, int32_t y);
    void on_native_focus_changed(bool focused);
    void on_native_close_requested();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    explicit MacosWindow(std::unique_ptr<Impl> impl);
};

} // namespace pingdart::pal::macos
