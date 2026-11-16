// include/pingdart/pal/ios/ios_platform.h
#pragma once
#include <pingdart/pal/i_platform_host.h>
#include <pingdart/pal/i_window.h>
#include <pingdart/pal/i_platform_event_pump.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include <pingdart/pal/input_events.h>
#include <pingdart/pal/native_types.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <string_view>
#include <functional>
#include <thread>
#include <vector>

namespace pingdart::pal::ios {

enum class IOSLifecycleState : uint8_t {
    NotRunning,
    Inactive,
    Active,
    Background,
    Suspended
};

struct IOSSafeAreaInsets {
    float top{0.0f};
    float bottom{0.0f};
    float left{0.0f};
    float right{0.0f};

    bool operator==(const IOSSafeAreaInsets& other) const noexcept {
        return top == other.top && bottom == other.bottom && left == other.left && right == other.right;
    }
    bool operator!=(const IOSSafeAreaInsets& other) const noexcept {
        return !(*this == other);
    }
};

class IOSEventPump final : public IPlatformEventPump {
public:
    IOSEventPump() = default;
    ~IOSEventPump() override = default;

    void poll_events() override;
    bool wait_for_events(std::chrono::milliseconds timeout) override;
    void wake_up() override;

    void post_event(const PointerEvent& event);
    [[nodiscard]] size_t pending_event_count() const noexcept;

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<PointerEvent> queue_;
    std::atomic<bool> woken_{false};
};

class IOSWindow final : public IWindow {
public:
    IOSWindow(std::string_view title, const WindowSize& initial_size);
    ~IOSWindow() override;

    // ISurfaceProvider implementation
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

    // IWindow implementation
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
    core::Status set_cursor(CursorKind cursor) override;
    [[nodiscard]] CursorKind cursor() const noexcept override;
    core::Status set_cursor_visible(bool visible) override;

    void set_text_input_client(ITextInputClient* client) noexcept override;
    [[nodiscard]] ITextInputClient* text_input_client() const noexcept override;
    void update_ime_caret_rect(const WindowPoint& position, const WindowSize& size) override;

    [[nodiscard]] bool is_on_ui_thread() const noexcept override;

    // iOS UIKit & CAMetalLayer Surface Integration
    void setup_metal_surface(
        void* ui_view,
        void* metal_layer,
        uint32_t logical_w,
        uint32_t logical_h,
        float scale
    );
    void update_drawable_size(uint32_t logical_w, uint32_t logical_h);
    void detach_metal_surface();

    void set_safe_area_insets(const IOSSafeAreaInsets& insets) noexcept;
    void update_safe_area_insets(const IOSSafeAreaInsets& insets);
    void set_on_safe_area_changed(std::function<void(const IOSSafeAreaInsets&)> callback) noexcept;
    [[nodiscard]] const IOSSafeAreaInsets& safe_area_insets() const noexcept;

    // Multi-touch Gesture & Event Ingestion
    void handle_ios_touches(TouchPhase phase, const std::vector<TouchPoint>& touches);

private:
    std::string title_;
    WindowSize size_{393, 852}; // e.g. iPhone 15 Pro logical dimensions
    WindowPoint position_{0, 0};
    float scale_factor_{3.0f};  // 3x Super Retina
    bool visible_{true};
    IOSSafeAreaInsets safe_area_{};
    std::function<void(const IOSSafeAreaInsets&)> on_safe_area_changed_{nullptr};

    IInputDispatcher* dispatcher_{nullptr};
    ITextInputClient* text_input_client_{nullptr};
    CursorKind cursor_{CursorKind::Arrow};
    bool cursor_visible_{true};
    std::thread::id ui_thread_id_{std::this_thread::get_id()};
    NativeSurfaceHandle surface_handle_{};
    mutable std::mutex surface_mutex_;
};

class IOSPlatformHost final : public IPlatformHost {
public:
    IOSPlatformHost();
    ~IOSPlatformHost() override;

    core::Status initialize() override;
    void shutdown() override;

    [[nodiscard]] PlatformType platform_type() const noexcept override;
    [[nodiscard]] IPlatformEventPump& event_pump() override;

    [[nodiscard]] core::Result<std::unique_ptr<IWindow>> create_window(
        std::string_view title,
        const WindowSize& initial_size
    ) override;

    void request_quit() noexcept override;
    [[nodiscard]] bool is_quit_requested() const noexcept override;

    // iOS Application Lifecycle Callbacks
    void on_application_did_become_active();
    void on_application_will_resign_active();
    void on_application_did_enter_background();
    void on_application_will_enter_foreground();

    [[nodiscard]] IOSLifecycleState lifecycle_state() const noexcept;

private:
    std::atomic<bool> initialized_{false};
    std::atomic<bool> quit_requested_{false};
    std::atomic<IOSLifecycleState> lifecycle_{IOSLifecycleState::NotRunning};
    IOSEventPump event_pump_;
};

} // namespace pingdart::pal::ios
