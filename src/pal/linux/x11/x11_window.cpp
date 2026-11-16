// src/pal/linux/x11/x11_window.cpp
#include "x11_window.h"
#include "x11_input_dispatcher.h"
#include <chrono>

#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

namespace pingdart::pal::linux_x11 {

core::Result<std::unique_ptr<X11Window>> X11Window::create(
    Display* display,
    std::string_view title,
    const WindowSize& size
) {
    if (!display) {
        return core::Status::InvalidArgument("X11 Display is null");
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    XSetWindowAttributes attrs{};
    attrs.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask |
                       KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
                       PointerMotionMask | FocusChangeMask;

    Window win = XCreateWindow(
        display,
        root,
        0, 0,
        size.width, size.height,
        0,
        CopyFromParent,
        InputOutput,
        CopyFromParent,
        CWEventMask,
        &attrs
    );

    if (!win) {
        return core::Status::Internal("XCreateWindow failed");
    }

    // Register WM_DELETE_WINDOW protocol
    Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, win, &wm_delete, 1);

    std::string str_title(title);
    XStoreName(display, win, str_title.c_str());

    auto window = std::unique_ptr<X11Window>(new X11Window(display, win, size));
    window->wm_delete_window_ = wm_delete;

    return window;
}

#else

namespace pingdart::pal::linux_x11 {

core::Result<std::unique_ptr<X11Window>> X11Window::create(
    Display* display,
    std::string_view title,
    const WindowSize& size
) {
    (void)display;
    (void)title;
    (void)size;
    return core::Status::Unimplemented("X11 only supported on Linux targets");
}

#endif

X11Window::X11Window(Display* display, Window window, WindowSize size)
    : display_(display), window_(window), size_(size),
      ui_thread_id_(std::this_thread::get_id()) {}

X11Window::~X11Window() {
    close();
#if defined(__linux__)
    if (display_ && window_) {
        XDestroyWindow(display_, window_);
        window_ = 0;
    }
#endif
    lifecycle_state_ = SurfaceLifecycleState::Destroyed;
}

NativeSurfaceHandle X11Window::native_surface_handle() const noexcept {
    NativeSurfaceHandle handle;
    handle.kind = SurfaceKind::X11;
    handle.token = X11SurfaceDescriptor{
        reinterpret_cast<void*>(display_),
        static_cast<uint64_t>(window_),
        nullptr
    };
    handle.surface_generation_id = surface_generation_id_;
    handle.swapchain_generation_id = swapchain_generation_id_;
    handle.generation_id = surface_generation_id_;
    handle.lifecycle_state = lifecycle_state_;
    handle.is_valid = (window_ != 0) && (lifecycle_state_ != SurfaceLifecycleState::Destroyed);
    return handle;
}

uint64_t X11Window::surface_generation_id() const noexcept {
    return surface_generation_id_;
}

uint64_t X11Window::swapchain_generation_id() const noexcept {
    return swapchain_generation_id_;
}

WindowSize X11Window::physical_size() const noexcept {
    return size_;
}

float X11Window::scale_factor() const noexcept {
    return scale_factor_;
}

SurfaceLifecycleState X11Window::surface_lifecycle_state() const noexcept {
    return lifecycle_state_;
}

void X11Window::mark_surface_retiring() noexcept {
    lifecycle_state_ = SurfaceLifecycleState::Retiring;
}

void X11Window::acknowledge_surface_retirement() noexcept {
    lifecycle_state_ = SurfaceLifecycleState::Retired;
}

bool X11Window::is_surface_ready_for_destruction() const noexcept {
    return lifecycle_state_ == SurfaceLifecycleState::Retired;
}

void X11Window::force_surface_retirement_timeout() noexcept {
    lifecycle_state_ = SurfaceLifecycleState::Abandoned;
}

core::Status X11Window::show() {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
#if defined(__linux__)
    if (display_ && window_) {
        XMapWindow(display_, window_);
        XFlush(display_);
    }
#endif
    visible_ = true;
    return core::Status::Ok();
}

core::Status X11Window::hide() {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
#if defined(__linux__)
    if (display_ && window_) {
        XUnmapWindow(display_, window_);
        XFlush(display_);
    }
#endif
    visible_ = false;
    return core::Status::Ok();
}

core::Status X11Window::close() {
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
            X11InputHelper::current_timestamp_us()
        });
    }
    return core::Status::Ok();
}

core::Status X11Window::set_title(std::string_view title) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
#if defined(__linux__)
    if (display_ && window_) {
        std::string str_title(title);
        XStoreName(display_, window_, str_title.c_str());
        XFlush(display_);
    }
#endif
    return core::Status::Ok();
}

core::Status X11Window::set_size(const WindowSize& size) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
#if defined(__linux__)
    if (display_ && window_) {
        XResizeWindow(display_, window_, size.width, size.height);
        XFlush(display_);
    }
