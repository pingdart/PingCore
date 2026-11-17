// src/pal/macos/macos_window.mm
// PKG-113: Native macOS Cocoa Window & CAMetalLayer Implementation
#include <pingdart/pal/macos/macos_window.h>
#include <pingdart/pal/macos/macos_input_dispatcher.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <chrono>
#include <thread>
#include <iostream>

@interface PingDartWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) pingdart::pal::macos::MacosWindow* owner;
@end

@interface PingDartMetalView : NSView <NSTextInputClient>
@property (nonatomic, assign) pingdart::pal::macos::MacosWindow* owner;
@property (nonatomic, strong) CAMetalLayer* metalLayer;
@property (nonatomic, strong) NSTrackingArea* trackingArea;
@property (nonatomic, strong) NSMutableAttributedString* markedString;
@property (nonatomic, assign) NSRange selectedRangeInternal;
@property (nonatomic, assign) NSRange markedRangeInternal;
@end

@implementation PingDartWindowDelegate

- (void)windowDidResize:(NSNotification *)notification {
    if (!self.owner) return;
    NSWindow* window = [notification object];
    NSView* view = [window contentView];
    CGFloat scale = [window backingScaleFactor];
    NSSize size = [view bounds].size;
    uint32_t width = static_cast<uint32_t>(size.width * scale);
    uint32_t height = static_cast<uint32_t>(size.height * scale);
    self.owner->on_native_resized(width, height, static_cast<float>(scale));
}

- (void)windowDidMove:(NSNotification *)notification {
    if (!self.owner) return;
    NSWindow* window = [notification object];
    NSRect frame = [window frame];
    self.owner->on_native_moved(static_cast<int32_t>(frame.origin.x), static_cast<int32_t>(frame.origin.y));
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
    (void)notification;
    if (self.owner) {
        self.owner->on_native_focus_changed(true);
    }
}

- (void)windowDidResignKey:(NSNotification *)notification {
    (void)notification;
    if (self.owner) {
        self.owner->on_native_focus_changed(false);
    }
}

- (BOOL)windowShouldClose:(NSWindow *)sender {
    (void)sender;
    if (self.owner) {
        self.owner->on_native_close_requested();
    }
    return NO; // Let PingDart manage destruction cooperative lifecycle
}

@end

@implementation PingDartMetalView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self setWantsLayer:YES];
        self.metalLayer = [CAMetalLayer layer];
        self.metalLayer.device = MTLCreateSystemDefaultDevice();
        self.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        self.metalLayer.framebufferOnly = YES;
        self.metalLayer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
        [self setLayer:self.metalLayer];
        [self setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawDuringViewResize];

        self.markedString = [[NSMutableAttributedString alloc] init];
        self.selectedRangeInternal = NSMakeRange(NSNotFound, 0);
        self.markedRangeInternal = NSMakeRange(NSNotFound, 0);

        [self updateTrackingAreas];
    }
    return self;
}

- (CALayer *)makeBackingLayer {
    return [CAMetalLayer layer];
}

- (BOOL)wantsUpdateLayer {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    (void)event;
    return YES;
}

- (void)updateTrackingAreas {
    if (self.trackingArea) {
        [self removeTrackingArea:self.trackingArea];
    }
    NSTrackingAreaOptions options = NSTrackingMouseMoved |
                                    NSTrackingActiveInKeyWindow |
                                    NSTrackingInVisibleRect |
                                    NSTrackingMouseEnteredAndExited;
    self.trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                     options:options
                                                       owner:self
                                                    userInfo:nil];
    [self addTrackingArea:self.trackingArea];
    [super updateTrackingAreas];
}

// Coordinate translation: Cocoa bottom-left to PingDart top-left
- (NSPoint)translatePoint:(NSPoint)windowLocation {
    NSPoint viewPoint = [self convertPoint:windowLocation fromView:nil];
    viewPoint.y = self.bounds.size.height - viewPoint.y;
    return viewPoint;
}

- (uint8_t)activeButtonsMask {
    uint8_t mask = 0;
    NSUInteger pressed = [NSEvent pressedMouseButtons];
    if (pressed & (1 << 0)) mask |= static_cast<uint8_t>(pingdart::pal::PointerButton::Left);
    if (pressed & (1 << 1)) mask |= static_cast<uint8_t>(pingdart::pal::PointerButton::Right);
    if (pressed & (1 << 2)) mask |= static_cast<uint8_t>(pingdart::pal::PointerButton::Middle);
    return mask;
}

