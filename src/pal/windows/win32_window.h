// src/pal/windows/win32_window.h
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <pingdart/pal/i_window.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include <string>
#include <string_view>
#include <thread>

namespace pingdart::pal::windows {

class Win32Window final : public IWindow {
public:
    static core::Result<std::unique_ptr<Win32Window>> create(
        HINSTANCE hinstance,
        std::string_view title,
        const WindowSize& size
    );

    ~Win32Window() override;

    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;

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

    [[nodiscard]] HWND hwnd() const noexcept { return hwnd_; }

    static LRESULT CALLBACK window_proc_thunk(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

private:
    Win32Window(HWND hwnd, HINSTANCE hinstance, WindowSize size, float scale);

    LRESULT handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    HWND hwnd_{nullptr};
    HINSTANCE hinstance_{nullptr};
    WindowSize size_{0, 0};
    WindowPoint position_{0, 0};
    float scale_factor_{1.0f};
    bool visible_{false};
    uint64_t surface_generation_id_{1};
    uint64_t swapchain_generation_id_{1};
    SurfaceLifecycleState lifecycle_state_{SurfaceLifecycleState::Active};
    IInputDispatcher* input_dispatcher_{nullptr};
    ITextInputClient* text_client_{nullptr};
    CursorKind cursor_{CursorKind::Arrow};
    bool cursor_visible_{true};
    std::thread::id ui_thread_id_;
};

} // namespace pingdart::pal::windows
