#pragma once

#include <cstdint>
#include <memory>

#include <gtk/gtk.h>

#include <pingcore/geometry.h>

namespace pingcore {

class PlacePlatformViewCommand;
class UiWindow;
struct RenderFrame;

namespace detail {

class PlatformRegistry;

class LinuxPlatformViews final {
public:
  LinuxPlatformViews(UiWindow& ui, PlatformRegistry& registry, GtkWindow* window, GtkWidget* canvas,
                     GtkWidget* input);
  ~LinuxPlatformViews();

  void Commit(const RenderFrame& frame);
  void Allocate(int width, int height);
  void SnapshotView(GtkSnapshot* snapshot, const PlacePlatformViewCommand& command) const;

  bool TargetsPingCore(Point point) const;
  void SynchronizeFocusFromGtk();
  void ApplyFocusToGtk(bool reverse);
  bool HandleTab(bool reverse);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace detail
} // namespace pingcore
