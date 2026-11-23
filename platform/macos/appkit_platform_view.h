#pragma once

#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include <pingcore/platform_registry.h>
#include <pingcore/render_scene.h>

namespace pingcore {
class UiWindow;
}

namespace pingcore::detail {

class AppKitRenderer;
class FrameCoordinator;

class AppKitPlatformViews final {
public:
  AppKitPlatformViews(AppKitRenderer& renderer, FrameCoordinator& frame_coordinator, PlatformRegistry& registry,
                      UiWindow& ui_window, NSWindow* platform_window);
  ~AppKitPlatformViews();

  AppKitPlatformViews(const AppKitPlatformViews&) = delete;
  AppKitPlatformViews& operator=(const AppKitPlatformViews&) = delete;

  [[nodiscard]] bool Commit(NSView* root, const RenderFrame& frame);
  std::optional<double> DrawBase(CGContextRef context, CGRect dirty_rect, bool draw_background, bool platform_ready);
  void DrawSlice(CGContextRef context, CGRect dirty_rect, std::size_t first_command, std::size_t command_count);
  void InvalidateSlices();
  NSView* HitTest(Point point) const;
  NSView* AccessibilityView(std::uint64_t identity) const;
  [[nodiscard]] bool BeginFocusTraversal(NSResponder* responder, bool reverse);
  void EndFocusTraversal();
  void SynchronizeFocus(NSResponder* responder);
  void Shutdown();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace pingcore::detail
