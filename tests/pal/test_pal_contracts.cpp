// tests/pal/test_pal_contracts.cpp
#include <gtest/gtest.h>
#include <pingdart/pal/native_types.h>
#include <pingdart/pal/input_events.h>
#include <pingdart/pal/i_window.h>
#include <pingdart/pal/i_platform_host.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include <pingdart/pal/event_pump_adapter.h>
#include <pingdart/core/event_loop.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace pingdart::pal;
using namespace pingdart::core;
using namespace std::chrono_literals;

namespace {

class MockInputDispatcher final : public IInputDispatcher {
public:
    std::vector<PointerEvent> pointer_events;
    std::vector<KeyboardEvent> keyboard_events;
    std::vector<WindowLifecycleEvent> lifecycle_events;

    void dispatch_pointer_event(const PointerEvent& event) override {
        pointer_events.push_back(event);
    }
    void dispatch_keyboard_event(const KeyboardEvent& event) override {
        keyboard_events.push_back(event);
    }
    void dispatch_lifecycle_event(const WindowLifecycleEvent& event) override {
        lifecycle_events.push_back(event);
    }
};

class MockWindow final : public IWindow {
public:
    MockWindow(std::string title, WindowSize size, std::thread::id ui_thread_id)
        : title_(std::move(title)), size_(size), ui_thread_id_(ui_thread_id) {
        surface_handle_.kind = SurfaceKind::Mock;
        surface_handle_.token = MockSurfaceDescriptor{1001};
        surface_handle_.generation_id = 1;
        surface_handle_.lifecycle_state = SurfaceLifecycleState::Active;
        surface_handle_.is_valid = true;
    }

    ~MockWindow() override {
        surface_handle_.is_valid = false;
        surface_handle_.lifecycle_state = SurfaceLifecycleState::Destroyed;
    }

    // ISurfaceProvider implementation
    [[nodiscard]] NativeSurfaceHandle native_surface_handle() const noexcept override {
        return surface_handle_;
    }

    [[nodiscard]] uint64_t surface_generation_id() const noexcept override {
        return surface_handle_.surface_generation_id;
    }

    [[nodiscard]] uint64_t swapchain_generation_id() const noexcept override {
        return surface_handle_.swapchain_generation_id;
    }

    [[nodiscard]] WindowSize physical_size() const noexcept override {
        return size_;
    }

    [[nodiscard]] float scale_factor() const noexcept override {
        return scale_factor_;
    }

    [[nodiscard]] SurfaceLifecycleState surface_lifecycle_state() const noexcept override {
        return surface_handle_.lifecycle_state;
    }

    void mark_surface_retiring() noexcept override {
        surface_handle_.lifecycle_state = SurfaceLifecycleState::Retiring;
    }

    void acknowledge_surface_retirement() noexcept override {
        surface_handle_.lifecycle_state = SurfaceLifecycleState::Retired;
    }

    [[nodiscard]] bool is_surface_ready_for_destruction() const noexcept override {
        return surface_handle_.lifecycle_state == SurfaceLifecycleState::Retired;
    }

    void force_surface_retirement_timeout() noexcept override {
        surface_handle_.lifecycle_state = SurfaceLifecycleState::Abandoned;
    }

    // IWindow implementation
    Status show() override {
        if (!is_on_ui_thread()) {
            return Status::FailedPrecondition("Window mutations must occur on UI thread");
        }
        visible_ = true;
        return Status::Ok();
    }

    Status hide() override {
        if (!is_on_ui_thread()) {
            return Status::FailedPrecondition("Window mutations must occur on UI thread");
        }
        visible_ = false;
        return Status::Ok();
    }

    Status close() override {
        if (!is_on_ui_thread()) {
            return Status::FailedPrecondition("Window mutations must occur on UI thread");
        }
        mark_surface_retiring();
        visible_ = false;
        if (dispatcher_) {
            dispatcher_->dispatch_lifecycle_event(WindowLifecycleEvent{
                EventType::WindowCloseRequested,
                size_.width,
                size_.height,
                scale_factor_,
                0
            });
        }
        return Status::Ok();
    }

    Status set_title(std::string_view title) override {
        if (!is_on_ui_thread()) {
            return Status::FailedPrecondition("Window mutations must occur on UI thread");
        }
        title_ = title;
        return Status::Ok();
    }

    Status set_size(const WindowSize& size) override {
        if (!is_on_ui_thread()) {
            return Status::FailedPrecondition("Window mutations must occur on UI thread");
        }
        size_ = size;
        surface_handle_.swapchain_generation_id++;
        if (dispatcher_) {
            dispatcher_->dispatch_lifecycle_event(WindowLifecycleEvent{
                EventType::WindowResized,
                size.width,
                size.height,
                scale_factor_,
                0
            });
        }
        return Status::Ok();
    }

