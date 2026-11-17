// src/pal/macos/macos_platform_host.mm
// PKG-113: Native macOS Platform Host Implementation
#include <pingdart/pal/macos/macos_platform_host.h>
#include <pingdart/pal/macos/macos_window.h>
#include <pingdart/pal/macos/macos_event_pump.h>
#import <Cocoa/Cocoa.h>
#include <atomic>
#include <iostream>

@interface PingDartAppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, assign) pingdart::pal::macos::MacosPlatformHost* host;
@end

@implementation PingDartAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    (void)sender;
    return NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    (void)sender;
    if (self.host) {
        self.host->request_quit();
    }
    return NSTerminateCancel;
}

@end

namespace pingdart::pal::macos {

struct MacosPlatformHost::Impl {
    MacosEventPump event_pump;
    PingDartAppDelegate* app_delegate{nil};
    std::thread::id ui_thread_id{std::this_thread::get_id()};
    bool initialized{false};
    std::atomic<bool> quit_requested{false};

    ~Impl() {
        if (app_delegate) {
            [NSApp setDelegate:nil];
            app_delegate = nil;
        }
    }
};

MacosPlatformHost::MacosPlatformHost() : impl_(std::make_unique<Impl>()) {}
MacosPlatformHost::~MacosPlatformHost() = default;

core::Status MacosPlatformHost::initialize() {
    if (impl_->initialized) {
        return core::Status{};
    }

    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        PingDartAppDelegate* delegate = [[PingDartAppDelegate alloc] init];
        delegate.host = this;
        [NSApp setDelegate:delegate];
        impl_->app_delegate = delegate;

        [NSApp finishLaunching];
        impl_->ui_thread_id = std::this_thread::get_id();
        impl_->initialized = true;
        impl_->quit_requested = false;
    }

    return core::Status{};
}

void MacosPlatformHost::shutdown() {
    if (!impl_->initialized) return;

    impl_->quit_requested = true;
    @autoreleasepool {
        if (impl_->app_delegate) {
            [NSApp setDelegate:nil];
            impl_->app_delegate = nil;
        }
    }
    impl_->initialized = false;
}

PlatformType MacosPlatformHost::platform_type() const noexcept {
    return PlatformType::MacOS;
}

IPlatformEventPump& MacosPlatformHost::event_pump() {
    return impl_->event_pump;
}

core::Result<std::unique_ptr<IWindow>> MacosPlatformHost::create_window(
    std::string_view title,
    const WindowSize& initial_size)
{
    if (!is_on_ui_thread()) {
        return core::Status(core::StatusCode::FailedPrecondition,
                            "create_window must be invoked on the main UI thread");
    }

    auto win_res = MacosWindow::create(title, initial_size);
    if (!win_res.has_value()) {
        return win_res.status();
    }

    return std::unique_ptr<IWindow>(win_res.value().release());
}

void MacosPlatformHost::request_quit() noexcept {
    impl_->quit_requested = true;
    impl_->event_pump.wake_up();
}

bool MacosPlatformHost::is_quit_requested() const noexcept {
    return impl_->quit_requested.load(std::memory_order_relaxed);
}

bool MacosPlatformHost::is_on_ui_thread() const noexcept {
    return std::this_thread::get_id() == impl_->ui_thread_id;
}

std::thread::id MacosPlatformHost::ui_thread_id() const noexcept {
    return impl_->ui_thread_id;
}

} // namespace pingdart::pal::macos