- (void)dispatchPointer:(NSEvent *)event type:(pingdart::pal::EventType)type {
    if (!self.owner) return;
    auto* dispatcher = self.owner->input_dispatcher();
    if (!dispatcher) return;

    NSPoint pt = [self translatePoint:[event locationInWindow]];
    pingdart::pal::PointerEvent pe;
    pe.type = type;
    pe.pointer_id = 0;
    pe.x = pt.x;
    pe.y = pt.y;
    pe.delta_x = [event scrollingDeltaX];
    pe.delta_y = [event scrollingDeltaY];
    pe.button = pingdart::pal::macos::MacosInputTranslator::translate_button([event buttonNumber]);
    pe.buttons_down = [self activeButtonsMask];
    pe.modifiers = pingdart::pal::macos::MacosInputTranslator::translate_modifiers([event modifierFlags]);
    pe.pressure = static_cast<float>([event pressure]);
    pe.timestamp_us = static_cast<uint64_t>([event timestamp] * 1000000.0);

    dispatcher->dispatch_pointer_event(pe);
}

- (void)mouseDown:(NSEvent *)event         { [self dispatchPointer:event type:pingdart::pal::EventType::PointerDown]; }
- (void)mouseUp:(NSEvent *)event           { [self dispatchPointer:event type:pingdart::pal::EventType::PointerUp]; }
- (void)rightMouseDown:(NSEvent *)event    { [self dispatchPointer:event type:pingdart::pal::EventType::PointerDown]; }
- (void)rightMouseUp:(NSEvent *)event      { [self dispatchPointer:event type:pingdart::pal::EventType::PointerUp]; }
- (void)otherMouseDown:(NSEvent *)event    { [self dispatchPointer:event type:pingdart::pal::EventType::PointerDown]; }
- (void)otherMouseUp:(NSEvent *)event      { [self dispatchPointer:event type:pingdart::pal::EventType::PointerUp]; }
- (void)mouseMoved:(NSEvent *)event        { [self dispatchPointer:event type:pingdart::pal::EventType::PointerMove]; }
- (void)mouseDragged:(NSEvent *)event      { [self dispatchPointer:event type:pingdart::pal::EventType::PointerMove]; }
- (void)rightMouseDragged:(NSEvent *)event { [self dispatchPointer:event type:pingdart::pal::EventType::PointerMove]; }
- (void)otherMouseDragged:(NSEvent *)event { [self dispatchPointer:event type:pingdart::pal::EventType::PointerMove]; }
- (void)mouseEntered:(NSEvent *)event      { [self dispatchPointer:event type:pingdart::pal::EventType::PointerEnter]; }
- (void)mouseExited:(NSEvent *)event       { [self dispatchPointer:event type:pingdart::pal::EventType::PointerLeave]; }

- (void)scrollWheel:(NSEvent *)event {
    if (!self.owner) return;
    auto* dispatcher = self.owner->input_dispatcher();
    if (!dispatcher) return;

    NSPoint pt = [self translatePoint:[event locationInWindow]];
    pingdart::pal::PointerEvent pe;
    pe.type = pingdart::pal::EventType::PointerWheel;
    pe.pointer_id = 0;
    pe.x = pt.x;
    pe.y = pt.y;
    pe.delta_x = [event hasPreciseScrollingDeltas] ? [event scrollingDeltaX] : [event deltaX] * 12.0;
    pe.delta_y = [event hasPreciseScrollingDeltas] ? [event scrollingDeltaY] : [event deltaY] * 12.0;
    pe.button = pingdart::pal::PointerButton::None;
    pe.buttons_down = [self activeButtonsMask];
    pe.modifiers = pingdart::pal::macos::MacosInputTranslator::translate_modifiers([event modifierFlags]);
    pe.timestamp_us = static_cast<uint64_t>([event timestamp] * 1000000.0);

    dispatcher->dispatch_pointer_event(pe);
}

