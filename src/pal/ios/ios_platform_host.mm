// src/pal/ios/ios_platform_host.mm
#include <pingdart/pal/ios/ios_platform.h>

namespace pingdart::pal::ios {

// --- IOSEventPump ---

void IOSEventPump::poll_events() {
    std::vector<PointerEvent> local_events;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        local_events.swap(queue_);
        woken_.store(false, std::memory_order_relaxed);
    }
}

bool IOSEventPump::wait_for_events(std::chrono::milliseconds timeout) {
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

void IOSEventPump::wake_up() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        woken_.store(true, std::memory_order_release);
    }
    cv_.notify_all();
}

void IOSEventPump::post_event(const PointerEvent& event) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(event);
    }
    cv_.notify_one();
}

size_t IOSEventPump::pending_event_count() const noexcept {
    return queue_.size();
}

// --- IOSPlatformHost ---

IOSPlatformHost::IOSPlatformHost() = default;
IOSPlatformHost::~IOSPlatformHost() {
    shutdown();
}

core::Status IOSPlatformHost::initialize() {
    initialized_.store(true, std::memory_order_release);
    lifecycle_.store(IOSLifecycleState::Active, std::memory_order_release);
    return core::Status{};
}

void IOSPlatformHost::shutdown() {
    initialized_.store(false, std::memory_order_release);
    lifecycle_.store(IOSLifecycleState::NotRunning, std::memory_order_release);
}

PlatformType IOSPlatformHost::platform_type() const noexcept {
    return PlatformType::IOS;
}

IPlatformEventPump& IOSPlatformHost::event_pump() {
    return event_pump_;
}

core::Result<std::unique_ptr<IWindow>> IOSPlatformHost::create_window(
    std::string_view title,
    const WindowSize& initial_size)
{
    return std::make_unique<IOSWindow>(title, initial_size);
}

void IOSPlatformHost::request_quit() noexcept {
    quit_requested_.store(true, std::memory_order_release);
}

bool IOSPlatformHost::is_quit_requested() const noexcept {
    return quit_requested_.load(std::memory_order_acquire);
}

void IOSPlatformHost::on_application_did_become_active() {
    lifecycle_.store(IOSLifecycleState::Active, std::memory_order_release);
}

void IOSPlatformHost::on_application_will_resign_active() {
    lifecycle_.store(IOSLifecycleState::Inactive, std::memory_order_release);
}

void IOSPlatformHost::on_application_did_enter_background() {
    lifecycle_.store(IOSLifecycleState::Background, std::memory_order_release);
}

void IOSPlatformHost::on_application_will_enter_foreground() {
    lifecycle_.store(IOSLifecycleState::Inactive, std::memory_order_release);
}

IOSLifecycleState IOSPlatformHost::lifecycle_state() const noexcept {
    return lifecycle_.load(std::memory_order_acquire);
}

} // namespace pingdart::pal::ios