    Status set_position(const WindowPoint& position) override {
        if (!is_on_ui_thread()) {
            return Status::FailedPrecondition("Window mutations must occur on UI thread");
        }
        position_ = position;
        return Status::Ok();
    }

    [[nodiscard]] WindowSize size() const noexcept override { return size_; }
    [[nodiscard]] WindowPoint position() const noexcept override { return position_; }
    [[nodiscard]] bool is_visible() const noexcept override { return visible_; }
    [[nodiscard]] bool is_minimized() const noexcept override { return false; }
    [[nodiscard]] bool is_maximized() const noexcept override { return false; }

    void set_input_dispatcher(IInputDispatcher* dispatcher) noexcept override {
        dispatcher_ = dispatcher;
    }

    [[nodiscard]] IInputDispatcher* input_dispatcher() const noexcept override {
        return dispatcher_;
    }

    Status set_cursor(CursorKind cursor) override {
        if (!is_on_ui_thread()) {
            return Status::FailedPrecondition("Cursor mutations must occur on UI thread");
        }
        cursor_ = cursor;
        return Status::Ok();
    }

    [[nodiscard]] CursorKind cursor() const noexcept override {
        return cursor_;
    }

    Status set_cursor_visible(bool visible) override {
        if (!is_on_ui_thread()) {
            return Status::FailedPrecondition("Cursor mutations must occur on UI thread");
        }
        cursor_visible_ = visible;
        return Status::Ok();
    }

    void set_text_input_client(ITextInputClient* client) noexcept override {
        text_client_ = client;
    }

    [[nodiscard]] ITextInputClient* text_input_client() const noexcept override {
        return text_client_;
    }

    void update_ime_caret_rect(const WindowPoint& position, const WindowSize& size) override {
        caret_pos_ = position;
        caret_size_ = size;
    }

    [[nodiscard]] bool is_on_ui_thread() const noexcept override {
        return std::this_thread::get_id() == ui_thread_id_;
    }

private:
    std::string title_;
    WindowSize size_{0, 0};
    WindowPoint position_{0, 0};
    float scale_factor_{1.0f};
    bool visible_{false};
    CursorKind cursor_{CursorKind::Arrow};
    bool cursor_visible_{true};
    ITextInputClient* text_client_{nullptr};
    WindowPoint caret_pos_{0, 0};
    WindowSize caret_size_{0, 0};
    NativeSurfaceHandle surface_handle_;
    IInputDispatcher* dispatcher_{nullptr};
    std::thread::id ui_thread_id_;
};

class MockEventPump final : public IPlatformEventPump {
public:
    std::atomic<int> wake_ups{0};
    std::atomic<int> poll_calls{0};
    std::atomic<int> wait_calls{0};

    void poll_events() override {
        poll_calls.fetch_add(1);
    }

    bool wait_for_events(std::chrono::milliseconds timeout) override {
        (void)timeout;
        wait_calls.fetch_add(1);
        return true;
    }

    void wake_up() override {
        wake_ups.fetch_add(1);
    }
};

class MockPlatformHost final : public IPlatformHost {
public:
    MockPlatformHost() : ui_thread_id_(std::this_thread::get_id()) {}

    Status initialize() override {
        initialized_ = true;
        return Status::Ok();
    }

    void shutdown() override {
        initialized_ = false;
    }

    [[nodiscard]] PlatformType platform_type() const noexcept override {
        return PlatformType::Mock;
    }

    [[nodiscard]] IPlatformEventPump& event_pump() override {
        return pump_;
    }

    [[nodiscard]] Result<std::unique_ptr<IWindow>> create_window(
        std::string_view title,
        const WindowSize& initial_size
    ) override {
        if (!initialized_) {
            return Status::FailedPrecondition("Platform host is not initialized");
        }
        if (std::this_thread::get_id() != ui_thread_id_) {
            return Status::FailedPrecondition("create_window must run on the UI thread");
        }
        return std::make_unique<MockWindow>(std::string(title), initial_size, ui_thread_id_);
    }

    void request_quit() noexcept override {
        quit_requested_ = true;
    }

    [[nodiscard]] bool is_quit_requested() const noexcept override {
        return quit_requested_;
    }

private:
    std::thread::id ui_thread_id_;
    MockEventPump pump_;
    bool initialized_{false};
    bool quit_requested_{false};
};

} // namespace

