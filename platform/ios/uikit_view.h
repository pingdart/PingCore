#pragma once

#import <UIKit/UIKit.h>

namespace pingcore {
class UiWindow;
}

namespace pingcore::detail {
class IosUiWindow;
} // namespace pingcore::detail

@interface PingCoreView : UIView {
@public
  pingcore::UiWindow* pingcoreWindow;
  pingcore::detail::IosUiWindow* pingcorePlatformWindow;
  __strong NSMutableSet<UITouch*>* pingcoreTouches;
  __strong NSMutableDictionary<NSNumber*, NSNumber*>* pingcorePointerButtons;
  __strong id<UIDropInteractionDelegate> pingcoreFileDropDelegate;
}
- (void)commitPingCoreFrame;
- (void)cancelPingCoreTouches;
- (void)pingcoreKeyboardFrameDidChange:(NSNotification*)notification;
- (void)pingcoreResourceConfigurationDidChange:(NSNotification*)notification;
@end
