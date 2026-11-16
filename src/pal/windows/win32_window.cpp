// src/pal/windows/win32_window.cpp
#include "win32_window.h"
#include "win32_input_dispatcher.h"
#include <windowsx.h>
#include <vector>

namespace pingdart::pal::windows {

namespace {

constexpr const wchar_t* kWindowClassName = L"PingDart_WindowClass_V1";

std::wstring Utf8ToUtf16(std::string_view utf8) {
    if (utf8.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring utf16(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), &utf16[0], size_needed);
    return utf16;
}

} // namespace

core::Result<std::unique_ptr<Win32Window>> Win32Window::create(
    HINSTANCE hinstance,
    std::string_view title,
    const WindowSize& size
) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = Win32Window::window_proc_thunk;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hinstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // Let renderer paint
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = kWindowClassName;
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    RegisterClassExW(&wc);

    RECT wr = {0, 0, static_cast<LONG>(size.width), static_cast<LONG>(size.height)};
    AdjustWindowRectEx(&wr, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);

    std::wstring wtitle = Utf8ToUtf16(title);

    HWND hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        kWindowClassName,
        wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr,
        nullptr,
        hinstance,
        nullptr
    );

    if (!hwnd) {
        return core::Status::Internal("CreateWindowExW failed", static_cast<int32_t>(GetLastError()));
    }

    // Determine initial DPI scale
    UINT dpi = GetDpiForWindow(hwnd);
    float scale = (dpi > 0) ? (static_cast<float>(dpi) / 96.0f) : 1.0f;

    auto window = std::unique_ptr<Win32Window>(new Win32Window(hwnd, hinstance, size, scale));
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window.get()));

    return window;
}

Win32Window::Win32Window(HWND hwnd, HINSTANCE hinstance, WindowSize size, float scale)
    : hwnd_(hwnd), hinstance_(hinstance), size_(size), scale_factor_(scale),
      ui_thread_id_(std::this_thread::get_id()) {}

Win32Window::~Win32Window() {
    if (hwnd_) {
        SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    lifecycle_state_ = SurfaceLifecycleState::Destroyed;
}

NativeSurfaceHandle Win32Window::native_surface_handle() const noexcept {
    NativeSurfaceHandle handle;
    handle.kind = SurfaceKind::Win32;
    handle.token = Win32SurfaceDescriptor{hwnd_, hinstance_};
    handle.surface_generation_id = surface_generation_id_;
    handle.swapchain_generation_id = swapchain_generation_id_;
    handle.generation_id = surface_generation_id_;
    handle.lifecycle_state = lifecycle_state_;
    handle.is_valid = (hwnd_ != nullptr) && (lifecycle_state_ != SurfaceLifecycleState::Destroyed);
    return handle;
}

uint64_t Win32Window::surface_generation_id() const noexcept {
    return surface_generation_id_;
}

uint64_t Win32Window::swapchain_generation_id() const noexcept {
    return swapchain_generation_id_;
}

WindowSize Win32Window::physical_size() const noexcept {
    return size_;
}

float Win32Window::scale_factor() const noexcept {
    return scale_factor_;
}

SurfaceLifecycleState Win32Window::surface_lifecycle_state() const noexcept {
    return lifecycle_state_;
}

void Win32Window::mark_surface_retiring() noexcept {
    lifecycle_state_ = SurfaceLifecycleState::Retiring;
}

void Win32Window::acknowledge_surface_retirement() noexcept {
    lifecycle_state_ = SurfaceLifecycleState::Retired;
}

bool Win32Window::is_surface_ready_for_destruction() const noexcept {
    return lifecycle_state_ == SurfaceLifecycleState::Retired;
}

void Win32Window::force_surface_retirement_timeout() noexcept {
    lifecycle_state_ = SurfaceLifecycleState::Abandoned;
}

core::Status Win32Window::show() {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
    ShowWindow(hwnd_, SW_SHOWNORMAL);
    UpdateWindow(hwnd_);
    visible_ = true;
    return core::Status::Ok();
}

core::Status Win32Window::hide() {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
    ShowWindow(hwnd_, SW_HIDE);
    visible_ = false;
    return core::Status::Ok();
}

core::Status Win32Window::close() {
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
            Win32InputHelper::current_timestamp_us()
        });
    }
    return core::Status::Ok();
}

