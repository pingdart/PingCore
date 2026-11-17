// src/pal/android/android_window.cpp
#include <pingdart/pal/android/android_platform.h>
#include <pingdart/pal/i_input_dispatcher.h>

namespace pingdart::pal::android {

AndroidWindow::AndroidWindow(std::string_view title, const WindowSize& initial_size)
    : title_(title), size_(initial_size) {
    surface_handle_.kind = SurfaceKind::AndroidSurface;
    surface_handle_.token = std::monostate{};
    surface_handle_.surface_generation_id = 0;
    surface_handle_.swapchain_generation_id = 0;
    surface_handle_.generation_id = 0;
    surface_handle_.lifecycle_state = SurfaceLifecycleState::Retired;
    surface_handle_.is_valid = false;
}

AndroidWindow::~AndroidWindow() {
    on_native_window_destroyed();
}

NativeSurfaceHandle AndroidWindow::native_surface_handle() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return surface_handle_;
}

uint64_t AndroidWindow::surface_generation_id() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return surface_handle_.surface_generation_id;
}

uint64_t AndroidWindow::swapchain_generation_id() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return surface_handle_.swapchain_generation_id;
}

WindowSize AndroidWindow::physical_size() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return size_;
}

float AndroidWindow::scale_factor() const noexcept {
    return metrics_.density_scale;
}

SurfaceLifecycleState AndroidWindow::surface_lifecycle_state() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return surface_handle_.lifecycle_state;
}

void AndroidWindow::mark_surface_retiring() noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    if (surface_handle_.lifecycle_state == SurfaceLifecycleState::Active) {
        surface_handle_.lifecycle_state = SurfaceLifecycleState::Retiring;
    }
}

void AndroidWindow::acknowledge_surface_retirement() noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    surface_handle_.lifecycle_state = SurfaceLifecycleState::Retired;
    surface_handle_.is_valid = false;
}

bool AndroidWindow::is_surface_ready_for_destruction() const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return surface_handle_.lifecycle_state == SurfaceLifecycleState::Retired ||
           surface_handle_.lifecycle_state == SurfaceLifecycleState::Destroyed;
}

void AndroidWindow::force_surface_retirement_timeout() noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    surface_handle_.lifecycle_state = SurfaceLifecycleState::Abandoned;
    surface_handle_.is_valid = false;
}

core::Status AndroidWindow::show() {
    visible_ = true;
    return core::Status{};
}

core::Status AndroidWindow::hide() {
    visible_ = false;
    return core::Status{};
}

core::Status AndroidWindow::close() {
    on_native_window_destroyed();
    return core::Status{};
}

core::Status AndroidWindow::set_title(std::string_view title) {
    title_ = title;
    return core::Status{};
}

core::Status AndroidWindow::set_size(const WindowSize& size) {
    size_ = size;
    return core::Status{};
}

core::Status AndroidWindow::set_position(const WindowPoint& position) {
    position_ = position;
    return core::Status{};
}

WindowSize AndroidWindow::size() const noexcept {
    return size_;
}

WindowPoint AndroidWindow::position() const noexcept {
    return position_;
}

bool AndroidWindow::is_visible() const noexcept {
    return visible_;
}

bool AndroidWindow::is_minimized() const noexcept {
    return !visible_;
}

bool AndroidWindow::is_maximized() const noexcept {
    return true; // Android mobile windows are typically full-screen maximized
}

void AndroidWindow::set_input_dispatcher(IInputDispatcher* dispatcher) noexcept {
    dispatcher_ = dispatcher;
}

IInputDispatcher* AndroidWindow::input_dispatcher() const noexcept {
    return dispatcher_;
}

core::Status AndroidWindow::set_cursor(CursorKind cursor) {
    cursor_ = cursor;
    return core::Status{};
}

CursorKind AndroidWindow::cursor() const noexcept {
    return cursor_;
}

core::Status AndroidWindow::set_cursor_visible(bool visible) {
    cursor_visible_ = visible;
    return core::Status{};
}

void AndroidWindow::set_text_input_client(ITextInputClient* client) noexcept {
    text_input_client_ = client;
}

ITextInputClient* AndroidWindow::text_input_client() const noexcept {
    return text_input_client_;
}

void AndroidWindow::update_ime_caret_rect(const WindowPoint& /*position*/, const WindowSize& /*size*/) {
}

bool AndroidWindow::is_on_ui_thread() const noexcept {
    return std::this_thread::get_id() == ui_thread_id_;
}

void AndroidWindow::on_native_window_created(void* anative_window, uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    surface_handle_.kind = SurfaceKind::AndroidSurface;
    surface_handle_.token = AndroidSurfaceDescriptor{anative_window};
    surface_handle_.surface_generation_id++;
    surface_handle_.swapchain_generation_id = surface_handle_.surface_generation_id;
    surface_handle_.generation_id = surface_handle_.surface_generation_id;
    surface_handle_.lifecycle_state = SurfaceLifecycleState::Active;
    surface_handle_.is_valid = (anative_window != nullptr);

    size_ = WindowSize{width, height};
    metrics_.width_pixels = width;
    metrics_.height_pixels = height;

    if (dispatcher_) {
        PointerEvent resize_evt{};
        resize_evt.type = EventType::WindowResized;
        resize_evt.x = static_cast<double>(width);
        resize_evt.y = static_cast<double>(height);
        dispatcher_->dispatch_pointer_event(resize_evt);
    }
}

void AndroidWindow::on_native_window_resized(uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    surface_handle_.swapchain_generation_id++;
    size_ = WindowSize{width, height};
    metrics_.width_pixels = width;
    metrics_.height_pixels = height;

    if (dispatcher_) {
        PointerEvent resize_evt{};
        resize_evt.type = EventType::WindowResized;
        resize_evt.x = static_cast<double>(width);
        resize_evt.y = static_cast<double>(height);
        dispatcher_->dispatch_pointer_event(resize_evt);
    }
}

void AndroidWindow::on_native_window_destroyed() {
    {
        std::lock_guard<std::mutex> lock(surface_mutex_);
        if (surface_handle_.lifecycle_state == SurfaceLifecycleState::Active) {
            surface_handle_.lifecycle_state = SurfaceLifecycleState::Retiring;
        }
        surface_handle_.lifecycle_state = SurfaceLifecycleState::Retired;
        surface_handle_.is_valid = false;
        surface_handle_.surface_generation_id++; // Invalidate queued render operations immediately
        surface_handle_.token = std::monostate{};
    }

    if (dispatcher_) {
        PointerEvent close_evt{};
        close_evt.type = EventType::WindowCloseRequested;
        dispatcher_->dispatch_pointer_event(close_evt);
    }
}

bool AndroidWindow::can_render_frame(uint64_t expected_generation) const noexcept {
    std::lock_guard<std::mutex> lock(surface_mutex_);
    return surface_handle_.is_valid &&
           surface_handle_.lifecycle_state == SurfaceLifecycleState::Active &&
           surface_handle_.surface_generation_id == expected_generation &&
           std::holds_alternative<AndroidSurfaceDescriptor>(surface_handle_.token) &&
           std::get<AndroidSurfaceDescriptor>(surface_handle_.token).native_window != nullptr;
}

void AndroidWindow::set_display_metrics(const AndroidDisplayMetrics& metrics) noexcept {
    metrics_ = metrics;
}

const AndroidDisplayMetrics& AndroidWindow::display_metrics() const noexcept {
    return metrics_;
}

} // namespace pingdart::pal::android
