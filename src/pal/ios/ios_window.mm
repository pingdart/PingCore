// src/pal/ios/ios_window.mm
#include <pingdart/pal/ios/ios_platform.h>
#include <pingdart/pal/i_input_dispatcher.h>

namespace pingdart::pal::ios {

IOSWindow::IOSWindow(std::string_view title, const WindowSize& initial_size)
    : title_(title), size_(initial_size) {
    surface_handle_.kind = SurfaceKind::IOSMetal;
    surface_handle_.token = std::monostate{};
    surface_handle_.surface_generation_id = 0;
    surface_handle_.swapchain_generation_id = 0;
    surface_handle_.generation_id = 0;
    surface_handle_.lifecycle_state = SurfaceLifecycleState::Retired;
    surface_handle_.is_valid = false;
}

IOSWindow::~IOSWindow() {
    detach_metal_surface();
}

NativeSurfaceHandle IOSWindow::native_surface_handle() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return surface_handle_;
}

uint64_t IOSWindow::surface_generation_id() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return surface_handle_.surface_generation_id;
}

uint64_t IOSWindow::swapchain_generation_id() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return surface_handle_.swapchain_generation_id;
}

WindowSize IOSWindow::physical_size() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return WindowSize{
        static_cast<uint32_t>(static_cast<float>(size_.width) * scale_factor_),
        static_cast<uint32_t>(static_cast<float>(size_.height) * scale_factor_)
    };
}

float IOSWindow::scale_factor() const noexcept {
    return scale_factor_;
}

SurfaceLifecycleState IOSWindow::surface_lifecycle_state() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return surface_handle_.lifecycle_state;
}

void IOSWindow::mark_surface_retiring() noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    if (surface_handle_.lifecycle_state == SurfaceLifecycleState::Active) {
        surface_handle_.lifecycle_state = SurfaceLifecycleState::Retiring;
    }
}

void IOSWindow::acknowledge_surface_retirement() noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    surface_handle_.lifecycle_state = SurfaceLifecycleState::Retired;
    surface_handle_.is_valid = false;
}

bool IOSWindow::is_surface_ready_for_destruction() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return surface_handle_.lifecycle_state == SurfaceLifecycleState::Retired ||
           surface_handle_.lifecycle_state == SurfaceLifecycleState::Destroyed;
}

void IOSWindow::force_surface_retirement_timeout() noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    surface_handle_.lifecycle_state = SurfaceLifecycleState::Abandoned;
    surface_handle_.is_valid = false;
}

core::Status IOSWindow::show() {
    visible_ = true;
    return core::Status{};
}

core::Status IOSWindow::hide() {
    visible_ = false;
    return core::Status{};
}

core::Status IOSWindow::close() {
    detach_metal_surface();
    return core::Status{};
}

core::Status IOSWindow::set_title(std::string_view title) {
    title_ = title;
    return core::Status{};
}

core::Status IOSWindow::set_size(const WindowSize& size) {
    size_ = size;
    return core::Status{};
}

core::Status IOSWindow::set_position(const WindowPoint& position) {
    position_ = position;
    return core::Status{};
}

WindowSize IOSWindow::size() const noexcept {
    return size_;
}

WindowPoint IOSWindow::position() const noexcept {
    return position_;
}

bool IOSWindow::is_visible() const noexcept {
    return visible_;
}

bool IOSWindow::is_minimized() const noexcept {
    return !visible_;
}

bool IOSWindow::is_maximized() const noexcept {
    return true; // iOS applications run full-screen
}

void IOSWindow::set_input_dispatcher(IInputDispatcher* dispatcher) noexcept {
    dispatcher_ = dispatcher;
}

IInputDispatcher* IOSWindow::input_dispatcher() const noexcept {
    return dispatcher_;
}

core::Status IOSWindow::set_cursor(CursorKind cursor) {
    cursor_ = cursor;
    return core::Status{};
}

CursorKind IOSWindow::cursor() const noexcept {
    return cursor_;
}

core::Status IOSWindow::set_cursor_visible(bool visible) {
    cursor_visible_ = visible;
    return core::Status{};
}

void IOSWindow::set_text_input_client(ITextInputClient* client) noexcept {
    text_input_client_ = client;
}

ITextInputClient* IOSWindow::text_input_client() const noexcept {
    return text_input_client_;
}

void IOSWindow::update_ime_caret_rect(const WindowPoint& /*position*/, const WindowSize& /*size*/) {
}

bool IOSWindow::is_on_ui_thread() const noexcept {
    return std::this_thread::get_id() == ui_thread_id_;
}

void IOSWindow::setup_metal_surface(
    void* ui_view,
    void* metal_layer,
    uint32_t logical_w,
    uint32_t logical_h,
    float scale)
{
    std::lock_guard<std::mutex> lock(surface_mutex_);
    surface_handle_.kind = SurfaceKind::IOSMetal;
    surface_handle_.token = IOSSurfaceDescriptor{ui_view, metal_layer};
    surface_handle_.surface_generation_id++;
    surface_handle_.swapchain_generation_id = surface_handle_.surface_generation_id;
    surface_handle_.generation_id = surface_handle_.surface_generation_id;
    surface_handle_.lifecycle_state = SurfaceLifecycleState::Active;
    surface_handle_.is_valid = (metal_layer != nullptr);

    scale_factor_ = scale;
    size_ = WindowSize{logical_w, logical_h};

    if (dispatcher_) {
        PointerEvent resize_evt{};
        resize_evt.type = EventType::WindowResized;
        resize_evt.x = static_cast<double>(logical_w);
        resize_evt.y = static_cast<double>(logical_h);
        dispatcher_->dispatch_pointer_event(resize_evt);
    }
}

void IOSWindow::update_drawable_size(uint32_t logical_w, uint32_t logical_h) {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    surface_handle_.swapchain_generation_id++;
    size_ = WindowSize{logical_w, logical_h};

    if (dispatcher_) {
        PointerEvent resize_evt{};
        resize_evt.type = EventType::WindowResized;
        resize_evt.x = static_cast<double>(logical_w);
        resize_evt.y = static_cast<double>(logical_h);
        dispatcher_->dispatch_pointer_event(resize_evt);
    }
}

void IOSWindow::detach_metal_surface() {
    {
        std::lock_guard<std::mutex> lock(surface_mutex_);
        if (surface_handle_.lifecycle_state == SurfaceLifecycleState::Active) {
            surface_handle_.lifecycle_state = SurfaceLifecycleState::Retiring;
        }
        surface_handle_.lifecycle_state = SurfaceLifecycleState::Retired;
        surface_handle_.is_valid = false;
        surface_handle_.token = std::monostate{};
    }

    if (dispatcher_) {
        PointerEvent close_evt{};
        close_evt.type = EventType::WindowCloseRequested;
        dispatcher_->dispatch_pointer_event(close_evt);
    }
}

} // namespace pingdart::pal::ios