core::Status Win32Window::set_title(std::string_view title) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
    std::wstring wtitle = Utf8ToUtf16(title);
    SetWindowTextW(hwnd_, wtitle.c_str());
    return core::Status::Ok();
}

core::Status Win32Window::set_size(const WindowSize& size) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
    RECT wr = {0, 0, static_cast<LONG>(size.width), static_cast<LONG>(size.height)};
    AdjustWindowRectEx(&wr, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);
    SetWindowPos(hwnd_, nullptr, 0, 0, wr.right - wr.left, wr.bottom - wr.top, SWP_NOMOVE | SWP_NOZORDER);
    return core::Status::Ok();
}

core::Status Win32Window::set_position(const WindowPoint& position) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
    SetWindowPos(hwnd_, nullptr, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    return core::Status::Ok();
}

WindowSize Win32Window::size() const noexcept {
    return size_;
}

WindowPoint Win32Window::position() const noexcept {
    return position_;
}

bool Win32Window::is_visible() const noexcept {
    return visible_;
}

bool Win32Window::is_minimized() const noexcept {
    return IsIconic(hwnd_) != FALSE;
}

bool Win32Window::is_maximized() const noexcept {
    return IsZoomed(hwnd_) != FALSE;
}

void Win32Window::set_input_dispatcher(IInputDispatcher* dispatcher) noexcept {
    input_dispatcher_ = dispatcher;
}

IInputDispatcher* Win32Window::input_dispatcher() const noexcept {
    return input_dispatcher_;
}

core::Status Win32Window::set_cursor(CursorKind cursor) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Cursor mutations must occur on UI thread");
    }
    cursor_ = cursor;
    LPCWSTR idc = IDC_ARROW;
    switch (cursor) {
        case CursorKind::Arrow: idc = IDC_ARROW; break;
        case CursorKind::IBeam: idc = IDC_IBEAM; break;
        case CursorKind::PointingHand: idc = IDC_HAND; break;
        case CursorKind::Crosshair: idc = IDC_CROSS; break;
        case CursorKind::ResizeHorizontal: idc = IDC_SIZEWE; break;
        case CursorKind::ResizeVertical: idc = IDC_SIZENS; break;
        case CursorKind::Wait: idc = IDC_WAIT; break;
        case CursorKind::Hidden: idc = nullptr; break;
    }
    if (idc) {
        SetCursor(LoadCursorW(nullptr, idc));
    } else {
        SetCursor(nullptr);
    }
    return core::Status::Ok();
}

CursorKind Win32Window::cursor() const noexcept {
    return cursor_;
}

core::Status Win32Window::set_cursor_visible(bool visible) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Cursor mutations must occur on UI thread");
    }
    cursor_visible_ = visible;
    ShowCursor(visible ? TRUE : FALSE);
    return core::Status::Ok();
}

void Win32Window::set_text_input_client(ITextInputClient* client) noexcept {
    text_client_ = client;
}

ITextInputClient* Win32Window::text_input_client() const noexcept {
    return text_client_;
}

void Win32Window::update_ime_caret_rect(const WindowPoint& position, const WindowSize& size) {
    (void)size;
    HIMC imc = ImmGetContext(hwnd_);
    if (imc) {
        COMPOSITIONFORM cf{};
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = position.x;
        cf.ptCurrentPos.y = position.y;
        ImmSetCompositionWindow(imc, &cf);
        ImmReleaseContext(hwnd_, imc);
    }
}

bool Win32Window::is_on_ui_thread() const noexcept {
    return std::this_thread::get_id() == ui_thread_id_;
}

