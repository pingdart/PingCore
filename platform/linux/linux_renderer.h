#pragma once

#include <memory>
#include <string_view>

#include <gtk/gtk.h>

#include <pingcore/render_scene.h>
#include <pingcore/text.h>

namespace pingcore::detail {

class LinuxPlatformViews;
class TextLayoutBackend;

// Records the platform-neutral scene into GTK Snapshot nodes, using Cairo nodes
// for ordinary drawing and texture nodes for GPU-backed external frames.
class LinuxRenderer final {
public:
  LinuxRenderer();
  ~LinuxRenderer();

  LinuxRenderer(const LinuxRenderer&) = delete;
  LinuxRenderer& operator=(const LinuxRenderer&) = delete;

  void Initialize();
  void Discard() noexcept;

  // Updates logical text scaling and clears shaped paragraphs only when the effective DPI changes.
  // Invalid or unspecified DPI falls back to 96; retained TextLayouts keep their original geometry.
  [[nodiscard]] bool UpdateTextDpi(double dpi);

  [[nodiscard]] FontMetrics Metrics(const Font& font);
  [[nodiscard]] TextRunMetrics
  MeasureRun(std::string_view text, const TextStyle& style, const TextShapingOptions& options);
  [[nodiscard]] TextLayoutMetrics
  MeasureText(std::string_view text, const TextStyle& style, float max_width, const TextLayoutOptions& options = {});
  std::unique_ptr<TextLayoutBackend> CreatePlatformTextLayout(
      std::string_view text, const TextStyle& style, float max_width, const TextLayoutOptions& options = {});
  [[nodiscard]] TextLayoutMetrics
  MeasureText(const AttributedText& text, const TextStyle& style, float max_width, const TextLayoutOptions& options);
  std::unique_ptr<TextLayoutBackend> CreatePlatformTextLayout(
      const AttributedText& text, const TextStyle& style, float max_width, const TextLayoutOptions& options);

  void Snapshot(GtkSnapshot* snapshot, const RenderFrame& frame, const LinuxPlatformViews* platform_views = nullptr);

public:
  struct Impl;

private:
  std::unique_ptr<Impl> impl_;
};

} // namespace pingcore::detail
