// src/pal/macos/macos_event_pump.mm
// PKG-113: Native macOS Cocoa Event Pump Implementation
#include <pingdart/pal/macos/macos_event_pump.h>
#import <Cocoa/Cocoa.h>

namespace pingdart::pal::macos {

struct MacosEventPump::Impl {
    void poll_events() {
        @autoreleasepool {
            while (true) {
                NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                    untilDate:nil
                                                       inMode:NSDefaultRunLoopMode
                                                      dequeue:YES];
                if (!event) {
                    break;
                }
                [NSApp sendEvent:event];
                [NSApp updateWindows];
            }
        }
    }

    bool wait_for_events(std::chrono::milliseconds timeout) {
        @autoreleasepool {
            NSTimeInterval seconds = static_cast<double>(timeout.count()) / 1000.0;
            NSDate* target_date = (timeout.count() > 0)
                ? [NSDate dateWithTimeIntervalSinceNow:seconds]
                : [NSDate distantFuture];

            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                untilDate:target_date
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];
            if (event) {
                [NSApp sendEvent:event];
                [NSApp updateWindows];
                // Drain any additional events queued in this turn
                poll_events();
                return true;
            }
            return false;
        }
    }

    void wake_up() {
        @autoreleasepool {
            NSEvent* wake_event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                                     location:NSZeroPoint
                                                modifierFlags:0
                                                    timestamp:0
                                                 windowNumber:0
                                                      context:nil
                                                      subtype:0
                                                        data1:0
                                                        data2:0];
            [NSApp postEvent:wake_event atStart:NO];
        }
    }
};

MacosEventPump::MacosEventPump() : impl_(std::make_unique<Impl>()) {}
MacosEventPump::~MacosEventPump() = default;

void MacosEventPump::poll_events() {
    impl_->poll_events();
}

bool MacosEventPump::wait_for_events(std::chrono::milliseconds timeout) {
    return impl_->wait_for_events(timeout);
}

void MacosEventPump::wake_up() {
    impl_->wake_up();
}

} // namespace pingdart::pal::macos
