// src/pal/linux/x11/x11_platform_host.h
#pragma once
#include <pingdart/pal/i_platform_host.h>
#include "x11_event_pump.h"
#include <memory>
#include <thread>

#if defined(__linux__)
#include <X11/Xlib.h>
#else
struct _XDisplay;
typedef struct _XDisplay Display;
#endif

namespace pingdart::pal::linux_x11 {

class X11PlatformHost final : public IPlatformHost {
public:
    X11PlatformHost();
    ~X11PlatformHost() override;

    X11PlatformHost(const X11PlatformHost&) = delete;
    X11PlatformHost& operator=(const X11PlatformHost&) = delete;

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

private:
    Display* display_{nullptr};
    std::unique_ptr<X11EventPump> event_pump_;
    std::thread::id ui_thread_id_;
    bool initialized_{false};
    bool quit_requested_{false};
};

} // namespace pingdart::pal::linux_x11
