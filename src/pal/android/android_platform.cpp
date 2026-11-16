// src/pal/android/android_platform.cpp
#include <pingdart/pal/android/android_platform.h>
#include <pingdart/pal/i_input_dispatcher.h>

namespace pingdart::pal::android {

// --- AndroidEventPump ---

void AndroidEventPump::poll_events() {
    std::vector<PointerEvent> local_events;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        local_events.swap(queue_);
        woken_.store(false, std::memory_order_relaxed);
    }
}

bool AndroidEventPump::wait_for_events(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!queue_.empty() || woken_.load(std::memory_order_relaxed)) {
        woken_.store(false, std::memory_order_relaxed);
        return true;
    }
    cv_.wait_for(lock, timeout, [this] {
        return !queue_.empty() || woken_.load(std::memory_order_relaxed);
    });
    woken_.store(false, std::memory_order_relaxed);
    return !queue_.empty();
}

void AndroidEventPump::wake_up() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        woken_.store(true, std::memory_order_release);
    }
    cv_.notify_all();
}

void AndroidEventPump::post_event(const PointerEvent& event) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(event);
    }
    cv_.notify_one();
}

size_t AndroidEventPump::pending_event_count() const noexcept {
    return queue_.size();
}

// --- AndroidWindow ---

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

void AndroidWindow::dispatch_motion_event(
    AndroidMotionEventAction action,
    uint32_t pointer_index,
    const std::vector<TouchPoint>& touches)
{
    if (!dispatcher_ || touches.empty()) {
        return;
    }

    EventType target_type = EventType::PointerMove;
    PointerButton btn = PointerButton::None;

    switch (action) {
        case AndroidMotionEventAction::Down:
        case AndroidMotionEventAction::PointerDown:
            target_type = EventType::PointerDown;
            btn = PointerButton::Left;
            break;
        case AndroidMotionEventAction::Up:
        case AndroidMotionEventAction::PointerUp:
            target_type = EventType::PointerUp;
            btn = PointerButton::Left;
            break;
        case AndroidMotionEventAction::Move:
            target_type = EventType::PointerMove;
            btn = PointerButton::Left;
            break;
        case AndroidMotionEventAction::Cancel:
            target_type = EventType::PointerLeave;
            break;
        default:
            return;
    }

    if (action == AndroidMotionEventAction::Move) {
        for (const auto& touch : touches) {
            PointerEvent pe{};
            pe.type = target_type;
            pe.pointer_id = touch.touch_id;
            pe.x = touch.x;
            pe.y = touch.y;
            pe.pressure = touch.pressure;
            pe.button = btn;
            pe.buttons_down = (btn != PointerButton::None ? static_cast<uint8_t>(btn) : uint8_t{0});
            pe.timestamp_us = touch.timestamp_us;
            dispatcher_->dispatch_pointer_event(pe);
        }
    } else {
        const auto& active_touch = (pointer_index < touches.size()) ? touches[pointer_index] : touches[0];
        PointerEvent pe{};
        pe.type = target_type;
        pe.pointer_id = active_touch.touch_id;
        pe.x = active_touch.x;
        pe.y = active_touch.y;
        pe.pressure = active_touch.pressure;
        pe.button = btn;
        pe.buttons_down = (action == AndroidMotionEventAction::Down || action == AndroidMotionEventAction::PointerDown) ? static_cast<uint8_t>(PointerButton::Left) : uint8_t{0};
        pe.timestamp_us = active_touch.timestamp_us;
        dispatcher_->dispatch_pointer_event(pe);
    }
}

// --- AndroidPlatformHost ---

AndroidPlatformHost::AndroidPlatformHost() = default;
AndroidPlatformHost::~AndroidPlatformHost() {
    shutdown();
}

core::Status AndroidPlatformHost::initialize() {
    initialized_.store(true, std::memory_order_release);
    lifecycle_.store(AndroidLifecycleState::Created, std::memory_order_release);
    return core::Status{};
}

void AndroidPlatformHost::shutdown() {
    initialized_.store(false, std::memory_order_release);
    lifecycle_.store(AndroidLifecycleState::Destroyed, std::memory_order_release);
}

PlatformType AndroidPlatformHost::platform_type() const noexcept {
    return PlatformType::Android;
}

IPlatformEventPump& AndroidPlatformHost::event_pump() {
    return event_pump_;
}

core::Result<std::unique_ptr<IWindow>> AndroidPlatformHost::create_window(
    std::string_view title,
    const WindowSize& initial_size)
{
    return std::make_unique<AndroidWindow>(title, initial_size);
}

void AndroidPlatformHost::request_quit() noexcept {
    quit_requested_.store(true, std::memory_order_release);
}

bool AndroidPlatformHost::is_quit_requested() const noexcept {
    return quit_requested_.load(std::memory_order_acquire);
}

void AndroidPlatformHost::handle_app_cmd(AndroidAppCmd cmd) {
    switch (cmd) {
        case AndroidAppCmd::Start:
            lifecycle_.store(AndroidLifecycleState::Started, std::memory_order_release);
            break;
        case AndroidAppCmd::Resume:
            lifecycle_.store(AndroidLifecycleState::Resumed, std::memory_order_release);
            break;
        case AndroidAppCmd::Pause:
            lifecycle_.store(AndroidLifecycleState::Paused, std::memory_order_release);
            break;
        case AndroidAppCmd::Stop:
            lifecycle_.store(AndroidLifecycleState::Stopped, std::memory_order_release);
            break;
        case AndroidAppCmd::Destroy:
            lifecycle_.store(AndroidLifecycleState::Destroyed, std::memory_order_release);
            request_quit();
            break;
        default:
            break;
    }
}

AndroidLifecycleState AndroidPlatformHost::lifecycle_state() const noexcept {
    return lifecycle_.load(std::memory_order_acquire);
}

} // namespace pingdart::pal::android
