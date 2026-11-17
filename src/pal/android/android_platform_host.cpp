// src/pal/android/android_platform_host.cpp
#include <pingdart/pal/android/android_platform.h>

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