#endif
    size_ = size;
    swapchain_generation_id_++;
    return core::Status::Ok();
}

core::Status X11Window::set_position(const WindowPoint& position) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Window mutations must occur on UI thread");
    }
#if defined(__linux__)
    if (display_ && window_) {
        XMoveWindow(display_, window_, position.x, position.y);
        XFlush(display_);
    }
#endif
    position_ = position;
    return core::Status::Ok();
}

WindowSize X11Window::size() const noexcept {
    return size_;
}

WindowPoint X11Window::position() const noexcept {
    return position_;
}

bool X11Window::is_visible() const noexcept {
    return visible_;
}

bool X11Window::is_minimized() const noexcept {
    return false;
}

bool X11Window::is_maximized() const noexcept {
    return false;
}

void X11Window::set_input_dispatcher(IInputDispatcher* dispatcher) noexcept {
    input_dispatcher_ = dispatcher;
}

IInputDispatcher* X11Window::input_dispatcher() const noexcept {
    return input_dispatcher_;
}

core::Status X11Window::set_cursor(CursorKind cursor) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Cursor mutations must occur on UI thread");
    }
    cursor_ = cursor;
    return core::Status::Ok();
}

CursorKind X11Window::cursor() const noexcept {
    return cursor_;
}

core::Status X11Window::set_cursor_visible(bool visible) {
    if (!is_on_ui_thread()) {
        return core::Status::FailedPrecondition("Cursor mutations must occur on UI thread");
    }
    cursor_visible_ = visible;
    return core::Status::Ok();
}

void X11Window::set_text_input_client(ITextInputClient* client) noexcept {
    text_client_ = client;
}

ITextInputClient* X11Window::text_input_client() const noexcept {
    return text_client_;
}

void X11Window::update_ime_caret_rect(const WindowPoint& position, const WindowSize& size) {
    (void)position;
    (void)size;
}

bool X11Window::is_on_ui_thread() const noexcept {
    return std::this_thread::get_id() == ui_thread_id_;
}

void X11Window::handle_x_event(const XEvent& event) {
#if defined(__linux__)
    const uint64_t timestamp = X11InputHelper::current_timestamp_us();

    switch (event.type) {
        case ClientMessage: {
            if (static_cast<Atom>(event.xclient.data.l[0]) == wm_delete_window_) {
                close();
            }
            break;
        }

        case ConfigureNotify: {
            uint32_t width = static_cast<uint32_t>(event.xconfigure.width);
            uint32_t height = static_cast<uint32_t>(event.xconfigure.height);
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
            break;
        }

        case MotionNotify: {
            if (input_dispatcher_) {
                PointerEvent pe;
                pe.type = EventType::PointerMove;
                pe.x = static_cast<double>(event.xmotion.x);
                pe.y = static_cast<double>(event.xmotion.y);
                pe.modifiers = X11InputHelper::map_x11_modifiers(event.xmotion.state);
                pe.timestamp_us = timestamp;
                input_dispatcher_->dispatch_pointer_event(pe);
            }
            break;
        }

        case ButtonPress:
        case ButtonRelease: {
            if (input_dispatcher_) {
                if (event.xbutton.button == 4 || event.xbutton.button == 5) {
                    // Scroll wheel
                    PointerEvent pe;
                    pe.type = EventType::PointerWheel;
                    pe.x = static_cast<double>(event.xbutton.x);
                    pe.y = static_cast<double>(event.xbutton.y);
                    pe.delta_y = (event.xbutton.button == 4) ? 1.0 : -1.0;
                    pe.modifiers = X11InputHelper::map_x11_modifiers(event.xbutton.state);
                    pe.timestamp_us = timestamp;
                    input_dispatcher_->dispatch_pointer_event(pe);
                } else {
                    PointerEvent pe;
                    pe.type = (event.type == ButtonPress) ? EventType::PointerDown : EventType::PointerUp;
                    pe.x = static_cast<double>(event.xbutton.x);
                    pe.y = static_cast<double>(event.xbutton.y);
                    pe.button = X11InputHelper::map_x11_button(event.xbutton.button);
                    pe.modifiers = X11InputHelper::map_x11_modifiers(event.xbutton.state);
                    pe.timestamp_us = timestamp;
                    input_dispatcher_->dispatch_pointer_event(pe);
                }
            }
            break;
        }

        case KeyPress:
        case KeyRelease: {
            if (input_dispatcher_) {
                KeyboardEvent ke;
                ke.type = (event.type == KeyPress) ? EventType::KeyDown : EventType::KeyUp;
                ke.key_code = event.xkey.keycode;
                ke.modifiers = X11InputHelper::map_x11_modifiers(event.xkey.state);
                ke.timestamp_us = timestamp;
                input_dispatcher_->dispatch_keyboard_event(ke);
            }
            break;
        }

        default:
            break;
    }
#else
    (void)event;
#endif
}

} // namespace pingdart::pal::linux_x11
