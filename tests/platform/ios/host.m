#import <UIKit/UIKit.h>

@interface PingCoreTestingDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation PingCoreTestingDelegate
@end

int main(int argc, char* argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass(PingCoreTestingDelegate.class));
  }
}