// Keyboard handling & NSTextInputClient integration
- (void)keyDown:(NSEvent *)event {
    if (!self.owner) return;
    auto* dispatcher = self.owner->input_dispatcher();

    if (dispatcher) {
        pingdart::pal::KeyboardEvent ke;
        ke.type = pingdart::pal::EventType::KeyDown;
        ke.key_code = [event keyCode];
        ke.virtual_key = pingdart::pal::macos::MacosInputTranslator::translate_virtual_key([event keyCode]);
        ke.modifiers = pingdart::pal::macos::MacosInputTranslator::translate_modifiers([event modifierFlags]);
        ke.is_repeat = [event isARepeat];
        ke.timestamp_us = static_cast<uint64_t>([event timestamp] * 1000000.0);
        dispatcher->dispatch_keyboard_event(ke);
    }

    // Drive NSTextInputClient for Unicode text shaping, dead keys, and composition
    [self interpretKeyEvents:@[event]];
}

- (void)keyUp:(NSEvent *)event {
    if (!self.owner) return;
    auto* dispatcher = self.owner->input_dispatcher();
    if (dispatcher) {
        pingdart::pal::KeyboardEvent ke;
        ke.type = pingdart::pal::EventType::KeyUp;
        ke.key_code = [event keyCode];
        ke.virtual_key = pingdart::pal::macos::MacosInputTranslator::translate_virtual_key([event keyCode]);
        ke.modifiers = pingdart::pal::macos::MacosInputTranslator::translate_modifiers([event modifierFlags]);
        ke.is_repeat = false;
        ke.timestamp_us = static_cast<uint64_t>([event timestamp] * 1000000.0);
        dispatcher->dispatch_keyboard_event(ke);
    }
}

// NSTextInputClient implementation
- (BOOL)hasMarkedText {
    return self.markedRangeInternal.location != NSNotFound;
}

- (NSRange)markedRange {
    return self.markedRangeInternal;
}

- (NSRange)selectedRange {
    return self.selectedRangeInternal;
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
    (void)replacementRange;
    NSString* text = [string isKindOfClass:[NSAttributedString class]] ? [(NSAttributedString *)string string] : string;
    [self.markedString setAttributedString:[[NSAttributedString alloc] initWithString:text]];
    self.markedRangeInternal = NSMakeRange(0, [text length]);
    self.selectedRangeInternal = selectedRange;

    if (self.owner) {
        pingdart::pal::TextInputEvent te;
        te.type = pingdart::pal::TextEventType::CompositionUpdate;
        te.utf8_text = [text UTF8String];
        te.selection_start = static_cast<uint32_t>(selectedRange.location);
        te.selection_end = static_cast<uint32_t>(selectedRange.location + selectedRange.length);

        if (self.owner->text_input_client()) {
            self.owner->text_input_client()->on_text_input(te);
        }
        if (self.owner->input_dispatcher()) {
            self.owner->input_dispatcher()->dispatch_text_event(te);
        }
    }
}

- (void)unmarkText {
    [self.markedString deleteCharactersInRange:NSMakeRange(0, [self.markedString length])];
    self.markedRangeInternal = NSMakeRange(NSNotFound, 0);

    if (self.owner) {
        pingdart::pal::TextInputEvent te;
        te.type = pingdart::pal::TextEventType::CompositionEnd;
        if (self.owner->text_input_client()) {
            self.owner->text_input_client()->on_text_input(te);
        }
        if (self.owner->input_dispatcher()) {
            self.owner->input_dispatcher()->dispatch_text_event(te);
        }
    }
}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {
    return @[];
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    if (actualRange) *actualRange = range;
    return [[NSAttributedString alloc] initWithString:@""];
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    (void)replacementRange;
    NSString* text = [string isKindOfClass:[NSAttributedString class]] ? [(NSAttributedString *)string string] : string;
    [self unmarkText];

    if (self.owner) {
        pingdart::pal::TextInputEvent te;
        te.type = pingdart::pal::TextEventType::CompositionCommit;
        te.utf8_text = [text UTF8String];
        te.selection_start = static_cast<uint32_t>([text length]);
        te.selection_end = static_cast<uint32_t>([text length]);

        if (self.owner->text_input_client()) {
            self.owner->text_input_client()->on_text_input(te);
        }
        if (self.owner->input_dispatcher()) {
            self.owner->input_dispatcher()->dispatch_text_event(te);
        }
    }
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    (void)point;
    return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    if (actualRange) *actualRange = range;
    return [self.window convertRectToScreen:self.bounds];
}