TEST(PalContractsTest, NativeSurfaceHandleDiscrimination) {
    NativeSurfaceHandle win32_surface;
    win32_surface.kind = SurfaceKind::Win32;
    win32_surface.token = Win32SurfaceDescriptor{reinterpret_cast<void*>(0x1234), nullptr};
    EXPECT_TRUE(std::holds_alternative<Win32SurfaceDescriptor>(win32_surface.token));
    EXPECT_EQ(win32_surface.kind, SurfaceKind::Win32);

    NativeSurfaceHandle wayland_surface;
    wayland_surface.kind = SurfaceKind::Wayland;
    wayland_surface.token = WaylandSurfaceDescriptor{reinterpret_cast<void*>(0x5678), nullptr};
    EXPECT_TRUE(std::holds_alternative<WaylandSurfaceDescriptor>(wayland_surface.token));
    EXPECT_EQ(wayland_surface.kind, SurfaceKind::Wayland);

    // Verify integer XID representation without unchecked void* conversions
    NativeSurfaceHandle x11_surface;
    x11_surface.kind = SurfaceKind::X11;
    x11_surface.token = X11SurfaceDescriptor{reinterpret_cast<void*>(0x9999), 0xABCDEF01ULL, nullptr};
    EXPECT_TRUE(std::holds_alternative<X11SurfaceDescriptor>(x11_surface.token));
    EXPECT_EQ(std::get<X11SurfaceDescriptor>(x11_surface.token).window, 0xABCDEF01ULL);
}

TEST(PalContractsTest, SurfaceGenerationIdIncrementsOnResize) {
    MockPlatformHost host;
    ASSERT_TRUE(host.initialize().ok());

    auto window_res = host.create_window("Test Window", WindowSize{800, 600});
    ASSERT_TRUE(window_res.has_value());
    auto window = std::move(window_res).value();

    EXPECT_EQ(window->surface_generation_id(), 1u);
    EXPECT_EQ(window->swapchain_generation_id(), 1u);
    auto initial_surface = window->native_surface_handle();
    EXPECT_TRUE(initial_surface.is_usable_for_rendering());

    // Resize window: swapchain generation increments, surface generation remains stable
    ASSERT_TRUE(window->set_size(WindowSize{1024, 768}).ok());
    EXPECT_EQ(window->surface_generation_id(), 1u);
    EXPECT_EQ(window->swapchain_generation_id(), 2u);

    // Initial swapchain handle generation is now stale
    EXPECT_LT(initial_surface.swapchain_generation_id, window->swapchain_generation_id());

    auto current_surface = window->native_surface_handle();
    EXPECT_EQ(current_surface.swapchain_generation_id, 2u);
}

TEST(PalContractsTest, UIThreadAffinityEnforcement) {
    MockPlatformHost host;
    ASSERT_TRUE(host.initialize().ok());

    auto window_res = host.create_window("Affinity Window", WindowSize{640, 480});
    ASSERT_TRUE(window_res.has_value());
    auto window = std::move(window_res).value();

    // UI thread operations succeed
    EXPECT_TRUE(window->show().ok());
    EXPECT_TRUE(window->is_visible());
    EXPECT_TRUE(window->set_size(WindowSize{800, 600}).ok());

    // Worker thread mutation attempts are rejected
    std::thread worker([&]() {
        EXPECT_EQ(window->show().code(), StatusCode::FailedPrecondition);
        EXPECT_EQ(window->hide().code(), StatusCode::FailedPrecondition);
        EXPECT_EQ(window->set_size(WindowSize{1280, 720}).code(), StatusCode::FailedPrecondition);
        EXPECT_EQ(window->close().code(), StatusCode::FailedPrecondition);
    });
    worker.join();
}

TEST(PalContractsTest, MultiPhaseSurfaceRetirementProtocol) {
    MockPlatformHost host;
    ASSERT_TRUE(host.initialize().ok());

    auto window_res = host.create_window("Lifecycle Window", WindowSize{800, 600});
    ASSERT_TRUE(window_res.has_value());
    auto window = std::move(window_res).value();

    EXPECT_EQ(window->surface_lifecycle_state(), SurfaceLifecycleState::Active);
    EXPECT_TRUE(window->native_surface_handle().is_usable_for_rendering());
    EXPECT_FALSE(window->is_surface_ready_for_destruction());

    // Phase 1: UI thread requests closure
    ASSERT_TRUE(window->close().ok());
    EXPECT_EQ(window->surface_lifecycle_state(), SurfaceLifecycleState::Retiring);
    EXPECT_FALSE(window->native_surface_handle().is_usable_for_rendering());
    EXPECT_FALSE(window->is_surface_ready_for_destruction());

    // Phase 2: Render thread acknowledges retirement after releasing GPU resources
    std::thread render_thread([&]() {
        // Render thread detects retiring state, completes pending presentations, releases resources
        auto surface = window->native_surface_handle();
        EXPECT_FALSE(surface.is_usable_for_rendering());
        window->acknowledge_surface_retirement();
    });
    render_thread.join();

    // Phase 3: UI thread confirms retirement is complete and safely destroys window
    EXPECT_EQ(window->surface_lifecycle_state(), SurfaceLifecycleState::Retired);
    EXPECT_TRUE(window->is_surface_ready_for_destruction());

    // Window destruction
    window.reset();
    // After destruction, no in-flight render thread can touch native handles
}

