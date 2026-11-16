// include/pingdart/pal/i_window.h
#pragma once
#include <pingdart/pal/native_types.h>
#include <pingdart/pal/input_events.h>
#include <pingdart/core/result.h>
#include <string_view>

namespace pingdart::pal {

class IInputDispatcher;

class ISurfaceProvider {
public:
    virtual ~ISurfaceProvider() = default;

    // Returns current native handle and generation metadata
    [[nodiscard]] virtual NativeSurfaceHandle native_surface_handle() const noexcept = 0;

    // Monotonically increasing generation IDs
    [[nodiscard]] virtual uint64_t surface_generation_id() const noexcept = 0;
    [[nodiscard]] virtual uint64_t swapchain_generation_id() const noexcept = 0;

    // Physical drawable dimensions in pixels
    [[nodiscard]] virtual WindowSize physical_size() const noexcept = 0;

    // Display DPI scaling factor
    [[nodiscard]] virtual float scale_factor() const noexcept = 0;

    // Surface lifecycle and destruction coordination
    [[nodiscard]] virtual SurfaceLifecycleState surface_lifecycle_state() const noexcept = 0;

    // Step 1 of destruction: UI thread initiates retirement
    virtual void mark_surface_retiring() noexcept = 0;

    // Step 2 & 3 of destruction: Render thread finishes in-flight GPU frames, releases resources, and acknowledges
    virtual void acknowledge_surface_retirement() noexcept = 0;

    // Step 4: UI thread confirms surface can safely be destroyed
    [[nodiscard]] virtual bool is_surface_ready_for_destruction() const noexcept = 0;

    // Fail-safe override if GPU thread crashes or device is lost
    virtual void force_surface_retirement_timeout() noexcept = 0;
};

class IWindow : public ISurfaceProvider {
public:
    ~IWindow() override = default;

    // Window visibility and lifecycle (Must be called on UI thread)
    virtual core::Status show() = 0;
    virtual core::Status hide() = 0;
    virtual core::Status close() = 0;

    // Window geometry and presentation (Must be called on UI thread)
    virtual core::Status set_title(std::string_view title) = 0;
    virtual core::Status set_size(const WindowSize& size) = 0;
    virtual core::Status set_position(const WindowPoint& position) = 0;

    [[nodiscard]] virtual WindowSize size() const noexcept = 0;
    [[nodiscard]] virtual WindowPoint position() const noexcept = 0;

    [[nodiscard]] virtual bool is_visible() const noexcept = 0;
    [[nodiscard]] virtual bool is_minimized() const noexcept = 0;
    [[nodiscard]] virtual bool is_maximized() const noexcept = 0;

    // Input event routing
    virtual void set_input_dispatcher(IInputDispatcher* dispatcher) noexcept = 0;
    [[nodiscard]] virtual IInputDispatcher* input_dispatcher() const noexcept = 0;

    // Cursor management (Must be called on UI thread)
    virtual core::Status set_cursor(CursorKind cursor) = 0;
    [[nodiscard]] virtual CursorKind cursor() const noexcept = 0;
    virtual core::Status set_cursor_visible(bool visible) = 0;

    // IME & text input client integration
    virtual void set_text_input_client(ITextInputClient* client) noexcept = 0;
    [[nodiscard]] virtual ITextInputClient* text_input_client() const noexcept = 0;
    virtual void update_ime_caret_rect(const WindowPoint& position, const WindowSize& size) = 0;

    // UI-thread verification
    [[nodiscard]] virtual bool is_on_ui_thread() const noexcept = 0;
};

} // namespace pingdart::pal
