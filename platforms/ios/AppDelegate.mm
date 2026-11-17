// platforms/ios/AppDelegate.mm
#import <UIKit/UIKit.h>
#include <pingdart/core/runtime.h>
#include <pingdart/pal/ios/ios_platform.h>

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow* window;
@end

@implementation AppDelegate {
    std::unique_ptr<pingdart::pal::ios::IOSPlatformHost> _platformHost;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    (void)pingdart::core::Runtime::initialize();
    _platformHost = std::make_unique<pingdart::pal::ios::IOSPlatformHost>();
    (void)_platformHost->initialize();
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    if (_platformHost) {
        _platformHost->on_application_will_resign_active();
    }
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    if (_platformHost) {
        _platformHost->on_application_did_enter_background();
    }
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    if (_platformHost) {
        _platformHost->on_application_will_enter_foreground();
    }
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    if (_platformHost) {
        _platformHost->on_application_did_become_active();
    }
}

- (void)applicationWillTerminate:(UIApplication *)application {
    if (_platformHost) {
        _platformHost->shutdown();
        _platformHost.reset();
    }
    pingdart::core::Runtime::shutdown();
}

@end

int main(int argc, char * argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