TEST(PalContractsTest, FailSafeSurfaceRetirementOnGpuCrash) {
    MockPlatformHost host;
    ASSERT_TRUE(host.initialize().ok());

    auto window_res = host.create_window("Crash Window", WindowSize{800, 600});
    ASSERT_TRUE(window_res.has_value());
    auto window = std::move(window_res).value();

    // UI initiates closure
    ASSERT_TRUE(window->close().ok());
    EXPECT_EQ(window->surface_lifecycle_state(), SurfaceLifecycleState::Retiring);
    EXPECT_FALSE(window->is_surface_ready_for_destruction());

    // GPU device is lost or render thread hangs and never ACKs.
    // UI thread triggers timeout override to prevent UI hang.
    window->force_surface_retirement_timeout();
    EXPECT_EQ(window->surface_lifecycle_state(), SurfaceLifecycleState::Abandoned);
    EXPECT_FALSE(window->native_surface_handle().is_usable_for_rendering());

    // Window can now be safely destroyed
    window.reset();
}

TEST(PalContractsTest, EventPumpWakeupAndDeadlineIntegration) {
    MockPlatformHost host;
    ASSERT_TRUE(host.initialize().ok());

    EventPumpWakeupAdapter adapter(host.event_pump());
    auto loop = EventLoop::create();
    loop->set_wakeup_handle(&adapter);

    auto& pump = static_cast<MockEventPump&>(host.event_pump());
    EXPECT_EQ(pump.wake_ups.load(), 0);

    // Posting a task triggers event pump wake_up
    loop->post_task([]() {});
    EXPECT_EQ(pump.wake_ups.load(), 1);

    // Delayed task deadline informs platform wait timeout
    loop->post_delayed_task([]() {}, 50ms);
    EXPECT_EQ(pump.wake_ups.load(), 2);
    auto deadline = loop->next_deadline();
    ASSERT_TRUE(deadline.has_value());
    EXPECT_GT(deadline.value().count(), 0);

    // Platform host pumps events with computed timeout
    pump.wait_for_events(deadline.value());
    EXPECT_EQ(pump.wait_calls.load(), 1);
}

TEST(PalContractsTest, FullMockPlatformHostLifecycle) {
    MockPlatformHost host;
    EXPECT_EQ(host.platform_type(), PlatformType::Mock);
    ASSERT_TRUE(host.initialize().ok());

    MockInputDispatcher dispatcher;
    auto window_res = host.create_window("App Main", WindowSize{1280, 720});
    ASSERT_TRUE(window_res.has_value());
    auto window = std::move(window_res).value();
    window->set_input_dispatcher(&dispatcher);

    ASSERT_TRUE(window->show().ok());
    ASSERT_TRUE(window->set_size(WindowSize{1920, 1080}).ok());

    // Verify lifecycle event dispatch
    ASSERT_EQ(dispatcher.lifecycle_events.size(), 1u);
    EXPECT_EQ(dispatcher.lifecycle_events[0].type, EventType::WindowResized);
    EXPECT_EQ(dispatcher.lifecycle_events[0].width, 1920u);

    // Dispatch simulated pointer event
    dispatcher.dispatch_pointer_event(PointerEvent{
        EventType::PointerDown,
        0,
        150.0,
        200.0,
        0.0,
        0.0,
        PointerButton::Left,
        static_cast<uint8_t>(PointerButton::Left),
        ModifierKey::None,
        1.0f,
        1000
    });
    EXPECT_EQ(dispatcher.pointer_events.size(), 1u);

    // Close window
    ASSERT_TRUE(window->close().ok());
    EXPECT_EQ(dispatcher.lifecycle_events.size(), 2u);
    EXPECT_EQ(dispatcher.lifecycle_events[1].type, EventType::WindowCloseRequested);

    // Request quit
    EXPECT_FALSE(host.is_quit_requested());
    host.request_quit();
    EXPECT_TRUE(host.is_quit_requested());

    host.shutdown();
}
