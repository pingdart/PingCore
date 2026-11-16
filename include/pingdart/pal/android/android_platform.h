// include/pingdart/pal/android/android_platform.h
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
#include <thread>
#include <vector>

namespace pingdart::pal::android {

// Standard Android NDK Looper & AppCmd constants (portable definitions)
enum class AndroidAppCmd : int32_t {
    InputChanged = 0,
    InitWindow = 1,
    TermWindow = 2,
    WindowResized = 3,
    WindowRedrawNeeded = 4,
    ContentRectChanged = 5,
    GainedFocus = 6,
    LostFocus = 7,
    ConfigChanged = 8,
    LowMemory = 9,
    Start = 10,
    Resume = 11,
    SaveState = 12,
    Pause = 13,
    Stop = 14,
    Destroy = 15
};

enum class AndroidMotionEventAction : int32_t {
    Down = 0,
    Up = 1,
    Move = 2,
    Cancel = 3,
    Outside = 4,
    PointerDown = 5,
    PointerUp = 6,
    HoverMove = 7,
    Scroll = 8,
    HoverEnter = 9,
    HoverExit = 10
};

enum class AndroidLifecycleState : uint8_t {
    Created,
    Started,
    Resumed,
    Paused,
    Stopped,
    Destroyed
};

struct AndroidDisplayMetrics {
    uint32_t width_pixels{1080};
    uint32_t height_pixels{2400};
    uint32_t density_dpi{440}; // ~xxhdpi
    float density_scale{2.75f};
};

class AndroidEventPump final : public IPlatformEventPump {
public:
    AndroidEventPump() = default;
    ~AndroidEventPump() override = default;

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

class AndroidWindow final : public IWindow {
public:
    AndroidWindow(std::string_view title, const WindowSize& initial_size);
    ~AndroidWindow() override;

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

    // Android NDK ANativeWindow Lifecycle & Event Ingestion
    void on_native_window_created(void* anative_window, uint32_t width, uint32_t height);
    void on_native_window_resized(uint32_t width, uint32_t height);
    void on_native_window_destroyed();

    // Generation barrier preventing GPU renders from accessing an invalidated ANativeWindow
    [[nodiscard]] bool can_render_frame(uint64_t expected_generation) const noexcept;

    void set_display_metrics(const AndroidDisplayMetrics& metrics) noexcept;
    [[nodiscard]] const AndroidDisplayMetrics& display_metrics() const noexcept;

    // Motion Event Ingestion (translates to PointerEvent / TouchEvent)
    void dispatch_motion_event(
        AndroidMotionEventAction action,
        uint32_t pointer_index,
        const std::vector<TouchPoint>& touches
    );

private:
    std::string title_;
    WindowSize size_{1080, 2400};
    WindowPoint position_{0, 0};
    bool visible_{true};
    AndroidDisplayMetrics metrics_;

    IInputDispatcher* dispatcher_{nullptr};
    ITextInputClient* text_input_client_{nullptr};
    CursorKind cursor_{CursorKind::Arrow};
    bool cursor_visible_{true};
    std::thread::id ui_thread_id_{std::this_thread::get_id()};
    NativeSurfaceHandle surface_handle_{};
    mutable std::mutex surface_mutex_;
};

class AndroidPlatformHost final : public IPlatformHost {
public:
    AndroidPlatformHost();
    ~AndroidPlatformHost() override;

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

    // Android Lifecycle & AppCmd Handlers
    void handle_app_cmd(AndroidAppCmd cmd);
    [[nodiscard]] AndroidLifecycleState lifecycle_state() const noexcept;

private:
    std::atomic<bool> initialized_{false};
    std::atomic<bool> quit_requested_{false};
    std::atomic<AndroidLifecycleState> lifecycle_{AndroidLifecycleState::Created};
    AndroidEventPump event_pump_;
};

} // namespace pingdart::pal::android
