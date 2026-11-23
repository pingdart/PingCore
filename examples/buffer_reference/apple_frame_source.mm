#include "frame_source.h"

#import <Foundation/Foundation.h>
#import <TargetConditionals.h>

#include <limits>
#include <utility>

#if TARGET_OS_IOS
#import <pingcore/ios/platform_registry.h>
namespace apple = pingcore::ios;
#else
#import <pingcore/macos/platform_registry.h>
namespace apple = pingcore::macos;
#endif

namespace demo = pingcore::example::buffer_reference;

@interface PingCoreExampleFrameSource : NSObject <HUXPlatformModule>
@end

@implementation PingCoreExampleFrameSource {
  NSMutableData* _storage;
  HUXBufferReference* _reference;
  int64_t _sequence;
}

- (instancetype)init {
  self = [super init];
  if (self != nil) {
    _storage = [NSMutableData dataWithLength:demo::storage_size];
    _reference = [[HUXBufferReference alloc] initWithBytes:_storage.bytes length:_storage.length owner:_storage];
  }
  return self;
}

- (id<HUXPlatformCancellation>)invoke:(NSString*)method arguments:(HUXPlatformPayload*)arguments
                             result:(id<HUXPlatformResult>)result {
  if (![method isEqualToString:@"next"] || arguments.kind != HUXPlatformPayloadKindNull || _reference == nil ||
      _sequence == std::numeric_limits<int64_t>::max()) {
    [result failWithCode:@"example/invalid-request" message:@"PingCore frame source cannot fulfill this request"
                details:HUXPlatformPayload.nullValue];
    return nil;
  }
  // A request arrives only after the C++ reader has finished with the preceding frame.
  demo::Fill({static_cast<std::byte*>(_storage.mutableBytes), _storage.length}, ++_sequence);
  [result complete:[HUXPlatformPayload objectValue:@{
    @"pixels" : [HUXPlatformPayload bufferReferenceValue:_reference],
    @"width" : [HUXPlatformPayload integerValue:demo::frame_width],
    @"height" : [HUXPlatformPayload integerValue:demo::frame_height],
    @"stride" : [HUXPlatformPayload integerValue:demo::row_stride],
    @"sequence" : [HUXPlatformPayload integerValue:_sequence],
  }]];
  return nil;
}

- (void)dispose {
  _reference = nil;
  _storage = nil;
}

@end

#if TARGET_OS_IOS
@interface PingCoreExampleFrameSourceFactory : NSObject <HUXPlatformModuleFactory>
#else
@interface PingCoreExampleFrameSourceFactory : NSObject <HUXAppKitPlatformModuleFactory>
#endif
@end

@implementation PingCoreExampleFrameSourceFactory

#if TARGET_OS_IOS
- (id<HUXPlatformModule>)createWithOptions:(HUXPlatformPayload*)options
                                   events:(id<HUXPlatformEventEmitter>)events {
#else
- (id<HUXPlatformModule>)createWithWindow:(NSWindow*)window options:(HUXPlatformPayload*)options
                                 events:(id<HUXPlatformEventEmitter>)events {
#endif
  return [PingCoreExampleFrameSource new];
}

@end

namespace pingcore::example::buffer_reference {

void Install(ApplicationContext& root) {
  apple::ObjectiveCPlatformModuleFactory<std::shared_ptr<FrameSource>> factory{
      .factory = [PingCoreExampleFrameSourceFactory new],
      .create = CreateBridgeSource,
  };
  root.RegisterPlatformModule<std::shared_ptr<FrameSource>>(module_name, std::move(factory));
}

} // namespace pingcore::example::buffer_reference
