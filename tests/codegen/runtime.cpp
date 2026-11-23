#include <pingcore/pingcore.h>

#include <catch2/catch_amalgamated.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "runtime/ui_window_internal.h"
#include "internal_access.h"

namespace {

using pingcore::Application;
using pingcore::DrawTextCommand;
using pingcore::FrameCommit;
using pingcore::UiWindow;
using pingcore::RenderFrame;
using pingcore::RenderNode;
using pingcore::Size;
using pingcore::State;
using pingcore::Text;
using pingcore::UseState;
using pingcore::View;
class TestPlatform final : public UiWindow, public pingcore::Runtime {
public:
  TestPlatform()
      : Runtime([this](std::function<void()> callback) { pending.push_back(std::move(callback)); }) {}

  ~TestPlatform() override { UiWindow::Retire(); Runtime::Retire(); }
  void OnRuntimeStopped() override {}
  std::vector<std::function<void()>> pending;
  void RequestFrameAt(double) override {
    ++requested_frames;
  }

  [[nodiscard]] double Now() const noexcept override {
    return 0.0;
  }

  pingcore::FontMetrics Metrics(const pingcore::Font&) override {
    return {.ascent = 15.0F, .descent = 5.0F};
  }

  pingcore::TextRunMetrics MeasureRun(
      std::string_view text, const pingcore::TextStyle& style, const pingcore::TextShapingOptions&
  ) override {
    const float width = static_cast<float>(text.size()) * 10.0F;
    const pingcore::FontMetrics metrics = Metrics(style.font);
    return {width, {0.0F, -15.0F, width, 20.0F}, metrics};
  }

  pingcore::TextLayoutMetrics MeasureText(const pingcore::AttributedText& paragraph, const pingcore::TextStyle&,
      float max_width, const pingcore::TextLayoutOptions& options) override {
    const auto& text = paragraph.PlainText();
    if (max_width <= 0.0F) {
      return {};
    }

    std::size_t hard_line_count = 1;
    std::size_t current_length = 0;
    std::size_t maximum_length = 0;
    std::size_t wrapped_line_count = 0;
    const auto finish_hard_line = [&] {
      maximum_length = std::max(maximum_length, current_length);
      if (std::isfinite(max_width) && options.wrap == pingcore::TextWrap::Word) {
        const float width = static_cast<float>(current_length) * 10.0F;
        wrapped_line_count += static_cast<std::size_t>(std::max(1.0F, std::ceil(width / max_width)));
      }
      current_length = 0;
    };
    for (char character : text) {
      if (character == '\n') {
        finish_hard_line();
        ++hard_line_count;
      } else {
        ++current_length;
      }
    }
    finish_hard_line();

    const float natural_width = static_cast<float>(maximum_length) * 10.0F;
    const std::size_t line_count = std::isfinite(max_width) && options.wrap == pingcore::TextWrap::Word
                                     ? wrapped_line_count
                                     : hard_line_count;
    const float measured_width = std::isfinite(max_width) ? std::min(natural_width, max_width) : natural_width;
    const Size size{measured_width, static_cast<float>(line_count) * 20.0F};
    return {size, 15.0F, size.height - 5.0F, line_count};
  }

  int requested_frames = 0;
};

State<int> generated_count;
State<bool> generated_grow_enabled;
int generated_compositions = 0;

[[pingcore::composable]] View GeneratedCounter(int initial) {
  ++generated_compositions;
  auto count = UseState(initial);
  generated_count = count;
  return Text(count);
}

View GeneratedApp() {
  return GeneratedCounter(3);
}

[[pingcore::composable]] View GeneratedGrowContent() {
  auto grow_enabled = UseState(true);
  generated_grow_enabled = grow_enabled;
  return Text("Grow content")
      .With(pingcore::Frame{.height = 20.0F}, pingcore::Grow{grow_enabled.Get() ? 1.0F : 0.0F});
}

View GeneratedGrowApp() {
  return pingcore::Column {
    Text("Fixed").With(pingcore::Frame{.height = 20.0F}),
    GeneratedGrowContent(),
  };
}

[[nodiscard]] std::string FirstText(const RenderNode& node) {
  for (const auto& command : node.content.Commands()) {
    if (const auto* text = std::get_if<DrawTextCommand>(&command)) {
      return text->text.PlainText();
    }
  }
  for (const RenderNode* child : node.children) {
    if (child != nullptr) {
      std::string text = FirstText(*child);
      if (!text.empty()) {
        return text;
      }
    }
  }
  for (const auto& command : node.foreground.Commands()) {
    if (const auto* text = std::get_if<DrawTextCommand>(&command)) {
      return text->text.PlainText();
    }
  }
  return {};
}

[[nodiscard]] std::string FirstText(const RenderFrame& frame) {
  return frame.scene.root != nullptr ? FirstText(*frame.scene.root) : std::string{};
}

[[nodiscard]] std::string FirstText(const FrameCommit& commit) {
  return FirstText(commit.render_frame);
}

} // namespace

TEST_CASE("Generated scopes run in UiWindow") {
  TestPlatform platform;
  Application application{GeneratedApp, {.show_debug_overlay = false}};
  pingcore::detail::InternalAccess::InitializeRuntime(platform, application);
  pingcore::detail::InternalAccess::InitializeWindow(platform, platform);
  UiWindow& runtime = platform;
  runtime.SetWindowMetrics({.viewport = {320.0F, 240.0F}});

  REQUIRE(FirstText(runtime.BuildFrame()) == "3");
  REQUIRE(generated_compositions == 1);
  REQUIRE(generated_count.IsValid());

  generated_count = 8;
  REQUIRE(platform.requested_frames > 0);
  REQUIRE(FirstText(runtime.BuildFrame()) == "8");
  REQUIRE(generated_compositions == 2);
}

TEST_CASE("Generated scopes expose recomposed parent layout values") {
  TestPlatform platform;
  Application application{GeneratedGrowApp, {.show_debug_overlay = false}};
  pingcore::detail::InternalAccess::InitializeRuntime(platform, application);
  pingcore::detail::InternalAccess::InitializeWindow(platform, platform);
  UiWindow& runtime = platform;
  runtime.SetWindowMetrics({.viewport = {100.0F, 100.0F}});
  runtime.BuildFrame();

  const auto* root = pingcore::detail::InternalAccess::RootNode(runtime);
  REQUIRE(root != nullptr);
  REQUIRE(root->children[1]->kind == pingcore::detail::NodeKind::Scope);
  REQUIRE(root->children[1]->bounds.height == 80.0F);
  REQUIRE(root->children[1]->children[0]->bounds.height == 80.0F);

  generated_grow_enabled = false;
  runtime.BuildFrame();

  root = pingcore::detail::InternalAccess::RootNode(runtime);
  REQUIRE(root->children[1]->bounds.height == 20.0F);
  REQUIRE(root->children[1]->children[0]->bounds.height == 20.0F);
}
