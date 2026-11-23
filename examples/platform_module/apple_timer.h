#pragma once

#import <TargetConditionals.h>

#include "timer.h"

#if TARGET_OS_IOS
#import <pingcore/ios/platform_registry.h>
#else
#import <pingcore/macos/platform_registry.h>
#endif

namespace pingcore::example {

std::shared_ptr<TimerService> CreateAppleTimerService(PlatformChannel channel);

#if TARGET_OS_IOS
id<HUXPlatformModuleFactory> CreateAppleTimerFactory();
#else
id<HUXAppKitPlatformModuleFactory> CreateAppleTimerFactory();
#endif

} // namespace pingcore::example
