@import PingCorePlatform;

@interface PingCoreTestPlatformModule : NSObject <HUXPlatformModule>
@end

@implementation PingCoreTestPlatformModule

- (id<HUXPlatformCancellation>)invoke:(NSString*)method
                            arguments:(HUXPlatformPayload*)arguments
                               result:(id<HUXPlatformResult>)result {
  [result complete:[HUXPlatformPayload objectValue:@{
                    @"method" : [HUXPlatformPayload stringValue:method],
                    @"arguments" : arguments,
                  }]];
  return nil;
}

- (void)dispose {
}

@end

@interface PingCoreTestPlatformModuleFactory : NSObject <HUXAppKitPlatformModuleFactory>
@end

@implementation PingCoreTestPlatformModuleFactory

- (id<HUXPlatformModule>)createWithWindow:(NSWindow*)window
                                  options:(HUXPlatformPayload*)options
                                   events:(id<HUXPlatformEventEmitter>)events {
  (void)window;
  [events emit:@"created" payload:options];
  return [PingCoreTestPlatformModule new];
}

@end

@interface PingCoreTestPlatformView : NSObject <HUXAppKitPlatformView>
@property(nonatomic, readonly) NSView* view;
@end

@implementation PingCoreTestPlatformView

- (instancetype)init {
  self = [super init];
  if (self != nil) {
    _view = [NSView new];
  }
  return self;
}

- (void)dispose {
}

@end

@interface PingCoreTestPlatformViewFactory : NSObject <HUXAppKitPlatformViewFactory>
@end

@implementation PingCoreTestPlatformViewFactory

- (id<HUXAppKitPlatformView>)createWithWindow:(NSWindow*)window
                                   properties:(HUXPlatformPayload*)properties
                                       events:(id<HUXPlatformEventEmitter>)events {
  (void)window;
  [events emit:@"created" payload:properties];
  return [PingCoreTestPlatformView new];
}

@end

static void PingCoreTestMetalTextureDeclarations(void) {
  HUXMetalTexture* texture = [[HUXMetalTexture alloc] initWithIntrinsicSize:CGSizeMake(16.0, 9.0)];
  HUXMetalTextureOrigin origin = HUXMetalTextureOriginTopLeft;
  HUXMetalTextureAlpha alpha = HUXMetalTextureAlphaPremultiplied;
  (void)origin;
  (void)alpha;
  [texture finish];
}

static void PingCoreTestFileReferenceDeclarations(HUXFileReference* reference) {
  NSURL* url = reference.fileURL;
  HUXFileReference* round_trip = [HUXPlatformPayload fileReferenceValue:reference].fileReferenceValue;
  (void)url;
  (void)round_trip;
}