LRESULT CALLBACK Win32Window::window_proc_thunk(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto* window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (window) {
        return window->handle_message(hwnd, msg, wparam, lparam);
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

LRESULT Win32Window::handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    const uint64_t timestamp = Win32InputHelper::current_timestamp_us();

    switch (msg) {
        case WM_SIZE: {
            uint32_t width = LOWORD(lparam);
            uint32_t height = HIWORD(lparam);
            if (width != size_.width || height != size_.height) {
                size_ = WindowSize{width, height};
                swapchain_generation_id_++;
                if (input_dispatcher_) {
                    input_dispatcher_->dispatch_lifecycle_event(WindowLifecycleEvent{
                        EventType::WindowResized,
                        width,
                        height,
                        scale_factor_,
                        timestamp
                    });
                }
            }
            return 0;
        }

        case WM_DPICHANGED: {
            UINT new_dpi = HIWORD(wparam);
            scale_factor_ = static_cast<float>(new_dpi) / 96.0f;
            swapchain_generation_id_++;
            auto* prc = reinterpret_cast<RECT*>(lparam);
            SetWindowPos(hwnd, nullptr, prc->left, prc->top,
                         prc->right - prc->left, prc->bottom - prc->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            if (input_dispatcher_) {
                input_dispatcher_->dispatch_lifecycle_event(WindowLifecycleEvent{
                    EventType::WindowDpiChanged,
                    size_.width,
                    size_.height,
                    scale_factor_,
                    timestamp
                });
            }
            return 0;
        }

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP: {
            if (input_dispatcher_) {
                EventType type = EventType::PointerMove;
                if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN) {
                    type = EventType::PointerDown;
                } else if (msg == WM_LBUTTONUP || msg == WM_RBUTTONUP || msg == WM_MBUTTONUP) {
                    type = EventType::PointerUp;
                }

                PointerEvent pe;
                pe.type = type;
                pe.pointer_id = 0;
                pe.x = static_cast<double>(GET_X_LPARAM(lparam));
                pe.y = static_cast<double>(GET_Y_LPARAM(lparam));
                pe.button = Win32InputHelper::query_mouse_button(msg, wparam);
                pe.buttons_down = Win32InputHelper::query_buttons_down(wparam);
                pe.modifiers = Win32InputHelper::query_modifiers();
                pe.pressure = 1.0f;
                pe.timestamp_us = timestamp;

                input_dispatcher_->dispatch_pointer_event(pe);
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            if (input_dispatcher_) {
                POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                ScreenToClient(hwnd, &pt);

                PointerEvent pe;
                pe.type = EventType::PointerWheel;
                pe.pointer_id = 0;
                pe.x = static_cast<double>(pt.x);
                pe.y = static_cast<double>(pt.y);
                pe.delta_y = static_cast<double>(GET_WHEEL_DELTA_WPARAM(wparam)) / WHEEL_DELTA;
                pe.modifiers = Win32InputHelper::query_modifiers();
                pe.timestamp_us = timestamp;

                input_dispatcher_->dispatch_pointer_event(pe);
            }
            return 0;
        }

        case WM_KEYDOWN:
        case WM_KEYUP: {
            if (input_dispatcher_) {
                KeyboardEvent ke;
                ke.type = (msg == WM_KEYDOWN) ? EventType::KeyDown : EventType::KeyUp;
                ke.key_code = static_cast<uint32_t>((lparam >> 16) & 0xFF);
                ke.virtual_key = Win32InputHelper::normalize_virtual_key(wparam, lparam);
                ke.modifiers = Win32InputHelper::query_modifiers();
                ke.is_repeat = (lparam & (1 << 30)) != 0;
                ke.timestamp_us = timestamp;

                input_dispatcher_->dispatch_keyboard_event(ke);
            }
            return 0;
        }

        case WM_CHAR: {
            if (input_dispatcher_) {
                KeyboardEvent ke;
                ke.type = EventType::CharacterInput;
                ke.unicode_codepoint = static_cast<uint32_t>(wparam);
                ke.modifiers = Win32InputHelper::query_modifiers();
                ke.timestamp_us = timestamp;

                input_dispatcher_->dispatch_keyboard_event(ke);
            }
            return 0;
        }

        case WM_CLOSE: {
            // Trigger surface retirement protocol
            close();
            return 0; // Handled; do not call DefWindowProc to prevent premature HWND destruction
        }

        case WM_DESTROY: {
            lifecycle_state_ = SurfaceLifecycleState::Destroyed;
            PostQuitMessage(0);
            return 0;
        }

        default:
            break;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

} // namespace pingdart::pal::windows