- (void)doCommandBySelector:(SEL)selector {
    (void)selector;
}

@end

// ============================================================================
// MacosWindow C++ Bridge Implementation
// ============================================================================

namespace pingdart::pal::macos {

struct MacosWindow::Impl {
    NSWindow* window{nil};
    PingDartMetalView* view{nil};
    PingDartWindowDelegate* delegate{nil};

    WindowSize size_{1280, 720};
    WindowPoint position_{100, 100};
    float scale_factor_{1.0f};

    uint64_t surface_generation_id_{1};
    uint64_t swapchain_generation_id_{1};
    SurfaceLifecycleState lifecycle_state_{SurfaceLifecycleState::Active};

    IInputDispatcher* input_dispatcher_{nullptr};
    ITextInputClient* text_input_client_{nullptr};
    CursorKind cursor_{CursorKind::Arrow};
    std::thread::id ui_thread_id_{std::this_thread::get_id()};

    ~Impl() {
        @autoreleasepool {
            if (delegate) {
                delegate.owner = nullptr;
            }
            if (view) {
                view.owner = nullptr;
            }
            if (window) {
                [window setDelegate:nil];
                [window setContentView:nil];
                [window close];
                window = nil;
            }
        }
    }
};

MacosWindow::MacosWindow(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

MacosWindow::~MacosWindow() {
    impl_->lifecycle_state_ = SurfaceLifecycleState::Destroyed;
}

core::Result<std::unique_ptr<MacosWindow>> MacosWindow::create(
    std::string_view title,
    const WindowSize& initial_size)
{
    @autoreleasepool {
        NSRect content_rect = NSMakeRect(100, 100, initial_size.width, initial_size.height);
        NSWindowStyleMask style_mask = NSWindowStyleMaskTitled |
                                       NSWindowStyleMaskClosable |
                                       NSWindowStyleMaskMiniaturizable |
                                       NSWindowStyleMaskResizable;

        NSWindow* ns_win = [[NSWindow alloc] initWithContentRect:content_rect
                                                       styleMask:style_mask
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
        if (!ns_win) {
            return core::Status(core::StatusCode::Internal, "Failed to create Cocoa NSWindow");
        }

        [ns_win setReleasedWhenClosed:NO];

        NSString* ns_title = [NSString stringWithUTF8String:std::string(title).c_str()];
        [ns_win setTitle:ns_title];
        [ns_win setAcceptsMouseMovedEvents:YES];

        PingDartMetalView* view = [[PingDartMetalView alloc] initWithFrame:content_rect];
        [ns_win setContentView:view];
        [ns_win makeFirstResponder:view];

        PingDartWindowDelegate* delegate = [[PingDartWindowDelegate alloc] init];
        [ns_win setDelegate:delegate];

        CGFloat scale = [ns_win backingScaleFactor];
        auto impl = std::make_unique<Impl>();
        impl->window = ns_win;
        impl->view = view;
        impl->delegate = delegate;
        impl->size_ = initial_size;
        impl->scale_factor_ = static_cast<float>(scale);

        auto win = std::unique_ptr<MacosWindow>(new MacosWindow(std::move(impl)));
        delegate.owner = win.get();
        view.owner = win.get();

        return win;
    }
}

NativeSurfaceHandle MacosWindow::native_surface_handle() const noexcept {
    NativeSurfaceHandle handle;
    handle.kind = SurfaceKind::Metal;
    handle.token = MetalSurfaceDescriptor{
        .layer = (__bridge void*)impl_->view.metalLayer
    };
    handle.surface_generation_id = impl_->surface_generation_id_;
    handle.swapchain_generation_id = impl_->swapchain_generation_id_;
    handle.generation_id = impl_->surface_generation_id_;
    handle.lifecycle_state = impl_->lifecycle_state_;
    handle.is_valid = (impl_->window != nil) && (impl_->lifecycle_state_ != SurfaceLifecycleState::Destroyed);
    return handle;
}

uint64_t MacosWindow::surface_generation_id() const noexcept {
    return impl_->surface_generation_id_;
}

uint64_t MacosWindow::swapchain_generation_id() const noexcept {
    return impl_->swapchain_generation_id_;
}

WindowSize MacosWindow::physical_size() const noexcept {
    return WindowSize{
        static_cast<uint32_t>(impl_->size_.width * impl_->scale_factor_),
        static_cast<uint32_t>(impl_->size_.height * impl_->scale_factor_)
    };
}

float MacosWindow::scale_factor() const noexcept {
    return impl_->scale_factor_;
}

SurfaceLifecycleState MacosWindow::surface_lifecycle_state() const noexcept {
    return impl_->lifecycle_state_;
}

void MacosWindow::mark_surface_retiring() noexcept {
    impl_->lifecycle_state_ = SurfaceLifecycleState::Retiring;
}

void MacosWindow::acknowledge_surface_retirement() noexcept {
    impl_->lifecycle_state_ = SurfaceLifecycleState::Retired;
}

bool MacosWindow::is_surface_ready_for_destruction() const noexcept {
    return impl_->lifecycle_state_ == SurfaceLifecycleState::Retired;
}

void MacosWindow::force_surface_retirement_timeout() noexcept {
    impl_->lifecycle_state_ = SurfaceLifecycleState::Abandoned;
}

core::Status MacosWindow::show() {
    if (!is_on_ui_thread()) {
        return core::Status(core::StatusCode::FailedPrecondition, "Window mutations must occur on UI thread");
    }
    @autoreleasepool {
        [impl_->window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
    }
    return core::Status{};
}

core::Status MacosWindow::hide() {
    if (!is_on_ui_thread()) {
        return core::Status(core::StatusCode::FailedPrecondition, "Window mutations must occur on UI thread");
    }
    @autoreleasepool {
        [impl_->window orderOut:nil];
    }
    return core::Status{};
}

core::Status MacosWindow::close() {
    if (!is_on_ui_thread()) {
        return core::Status(core::StatusCode::FailedPrecondition, "Window mutations must occur on UI thread");
    }
    mark_surface_retiring();
    @autoreleasepool {
        [impl_->window close];
    }
    return core::Status{};
}

core::Status MacosWindow::set_title(std::string_view title) {
    if (!is_on_ui_thread()) {
        return core::Status(core::StatusCode::FailedPrecondition, "Window mutations must occur on UI thread");
    }
    @autoreleasepool {
        [impl_->window setTitle:[NSString stringWithUTF8String:std::string(title).c_str()]];
    }
    return core::Status{};
}

core::Status MacosWindow::set_size(const WindowSize& size) {
    if (!is_on_ui_thread()) {
        return core::Status(core::StatusCode::FailedPrecondition, "Window mutations must occur on UI thread");
    }
    @autoreleasepool {
        NSRect frame = [impl_->window frame];
        frame.size = NSMakeSize(size.width, size.height);
        [impl_->window setFrame:frame display:YES animate:NO];
        impl_->size_ = size;
        impl_->swapchain_generation_id_++;
    }
    return core::Status{};
}

core::Status MacosWindow::set_position(const WindowPoint& position) {
    if (!is_on_ui_thread()) {
        return core::Status(core::StatusCode::FailedPrecondition, "Window mutations must occur on UI thread");
    }
    @autoreleasepool {
        [impl_->window setFrameOrigin:NSMakePoint(position.x, position.y)];
        impl_->position_ = position;
    }
    return core::Status{};
}

WindowSize MacosWindow::size() const noexcept {
    return impl_->size_;
}

WindowPoint MacosWindow::position() const noexcept {
    return impl_->position_;
}

bool MacosWindow::is_visible() const noexcept {
    return [impl_->window isVisible];
}

bool MacosWindow::is_minimized() const noexcept {
    return [impl_->window isMiniaturized];
}

bool MacosWindow::is_maximized() const noexcept {
    return [impl_->window isZoomed];
}

void MacosWindow::set_input_dispatcher(IInputDispatcher* dispatcher) noexcept {
    impl_->input_dispatcher_ = dispatcher;
}

IInputDispatcher* MacosWindow::input_dispatcher() const noexcept {
    return impl_->input_dispatcher_;
}

core::Status MacosWindow::set_cursor(CursorKind cursor) {
    if (!is_on_ui_thread()) {
        return core::Status(core::StatusCode::FailedPrecondition, "Cursor mutations must occur on UI thread");
    }
    @autoreleasepool {
        NSCursor* ns_cursor = [NSCursor arrowCursor];
        switch (cursor) {
            case CursorKind::Arrow: ns_cursor = [NSCursor arrowCursor]; break;
            case CursorKind::IBeam: ns_cursor = [NSCursor IBeamCursor]; break;
            case CursorKind::PointingHand: ns_cursor = [NSCursor pointingHandCursor]; break;
            case CursorKind::Crosshair: ns_cursor = [NSCursor crosshairCursor]; break;
            case CursorKind::ResizeHorizontal: ns_cursor = [NSCursor resizeLeftRightCursor]; break;
            case CursorKind::ResizeVertical: ns_cursor = [NSCursor resizeUpDownCursor]; break;
            default: ns_cursor = [NSCursor arrowCursor]; break;
        }
        [ns_cursor set];
        impl_->cursor_ = cursor;
    }
    return core::Status{};
}

CursorKind MacosWindow::cursor() const noexcept {
    return impl_->cursor_;
}

core::Status MacosWindow::set_cursor_visible(bool visible) {
    if (!is_on_ui_thread()) {
        return core::Status(core::StatusCode::FailedPrecondition, "Cursor mutations must occur on UI thread");
    }
    @autoreleasepool {
        if (visible) {
            [NSCursor unhide];
        } else {
            [NSCursor hide];
        }
    }
    return core::Status{};
}

void MacosWindow::set_text_input_client(ITextInputClient* client) noexcept {
    impl_->text_input_client_ = client;
}

ITextInputClient* MacosWindow::text_input_client() const noexcept {
    return impl_->text_input_client_;
}

void MacosWindow::update_ime_caret_rect(const WindowPoint& position, const WindowSize& size) {
    (void)position;
    (void)size;
}

bool MacosWindow::is_on_ui_thread() const noexcept {
    return std::this_thread::get_id() == impl_->ui_thread_id_;
}

void* MacosWindow::native_nswindow() const noexcept {
    return (__bridge void*)impl_->window;
}

void* MacosWindow::native_nsview() const noexcept {
    return (__bridge void*)impl_->view;
}

void* MacosWindow::native_cametal_layer() const noexcept {
    return (__bridge void*)impl_->view.metalLayer;
}

void MacosWindow::on_native_resized(uint32_t width, uint32_t height, float scale) {
    impl_->size_ = WindowSize{static_cast<uint32_t>(width / scale), static_cast<uint32_t>(height / scale)};
    impl_->scale_factor_ = scale;
    impl_->swapchain_generation_id_++;

    if (impl_->input_dispatcher_) {
        WindowLifecycleEvent event{
            .type = EventType::WindowResized,
            .width = width,
            .height = height,
            .scale_factor = scale,
            .timestamp_us = 0
        };
        impl_->input_dispatcher_->dispatch_lifecycle_event(event);
    }
}

void MacosWindow::on_native_moved(int32_t x, int32_t y) {
    impl_->position_ = WindowPoint{x, y};
    if (impl_->input_dispatcher_) {
        WindowLifecycleEvent event{
            .type = EventType::WindowMoved,
            .width = impl_->size_.width,
            .height = impl_->size_.height,
            .scale_factor = impl_->scale_factor_,
            .timestamp_us = 0
        };
        impl_->input_dispatcher_->dispatch_lifecycle_event(event);
    }
}

void MacosWindow::on_native_focus_changed(bool focused) {
    if (impl_->text_input_client_) {
        impl_->text_input_client_->on_focus_changed(focused);
    }
    if (impl_->input_dispatcher_) {
        WindowLifecycleEvent event{
            .type = focused ? EventType::WindowFocusGained : EventType::WindowFocusLost,
            .width = impl_->size_.width,
            .height = impl_->size_.height,
            .scale_factor = impl_->scale_factor_,
            .timestamp_us = 0
        };
        impl_->input_dispatcher_->dispatch_lifecycle_event(event);
    }
}

void MacosWindow::on_native_close_requested() {
    if (impl_->input_dispatcher_) {
        WindowLifecycleEvent event{
            .type = EventType::WindowCloseRequested,
            .width = impl_->size_.width,
            .height = impl_->size_.height,
            .scale_factor = impl_->scale_factor_,
            .timestamp_us = 0
        };
        impl_->input_dispatcher_->dispatch_lifecycle_event(event);
    }
}

} // namespace pingdart::pal::macos
