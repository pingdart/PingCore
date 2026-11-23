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

@interface PingCoreTestPlatformModuleFactory : NSObject <HUXUIKitWindowPlatformModuleFactory>
@end

@implementation PingCoreTestPlatformModuleFactory

- (id<HUXPlatformModule>)createWithViewController:(UIViewController*)viewController
                                           options:(HUXPlatformPayload*)options
                                            events:(id<HUXPlatformEventEmitter>)events {
  (void)viewController;
  [events emit:@"created" payload:options];
  return [PingCoreTestPlatformModule new];
}

@end

@interface PingCoreTestPlatformView : NSObject <HUXUIKitPlatformView>
@property(nonatomic, readonly) UIView* view;
@end

@implementation PingCoreTestPlatformView

- (instancetype)init {
  self = [super init];
  if (self != nil) {
    _view = [UIView new];
  }
  return self;
}

- (void)dispose {
}

@end

@interface PingCoreTestPlatformViewFactory : NSObject <HUXUIKitPlatformViewFactory>
@end

@implementation PingCoreTestPlatformViewFactory

- (id<HUXUIKitPlatformView>)createWithViewController:(UIViewController*)viewController
                                           properties:(HUXPlatformPayload*)properties
                                               events:(id<HUXPlatformEventEmitter>)events {
  (void)viewController;
  [events emit:@"created" payload:properties];
  return [PingCoreTestPlatformView new];
}

@end

static void PingCoreTestFileReferenceDeclarations(HUXFileReference* reference) {
  NSURL* url = reference.fileURL;
  NSError* error = nil;
  HUXFileReference* copied = [HUXFileReference copyFromFileURL:url error:&error];
  HUXFileReference* round_trip = [HUXPlatformPayload fileReferenceValue:reference].fileReferenceValue;
  (void)copied;
  (void)url;
  (void)round_trip;
}

@interface PingCoreTestApplicationModuleFactory : NSObject <HUXPlatformModuleFactory>
@end
@implementation PingCoreTestApplicationModuleFactory
- (id<HUXPlatformModule>)createWithOptions:(HUXPlatformPayload*)options
                                   events:(id<HUXPlatformEventEmitter>)events {
  [events emit:@"created" payload:options];
  return [PingCoreTestPlatformModule new];
}
@end

static id<HUXPlatformCancellation> PingCoreTestPresentation(UIViewController* owner) {
  return [HUXGetPresentationCoordinator(owner) enqueueWithStart:
      ^id<HUXPlatformCancellation>(UIViewController* presenter, void (^finished)(void)) {
    (void)presenter;
    finished();
    return nil;
  } failure:^(NSError* error) { (void)error; }];
}
