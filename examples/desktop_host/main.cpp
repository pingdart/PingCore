// examples/desktop_host/main.cpp
#include <pingdart/core/runtime.h>
#include <pingdart/core/event_loop.h>
#include <pingdart/pal/native_types.h>
#include <pingdart/pal/input_events.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include <pingdart/pal/i_platform_host.h>
#include <pingdart/pal/event_pump_adapter.h>
#include <iostream>

#if defined(_WIN32)
#include "win32_platform_host.h"
#endif

namespace {

class ConsoleInputDispatcher final : public pingdart::pal::IInputDispatcher {
public:
    void dispatch_pointer_event(const pingdart::pal::PointerEvent& event) override {
        if (event.type == pingdart::pal::EventType::PointerDown) {
            std::cout << "[DesktopHost] PointerDown at (" << event.x << ", " << event.y << ")" << std::endl;
        }
    }

    void dispatch_keyboard_event(const pingdart::pal::KeyboardEvent& event) override {
        if (event.type == pingdart::pal::EventType::KeyDown) {
            std::cout << "[DesktopHost] KeyDown: " << event.virtual_key << std::endl;
        }
    }

    void dispatch_lifecycle_event(const pingdart::pal::WindowLifecycleEvent& event) override {
        if (event.type == pingdart::pal::EventType::WindowResized) {
            std::cout << "[DesktopHost] WindowResized: " << event.width << "x" << event.height << std::endl;
        } else if (event.type == pingdart::pal::EventType::WindowCloseRequested) {
            std::cout << "[DesktopHost] WindowCloseRequested" << std::endl;
        }
    }
};

} // namespace

int main() {
    std::cout << "Starting PingDart Desktop Host..." << std::endl;
    auto status = pingdart::core::Runtime::initialize();
    if (!status.ok()) {
        std::cerr << "Failed to initialize PingDart: " << status.message() << std::endl;
        return 1;
    }

    std::cout << "PingDart Engine " << pingdart::core::Runtime::version() 
              << " running successfully." << std::endl;

#if defined(_WIN32)
    using namespace pingdart::pal;
    using namespace pingdart::pal::windows;

    std::cout << "[DesktopHost] Initializing Win32PlatformHost..." << std::endl;
    auto host = std::make_unique<Win32PlatformHost>();
    auto host_status = host->initialize();
    if (!host_status.ok()) {
        std::cerr << "Win32 host init failed: " << host_status.message() << std::endl;
        pingdart::core::Runtime::shutdown();
        return 1;
    }

    auto window_res = host->create_window("PingDart Native Desktop Host (Win32)", WindowSize{1280, 720});
    if (!window_res.has_value()) {
        std::cerr << "Failed to create Win32 window: " << window_res.status().message() << std::endl;
        host->shutdown();
        pingdart::core::Runtime::shutdown();
        return 1;
    }

    auto window = std::move(window_res).value();
    ConsoleInputDispatcher dispatcher;
    window->set_input_dispatcher(&dispatcher);

    window->show();
    std::cout << "[DesktopHost] Native Win32 window visible (1280x720)." << std::endl;

    auto loop = pingdart::core::EventLoop::create();
    EventPumpWakeupAdapter adapter(host->event_pump());
    loop->set_wakeup_handle(&adapter);

    constexpr size_t kMaxTasksPerBatch = 256;
    constexpr auto kMaxTaskProcessingBudget = std::chrono::milliseconds(2);

    // Run platform event pump cooperatively with runtime EventLoop
    while (!host->is_quit_requested()) {
        // Step 1: Process runnable runtime tasks with bounded execution budget to prevent starvation
        size_t tasks_processed = 0;
        const auto batch_start = std::chrono::steady_clock::now();
        while (tasks_processed < kMaxTasksPerBatch &&
               (std::chrono::steady_clock::now() - batch_start) < kMaxTaskProcessingBudget &&
               loop->run_one(std::chrono::milliseconds(0))) {
            ++tasks_processed;
        }

        if (host->is_quit_requested()) {
            break;
        }

        // Step 2: Poll native OS events without blocking
        host->event_pump().poll_events();

        if (host->is_quit_requested()) {
            break;
        }

        // Step 3: If runtime tasks remain ready, continue without sleeping!
        if (loop->has_ready_tasks()) {
            continue;
        }

        // Step 4: Compute relative timeout to next timer deadline using steady_clock
        auto deadline = loop->next_deadline().value_or(std::chrono::milliseconds(16));
        host->event_pump().wait_for_events(deadline);
    }

    std::cout << "[DesktopHost] Initiating window closure..." << std::endl;
    window->close();
    window->acknowledge_surface_retirement();
    window.reset();
    host->shutdown();
#else
    std::cout << "[DesktopHost] Non-Windows host: Core and PAL interfaces initialized." << std::endl;
#endif

    pingdart::core::Runtime::shutdown();
    std::cout << "PingDart Desktop Host exited cleanly." << std::endl;
    return 0;
}
