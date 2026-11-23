#include "platform_text_field.h"

#import <AppKit/AppKit.h>

#include <utility>

#include <pingcore/macos/platform_registry.h>

@interface PingCoreExamplePlatformTextFieldView : NSObject <HUXAppKitPlatformView, NSTextFieldDelegate>
- (instancetype)initWithProperties:(HUXPlatformPayload*)properties
                             events:(id<HUXPlatformEventEmitter>)events;
@end

@implementation PingCoreExamplePlatformTextFieldView {
  __strong NSTextField* _view;
  __strong id<HUXPlatformEventEmitter> _events;
}

- (instancetype)initWithProperties:(HUXPlatformPayload*)properties
                             events:(id<HUXPlatformEventEmitter>)events {
  self = [super init];
  if (self == nil) {
    return nil;
  }
  _view = [[NSTextField alloc] initWithFrame:NSZeroRect];
  _view.delegate = self;
  _view.placeholderString = @"Edit PlatformView text";
  _view.controlSize = NSControlSizeLarge;
  _events = events;
  [self updateWithProperties:properties];
  return self;
}

- (NSView*)view {
  return _view;
}

- (void)updateWithProperties:(HUXPlatformPayload*)properties {
  NSString* value = properties.stringValue;
  if (![_view.stringValue isEqualToString:value]) {
    _view.stringValue = value;
  }
}

- (void)dispose {
  _view.delegate = nil;
  _events = nil;
}

- (void)controlTextDidChange:(NSNotification*)notification {
  [_events emit:@"changed" payload:[HUXPlatformPayload stringValue:_view.stringValue]];
}

@end

@interface PingCoreExamplePlatformTextFieldFactory : NSObject <HUXAppKitPlatformViewFactory>
@end

@implementation PingCoreExamplePlatformTextFieldFactory

- (id<HUXAppKitPlatformView>)createWithWindow:(NSWindow*)window
                                   properties:(HUXPlatformPayload*)properties
                                       events:(id<HUXPlatformEventEmitter>)events {
  return [[PingCoreExamplePlatformTextFieldView alloc] initWithProperties:properties events:events];
}

@end

namespace pingcore::example {

void InstallPlatformTextField(ApplicationContext& root) {
  macos::ObjectiveCPlatformViewFactory<PlatformTextFieldProperties> factory{
      .factory = [PingCoreExamplePlatformTextFieldFactory new],
  };
  root.RegisterPlatformView<PlatformTextFieldProperties>(platform_text_field::type, std::move(factory));
}

} // namespace pingcore::example
