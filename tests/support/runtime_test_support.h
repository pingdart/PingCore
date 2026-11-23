#pragma once
#include <catch2/catch_amalgamated.hpp>

#include <pingcore/pingcore.h>

#include <cmath>
#include <functional>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "application/application_internal.h"
#include "internal_access.h"
#include "runtime/ui_window_internal.h"
#include "application/system_tray_internal.h"
#include "text/text_internal.h"

namespace pingcore::test {

class UiWindow;

PlatformResources* BuiltinTestResources();

using pingcore::AnimateTo;
using pingcore::Axis;
using pingcore::BackEvent;
using pingcore::BackPhase;
using pingcore::BottomSheetContext;
using pingcore::BottomSheetHandle;
using pingcore::Button;
using pingcore::ButtonStyle;
using pingcore::IconButton;
using pingcore::IconButtonStyle;
using pingcore::Checkbox;
using pingcore::CheckboxStyle;
using pingcore::Color;
using pingcore::Column;
using pingcore::CrossAxisAlignment;
using pingcore::Dialog;
using pingcore::DialogContext;
using pingcore::DialogHandle;
using pingcore::DrawArcCommand;
using pingcore::DrawBorderCommand;
using pingcore::DrawRectCommand;
using pingcore::DrawTextCommand;
using pingcore::Easing;
using pingcore::Enabled;
using pingcore::Environment;
using pingcore::Event;
using pingcore::EventEmitter;
using pingcore::Focusable;
using pingcore::Font;
using pingcore::FontMetrics;
using pingcore::ForEach;
using pingcore::FrameCommit;
using pingcore::GridColumns;
using pingcore::HorizontalAlignment;
using pingcore::IndexedPages;
using pingcore::Key;
using pingcore::KeyEvent;
using pingcore::KeyEventType;
using pingcore::LayerCancelPolicy;
using pingcore::LayerController;
using pingcore::LayerId;
using pingcore::LayerLevel;
using pingcore::LayerOptions;
using pingcore::LayerPointerPolicy;
using pingcore::Layout;
using pingcore::LayoutContext;
using pingcore::LayoutResult;
using pingcore::MainAxisAlignment;
using pingcore::MenuEntry;
using pingcore::MenuHandle;
using pingcore::MenuItem;
using pingcore::MenuSection;
using pingcore::ViewNode;
using pingcore::NavigationController;
using pingcore::NavigationStack;
using pingcore::NodeExtension;
using pingcore::Offset;
using pingcore::Opacity;
using pingcore::PaintCommand;
using pingcore::PaintContext;
using pingcore::PaintSequence;
using pingcore::Pager;
using pingcore::Point;
using pingcore::PointerEvent;
using pingcore::PointerEventType;
using pingcore::PopClipCommand;
using pingcore::PopTransformCommand;
using pingcore::PopupContext;
using pingcore::PopupHandle;
using pingcore::ProgressBar;
using pingcore::ProgressBarStyle;
using pingcore::ProgressCircle;
using pingcore::ProgressCircleStyle;
using pingcore::PushClipCommand;
using pingcore::PushTransformCommand;
using pingcore::RadioButton;
using pingcore::RadioButtonStyle;
using pingcore::Rect;
using pingcore::RefreshBox;
using pingcore::RenderFrame;
using pingcore::RenderNode;
using pingcore::Rotation;
using pingcore::Row;
using pingcore::Scale;
using pingcore::StrokePathCommand;
using pingcore::ScrollAlignment;
using pingcore::ScrollController;
using pingcore::ScrollInputEvent;
using pingcore::ScrollView;
using pingcore::Select;
using pingcore::SelectEvents;
using pingcore::SelectStyle;
using pingcore::SelectionArea;
using pingcore::SegmentedButton;
using pingcore::SegmentedButtonEvents;
using pingcore::SegmentedButtonItem;
using pingcore::SegmentedButtonStyle;
using pingcore::Size;
using pingcore::Slider;
using pingcore::SliderStyle;
using pingcore::SnackBarHandle;
using pingcore::SnackBarOptions;
using pingcore::SnackBarStyle;
using pingcore::Spacer;
using pingcore::Stack;
using pingcore::State;
using pingcore::StateList;
using pingcore::StringVariant;
using pingcore::StrokeCap;
using pingcore::Switch;
using pingcore::SwitchStyle;
using pingcore::TabItem;
using pingcore::Tabs;
using pingcore::TabsEvents;
using pingcore::TabsStyle;
using pingcore::Text;
using pingcore::TextEditingAction;
using pingcore::TextEditingValue;
using pingcore::TextField;
using pingcore::TextFieldEvents;
using pingcore::TextFieldStyle;
using pingcore::TextInputApplyResult;
using pingcore::TextInputCommandBatch;
using pingcore::TextInputContext;
using pingcore::TextInputGeometry;
using pingcore::TextInputSessionId;
using pingcore::TextLayoutMetrics;
using pingcore::TextLayoutOptions;
using pingcore::TextOffset;
using pingcore::TextRole;
using pingcore::TextRunMetrics;
using pingcore::TextShapingOptions;
using pingcore::TextStyle;
using pingcore::TextWrap;
using pingcore::Theme;
using pingcore::ThemeDefinition;
using pingcore::ThemeSpec;
using pingcore::ToastHandle;
using pingcore::ToggleEvents;
using pingcore::Transform2D;
using pingcore::TweenSpec;
using pingcore::UseBottomSheet;
using pingcore::UseDialog;
using pingcore::UseEnvironment;
using pingcore::UseEvents;
using pingcore::UseMenu;
using pingcore::UseNavigation;
using pingcore::UsePopup;
using pingcore::UseScrollController;
using pingcore::UseService;
using pingcore::UseSnackBar;
using pingcore::UseState;
using pingcore::UseStateList;
using pingcore::UseTheme;
using pingcore::UseToast;
using pingcore::UseViewportClass;
using pingcore::UseWindow;
using pingcore::VerticalAlignment;
using pingcore::View;
using pingcore::ViewEvents;
using pingcore::ViewportBreakpoints;
using pingcore::ViewportClass;
using pingcore::VirtualGrid;
using pingcore::VirtualLayout;
using pingcore::VirtualLayoutContext;
using pingcore::VirtualLayoutResult;
using pingcore::VirtualList;
using pingcore::WindowChromeMode;
using pingcore::WindowCommand;
using pingcore::WindowHandle;
using pingcore::WindowTitleBar;

template <pingcore::EnvironmentValue Value> Value ThemeDefinitionValue(const ThemeDefinition& definition) {
  Environment environment;
  pingcore::detail::ApplyThemeDefinition(environment, definition);
  const std::any* stored = pingcore::detail::FindLocalEnvironmentValue(environment, typeid(Value));
  const auto* typed = stored ? std::any_cast<Value>(stored) : nullptr;
  if (!typed) {
    throw std::logic_error("PingCore test theme definition does not contain the requested value");
  }
  return *typed;
}

class FlattenedScene {
public:
  [[nodiscard]] const std::vector<PaintCommand>& Commands() const noexcept {
    return commands_;
  }

private:
  void Append(const PaintSequence& sequence) {
    commands_.insert(commands_.end(), sequence.Commands().begin(), sequence.Commands().end());
  }

  void Append(const RenderNode& node) {
    if (!node.visible) {
      return;
    }
    Transform2D transform = node.transform;
    transform.translate_x += node.offset.x;
    transform.translate_y += node.offset.y;
    const bool transformed = !transform.IsIdentity();
    if (transformed) {
      commands_.emplace_back(PushTransformCommand{transform});
    }
    Append(node.content);
    for (const RenderClip& clip : node.child_clips) {
      std::visit([this](const auto& command) { commands_.emplace_back(command); }, clip);
    }
    const bool children_transformed = !node.children_transform.IsIdentity();
    if (children_transformed) {
      commands_.emplace_back(PushTransformCommand{node.children_transform});
    }
    for (const RenderNode* child : node.children) {
      if (child != nullptr) {
        Append(*child);
      }
    }
    if (children_transformed) {
      commands_.emplace_back(PopTransformCommand{});
    }
    for (std::size_t index = 0; index < node.child_clips.size(); ++index) {
      commands_.emplace_back(PopClipCommand{});
    }
    Append(node.foreground);
    if (transformed) {
      commands_.emplace_back(PopTransformCommand{});
    }
  }

  void Update(const RenderFrame& frame) {
    commands_.clear();
    if (frame.scene.root != nullptr) {
      Append(*frame.scene.root);
    }
  }

  std::vector<PaintCommand> commands_;

  friend class UiWindow;
};

class RuntimeLifetime final {
public:
  RuntimeLifetime(const Application& application, pingcore::Runtime& runtime,
      std::optional<ApplicationActivation> startup = LaunchActivation{},
      ApplicationLifecycleState lifecycle = ApplicationLifecycleState::Background) : runtime_(runtime) {
    detail::InternalAccess::InitializeRuntime(runtime_, application, std::move(startup), true, lifecycle);
  }
  ~RuntimeLifetime() { detail::InternalAccess::RetireRuntime(runtime_); }
  operator pingcore::Runtime&() const { return runtime_; }
  pingcore::Runtime& Get() const { return runtime_; }
  void RequestShutdown() { runtime_.RequestShutdown(); }
  void HandleApplicationActivation(ApplicationActivation activation) {
    runtime_.HandleApplicationActivation(std::move(activation));
  }
  void UpdateApplicationLifecycleState(ApplicationLifecycleState state) {
    runtime_.UpdateApplicationLifecycleState(state);
  }
private:
  pingcore::Runtime& runtime_;
};

class UiWindow final {
public:
  UiWindow(
      pingcore::RootFactory root_factory,
      pingcore::UiWindow& platform,
      pingcore::AppOptions options = {.show_debug_overlay = false},
      pingcore::ApplicationActivation startup_activation = pingcore::LaunchActivation{}
  )
      : application_(std::make_unique<pingcore::Application>(root_factory, std::move(options))),
        application_runtime_(&dynamic_cast<pingcore::Runtime&>(platform)), runtime_(platform) {
    try {
      detail::InternalAccess::InitializeRuntime(*application_runtime_, *application_, std::move(startup_activation));
      detail::InternalAccess::InitializeWindow(runtime_, *application_runtime_);
    } catch (...) {
      detail::InternalAccess::RetireWindow(runtime_);
      detail::InternalAccess::RetireRuntime(*application_runtime_);
      throw;
    }
  }

  UiWindow(pingcore::Runtime& application, pingcore::UiWindow& platform,
      std::optional<ResourceConfiguration> configuration = std::nullopt,
      WindowLifecycleState lifecycle = WindowLifecycleState::Background)
      : application_runtime_(&application), runtime_(platform) {
    detail::InternalAccess::InitializeWindow(runtime_, application, std::move(configuration), lifecycle);
  }

  ~UiWindow() {
    detail::InternalAccess::RetireWindow(runtime_);
    if (application_) detail::InternalAccess::RetireRuntime(*application_runtime_);
  }

  operator pingcore::UiWindow&() const { return runtime_; }
  void UpdateWindowLifecycleState(WindowLifecycleState state) { runtime_.UpdateWindowLifecycleState(state); }

  void SetWindowMetrics(pingcore::WindowMetrics metrics) {
    runtime_.SetWindowMetrics(metrics);
  }

  bool IsWindowDragRegion(Point position) const {
    return runtime_.IsWindowDragRegion(position);
  }

  bool HasContextMenuHandler(Point position) const {
    return runtime_.HasContextMenuHandler(position);
  }

  void UpdateResourceConfiguration(pingcore::ResourceConfiguration configuration) {
    runtime_.UpdateResourceConfiguration(std::move(configuration));
  }

  const FlattenedScene& BuildFrame() {
    flattened_scene_.Update(BuildCommit().render_frame);
    return flattened_scene_;
  }

  const RenderFrame& BuildRenderFrame() {
    return BuildCommit().render_frame;
  }

  const FrameCommit& BuildCommit() {
    last_commit_ = &runtime_.BuildFrame();
    return *last_commit_;
  }

  [[nodiscard]] const FrameCommit& LastCommit() const {
    if (last_commit_ == nullptr) {
      throw std::logic_error("PingCore test UiWindow has not built a frame");
    }
    return *last_commit_;
  }

  void HandlePointerEvent(const PointerEvent& event) {
    runtime_.HandlePointerEvent(event);
  }

  Point HandleScrollInput(const ScrollInputEvent& event) {
    return runtime_.HandleScrollInput(event);
  }

  bool HandleKeyEvent(const KeyEvent& event) {
    return runtime_.HandleKeyEvent(event);
  }

  void HandleApplicationActivation(pingcore::ApplicationActivation activation) {
    application_runtime_->HandleApplicationActivation(std::move(activation));
  }

  void UpdateApplicationLifecycleState(pingcore::ApplicationLifecycleState lifecycle_state) {
    application_runtime_->UpdateApplicationLifecycleState(lifecycle_state);
  }

  bool HandleWindowRequest(pingcore::WindowCommand command) {
    return runtime_.HandleWindowRequest(command);
  }

  bool HandleBack() {
    return runtime_.HandleBack();
  }

  bool HandleBack(const BackEvent& event) {
    return runtime_.HandleBack(event);
  }

  bool PerformTextInputAction(TextInputSessionId session_id, pingcore::TextInputAction action) {
    return runtime_.PerformTextInputAction(session_id, action);
  }

  bool CanPerformTextEditingAction(pingcore::TextEditingAction action) const {
    return runtime_.CanPerformTextEditingAction(action);
  }

  bool PerformTextEditingAction(pingcore::TextEditingAction action) {
    return runtime_.PerformTextEditingAction(action);
  }

  TextInputApplyResult HandleTextInputCommands(const TextInputCommandBatch& batch) {
    return runtime_.HandleTextInputCommands(batch);
  }

  TextInputContext QueryTextInputContext(TextInputSessionId session_id, TextOffset start, TextOffset length) const {
    return runtime_.QueryTextInputContext(session_id, start, length);
  }

  TextInputGeometry QueryTextInputGeometry(TextInputSessionId session_id, pingcore::TextRange range) const {
    return runtime_.QueryTextInputGeometry(session_id, range);
  }

  pingcore::TextInputPositionResult QueryTextInputPosition(TextInputSessionId session_id, Point point) const {
    return runtime_.QueryTextInputPosition(session_id, point);
  }

  pingcore::UiWindow& CoreRuntime() noexcept {
    return runtime_;
  }

  void InvalidateRoot() {
    pingcore::detail::InternalAccess::InvalidateRoot(runtime_);
  }

  const pingcore::detail::MountedNode* RootNode() const noexcept {
    return pingcore::detail::InternalAccess::RootNode(runtime_);
  }

private:
  std::unique_ptr<pingcore::Application> application_;
  pingcore::Runtime* application_runtime_;
  pingcore::UiWindow& runtime_;
  FlattenedScene flattened_scene_;
  const FrameCommit* last_commit_ = nullptr;
};

class TestSystemTrayTransport : public pingcore::detail::SystemTrayTransport {
public:
  [[nodiscard]] bool IsAvailable() const noexcept override {
    return available;
  }

  void SetEventHandler(std::function<void(pingcore::detail::SystemTrayEvent)> handler) override {
    event_handler = std::move(handler);
    ++event_handler_updates;
  }

  void Show(const pingcore::detail::ResolvedSystemTrayPresentation& value) override {
    presentation = value;
    ++show_count;
  }

  void Hide() noexcept override {
    presentation.reset();
    ++hide_count;
  }

  void SetAvailable(bool value) {
    available = value;
    if (event_handler) {
      event_handler({
          .type = pingcore::detail::SystemTrayEventType::AvailabilityChanged,
          .available = value,
      });
    }
  }

  void Activate() {
    if (event_handler) {
      event_handler({.type = pingcore::detail::SystemTrayEventType::Activate});
    }
  }

  void Invoke(std::uint64_t generation, std::uint64_t command) {
    if (event_handler) {
      event_handler({
          .type = pingcore::detail::SystemTrayEventType::Command,
          .generation = generation,
          .command = command,
      });
    }
  }

  bool available = false;
  int show_count = 0;
  int hide_count = 0;
  int event_handler_updates = 0;
  std::optional<pingcore::detail::ResolvedSystemTrayPresentation> presentation;
  std::function<void(pingcore::detail::SystemTrayEvent)> event_handler;
};

class TestPermissionTransport : public pingcore::detail::PermissionTransport {
public:
  struct StatusRequest {
    pingcore::Permission permission = pingcore::Permission::Camera;
    pingcore::detail::PermissionStatusCompletion completion;
    bool canceled = false;
  };

  struct SettingsRequest {
    pingcore::Permission permission = pingcore::Permission::Camera;
    pingcore::PermissionSettingsTarget target = pingcore::PermissionSettingsTarget::Application;
    pingcore::detail::PermissionSettingsCompletion completion;
    bool canceled = false;
  };

  std::function<void()> Check(pingcore::Permission permission,
      pingcore::detail::PermissionStatusCompletion completion) override {
    checks.push_back({permission, std::move(completion)});
    const std::size_t index = checks.size() - 1;
    if (immediate_check.has_value()) {
      checks[index].completion(*immediate_check);
    }
    return [this, index] { checks[index].canceled = true; };
  }

  std::function<void()> Request(pingcore::Permission permission,
      pingcore::detail::PermissionStatusCompletion completion) override {
    requests.push_back({permission, std::move(completion)});
    const std::size_t index = requests.size() - 1;
    if (immediate_request.has_value()) {
      requests[index].completion(*immediate_request);
    }
    if (!request_cancellable) {
      return {};
    }
    return [this, index] { requests[index].canceled = true; };
  }

  std::function<void()> OpenSettings(pingcore::Permission permission, pingcore::PermissionSettingsTarget target,
      pingcore::detail::PermissionSettingsCompletion completion) override {
    settings.push_back({permission, target, std::move(completion)});
    const std::size_t index = settings.size() - 1;
    if (immediate_settings.has_value()) {
      settings[index].completion(*immediate_settings);
    }
    return [this, index] { settings[index].canceled = true; };
  }

  void CompleteCheck(std::size_t index, pingcore::PermissionStatus status) {
    checks.at(index).completion(status);
  }

  void CompleteRequest(std::size_t index, pingcore::PermissionStatus status) {
    requests.at(index).completion(status);
  }

  void CompleteSettings(std::size_t index, bool opened) {
    settings.at(index).completion(opened);
  }

  std::vector<StatusRequest> checks;
  std::vector<StatusRequest> requests;
  std::vector<SettingsRequest> settings;
  std::optional<pingcore::PermissionStatus> immediate_check;
  std::optional<pingcore::PermissionStatus> immediate_request;
  std::optional<bool> immediate_settings;
  bool request_cancellable = true;
};

class TestPlatform : public pingcore::UiWindow, public pingcore::Runtime {
public:
  TestPlatform()
      : TestPlatform([this](std::function<void()> task) { platform_module_tasks_.push_back(std::move(task)); }) {}

  explicit TestPlatform(pingcore::PlatformResources* resources) : TestPlatform() {
    platform_resources = resources;
  }

  explicit TestPlatform(pingcore::UiThreadDispatcher dispatch_to_ui_thread)
      : Runtime(std::move(dispatch_to_ui_thread)) {}

  ~TestPlatform() override { pingcore::UiWindow::Retire(); pingcore::Runtime::Retire(); }

  void OnRuntimeStopped() override { ++application_quit_requests; }

  using pingcore::UiWindow::UpdateSystemThemeBrightness;
  using pingcore::UiWindow::BuildFrameOrErrorPage;
  ThemeBrightness QuerySystemThemeBrightness() const override { return system_theme_brightness; }
  ThemeBrightness system_theme_brightness = ThemeBrightness::Light;

  std::chrono::steady_clock::time_point TimerNow() const noexcept override {
    return std::chrono::steady_clock::time_point{} +
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(current_time));
  }

  std::function<void()> ScheduleTimerAt(std::chrono::steady_clock::time_point deadline, std::function<void()> callback) override {
    auto entry = std::make_shared<std::function<void()>>(std::move(callback));
    application_timers_.emplace(deadline, entry);
    return [entry] { *entry = {}; };
  }

  void RunPlatformModuleTasks() {
    std::vector<std::function<void()>> ready_timers;
    while (!application_timers_.empty() && application_timers_.begin()->first <= TimerNow()) {
      auto entry = application_timers_.begin()->second;
      application_timers_.erase(application_timers_.begin());
      if (*entry) {
        ready_timers.push_back(std::move(*entry));
      }
    }
    for (auto& callback : ready_timers) callback();
    while (!platform_module_tasks_.empty()) {
      std::vector<std::function<void()>> tasks = std::move(platform_module_tasks_);
      platform_module_tasks_.clear();
      for (const auto& task : tasks) {
        task();
      }
    }
  }

  class StubTextLayout final : public pingcore::detail::TextLayoutBackend {
  public:
    StubTextLayout(std::string_view text, float max_width) {
      lines_.push_back({});
      offsets_.push_back(0);
      lines_.back().boundaries.push_back({0, 0.0F});
      TextOffset offset = 0;
      for (std::size_t index = 0; index < text.size();) {
        const unsigned char first = static_cast<unsigned char>(text[index]);
        std::uint32_t code_point = first;
        std::size_t length = 1;
        if ((first & 0xE0U) == 0xC0U) {
          code_point = first & 0x1FU;
          length = 2;
        } else if ((first & 0xF0U) == 0xE0U) {
          code_point = first & 0x0FU;
          length = 3;
        } else if ((first & 0xF8U) == 0xF0U) {
          code_point = first & 0x07U;
          length = 4;
        }
        for (std::size_t continuation = 1; continuation < length; ++continuation) {
          code_point = (code_point << 6U) | (static_cast<unsigned char>(text[index + continuation]) & 0x3FU);
        }
        index += length;
        const TextOffset code_units = code_point > 0xFFFFU ? 2 : 1;
        if (code_point == '\n') {
          offset += code_units;
          offsets_.push_back(offset);
          lines_.back().hard_break = true;
          lines_.push_back({});
          lines_.back().boundaries.push_back({offset, 0.0F});
          continue;
        }

        const bool combining = code_point >= 0x0300U && code_point <= 0x036FU;
        const float width = code_point > 0xFFFFU ? 20.0F : 10.0F;
        Line& line = lines_.back();
        const float position = line.boundaries.back().x;
        if (!combining && std::isfinite(max_width) && position > 0.0F && position + width > max_width) {
          lines_.push_back({});
          lines_.back().boundaries.push_back({offset, 0.0F});
        }
        offset += code_units;
        if (!combining) {
          offsets_.push_back(offset);
          Line& target = lines_.back();
          target.boundaries.push_back({offset, target.boundaries.back().x + width});
        } else if (offsets_.size() > 1) {
          offsets_.back() = offset;
          lines_.back().boundaries.back().offset = offset;
        }
      }
    }

    Size Measure() const override {
      float width = 0.0F;
      for (const Line& line : lines_) {
        width = std::max(width, line.boundaries.back().x);
      }
      return {width, static_cast<float>(lines_.size()) * 20.0F};
    }

    std::vector<pingcore::TextLineMetrics> LineMetrics() const override {
      std::vector<pingcore::TextLineMetrics> result;
      for (const Line& line : lines_) {
        const float y = static_cast<float>(result.size()) * 20.0F;
        result.push_back({{line.boundaries.front().offset, line.boundaries.back().offset},
                          {0.0F, y, line.boundaries.back().x, 20.0F}, y + 15.0F});
      }
      return result;
    }

    TextPosition HitTest(Point point) const override {
      const std::size_t line_index = std::min(
          lines_.size() - 1,
          static_cast<std::size_t>(std::max(0.0F, std::floor(point.y / 20.0F)))
      );
      const Line& line = lines_[line_index];
      for (std::size_t index = 1; index < line.boundaries.size(); ++index) {
        if (point.x < (line.boundaries[index - 1].x + line.boundaries[index].x) * 0.5F) {
          return {
              line.boundaries[index - 1].offset,
              pingcore::TextAffinity::Downstream,
          };
        }
      }
      return {
          line.boundaries.back().offset,
          line_index + 1 < lines_.size() && !line.hard_break &&
                  lines_[line_index + 1].boundaries.front().offset == line.boundaries.back().offset
              ? pingcore::TextAffinity::Upstream
              : pingcore::TextAffinity::Downstream,
      };
    }

    Rect CaretRect(TextOffset offset, pingcore::TextAffinity affinity) const override {
      const std::size_t line_index = LineIndex(offset, affinity);
      return {Position(lines_[line_index], offset), static_cast<float>(line_index) * 20.0F, 1.0F, 20.0F};
    }

    std::vector<Rect> RangeRects(pingcore::TextRange range) const override {
      if (range.IsCollapsed()) {
        return {};
      }
      std::vector<Rect> rects;
      for (std::size_t index = 0; index < lines_.size(); ++index) {
        const Line& line = lines_[index];
        const TextOffset line_start = line.boundaries.front().offset;
        const TextOffset line_end = line.boundaries.back().offset;
        const TextOffset start = std::max(range.start, line_start);
        const TextOffset end = std::min(range.end, line_end);
        if (start < end) {
          const float x = Position(line, start);
          rects.push_back({
              x,
              static_cast<float>(index) * 20.0F,
              Position(line, end) - x,
              20.0F,
          });
        }
      }
      return rects;
    }

    TextOffset PreviousCaretOffset(TextOffset offset) const override {
      const auto found = std::lower_bound(offsets_.begin(), offsets_.end(), offset);
      return found == offsets_.begin() ? 0 : *std::prev(found);
    }

    TextOffset NextCaretOffset(TextOffset offset) const override {
      const auto found = std::upper_bound(offsets_.begin(), offsets_.end(), offset);
      return found == offsets_.end() ? offsets_.back() : *found;
    }

  private:
    struct Boundary {
      TextOffset offset = 0;
      float x = 0.0F;
    };

    struct Line {
      std::vector<Boundary> boundaries;
      bool hard_break = false;
    };

    std::size_t LineIndex(TextOffset offset, pingcore::TextAffinity affinity) const {
      for (std::size_t index = 0; index < lines_.size(); ++index) {
        const Line& line = lines_[index];
        const TextOffset start = line.boundaries.front().offset;
        const TextOffset end = line.boundaries.back().offset;
        if (offset < end || (offset == end && (affinity == pingcore::TextAffinity::Upstream ||
                                               index + 1 == lines_.size() || line.hard_break))) {
          return index;
        }
        if (offset < start) {
          return index;
        }
      }
      return lines_.size() - 1;
    }

    static float Position(const Line& line, TextOffset offset) {
      const auto found = std::lower_bound(
          line.boundaries.begin(),
          line.boundaries.end(),
          offset,
          [](const Boundary& boundary, TextOffset value) { return boundary.offset < value; }
      );
      return found == line.boundaries.end() ? line.boundaries.back().x : found->x;
    }

    std::vector<TextOffset> offsets_;
    std::vector<Line> lines_;
  };

  void RequestFrameAt(double deadline) override {
    ++requested_frames;
    requested_deadlines.push_back(deadline);
  }

  double Now() const noexcept override {
    return current_time;
  }

  void SetPointerCursor(pingcore::PointerCursorKind kind) override {
    pointer_cursors.push_back(kind);
  }

  void SetSystemBarsContentBrightness(
      pingcore::SystemBarContentBrightness status_bar, pingcore::SystemBarContentBrightness navigation_bar
  ) override {
    ++system_bars_updates;
    system_bar_brightness = std::pair{status_bar, navigation_bar};
  }

  void AdvanceTime(double seconds) {
    current_time += seconds;
  }

  FontMetrics Metrics(const Font&) override {
    return {
        .ascent = 15.0F,
        .descent = 5.0F,
        .underline_position = 2.0F,
        .underline_thickness = 1.0F,
        .strike_through_position = 7.0F,
        .strike_through_thickness = 1.0F,
    };
  }

  TextRunMetrics MeasureRun(std::string_view text, const TextStyle& style, const TextShapingOptions&) override {
    const float width = static_cast<float>(text.size()) * 10.0F;
    const FontMetrics metrics = Metrics(style.font);
    const Rect bounds{0.0F, -metrics.ascent, width, metrics.LineHeight()};
    return {width, bounds, metrics};
  }

  using TextMeasurer::MeasureText;
  using UiWindow::CreatePlatformTextLayout;

  TextLayoutMetrics MeasureText(const pingcore::AttributedText& paragraph, const TextStyle&, float max_width,
      const TextLayoutOptions& options) override {
    const auto& text = paragraph.PlainText();
    if (max_width <= 0.0F) {
      return {};
    }

    std::vector<std::size_t> hard_lines{0};
    for (char character : text) {
      if (character == '\n') {
        hard_lines.push_back(0);
      } else {
        ++hard_lines.back();
      }
    }
    const auto widest = std::max_element(hard_lines.begin(), hard_lines.end());
    const float natural_width = static_cast<float>(*widest) * 10.0F;
    const bool wraps = std::isfinite(max_width) && options.wrap == TextWrap::Word;
    std::size_t line_count = hard_lines.size();
    if (wraps) {
      line_count = 0;
      for (std::size_t length : hard_lines) {
        const float width = static_cast<float>(length) * 10.0F;
        line_count += static_cast<std::size_t>(std::max(1.0F, std::ceil(width / max_width)));
      }
    }
    const float measured_width = std::isfinite(max_width) ? std::min(natural_width, max_width) : natural_width;
    const Size size{measured_width, static_cast<float>(line_count) * 20.0F};
    return {size, 15.0F, size.height - 5.0F, line_count};
  }

  std::unique_ptr<pingcore::detail::TextLayoutBackend> CreatePlatformTextLayout(const pingcore::AttributedText& paragraph,
      const TextStyle&, float max_width, const TextLayoutOptions& options) override {
    const auto& text = paragraph.PlainText();
    return std::make_unique<StubTextLayout>(
        text,
        options.wrap == TextWrap::NoWrap ? std::numeric_limits<float>::infinity() : max_width
    );
  }

  pingcore::PlatformTextInput* TextInput() noexcept override {
    return platform_text_input;
  }

  pingcore::PlatformClipboard* Clipboard() noexcept override {
    return platform_clipboard;
  }

  pingcore::PlatformResources* Resources() noexcept override {
    return platform_resources;
  }

  std::optional<pingcore::ProcessMetrics> QueryProcessMetrics() noexcept override {
    return process_metrics;
  }

  void RequestWindowCommand(pingcore::WindowCommand command) override {
    window_commands.push_back(command);
  }



  std::shared_ptr<pingcore::detail::SystemTrayTransport> CreateSystemTrayTransport() override {
    return system_tray_transport;
  }

  std::shared_ptr<pingcore::detail::PermissionTransport> CreatePermissionTransport() override {
    return permission_transport;
  }

  int requested_frames = 0;
  std::multimap<std::chrono::steady_clock::time_point, std::shared_ptr<std::function<void()>>> application_timers_;
  double current_time = 0.0;
  std::vector<double> requested_deadlines;
  std::vector<pingcore::PointerCursorKind> pointer_cursors;
  int system_bars_updates = 0;
  std::optional<std::pair<pingcore::SystemBarContentBrightness, pingcore::SystemBarContentBrightness>>
      system_bar_brightness;
  std::optional<pingcore::ProcessMetrics> process_metrics;
  std::vector<pingcore::WindowCommand> window_commands;
  int application_quit_requests = 0;
  std::shared_ptr<TestSystemTrayTransport> system_tray_transport;
  std::shared_ptr<TestPermissionTransport> permission_transport;
  pingcore::PlatformTextInput* platform_text_input = nullptr;
  pingcore::PlatformClipboard* platform_clipboard = nullptr;
  pingcore::PlatformResources* platform_resources = nullptr;

private:
  std::vector<std::function<void()>> platform_module_tasks_;
};

inline void SettlePresentation(TestPlatform& platform, UiWindow& runtime, double duration = 0.5) {
  platform.AdvanceTime(duration);
  runtime.BuildFrame();
  // Exit completion invalidates the layer stack; the following commit removes the retained entry.
  runtime.BuildFrame();
}

inline std::string FirstText(const FlattenedScene& scene) {
  for (const auto& command : scene.Commands()) {
    if (const auto* text = std::get_if<DrawTextCommand>(&command)) {
      return text->text.PlainText();
    }
  }
  return {};
}

inline bool ContainsText(const FlattenedScene& scene, std::string_view expected) {
  for (const auto& command : scene.Commands()) {
    const auto* text = std::get_if<DrawTextCommand>(&command);
    if (text && text->text.PlainText() == expected) {
      return true;
    }
  }
  return false;
}

inline const DrawTextCommand* FindText(const FlattenedScene& scene, std::string_view expected) {
  for (const auto& command : scene.Commands()) {
    const auto* text = std::get_if<DrawTextCommand>(&command);
    if (text && text->text.PlainText() == expected) {
      return text;
    }
  }
  return nullptr;
}

inline const DrawRectCommand* FindRect(const FlattenedScene& scene, Rect expected) {
  for (const auto& command : scene.Commands()) {
    const auto* rect = std::get_if<DrawRectCommand>(&command);
    if (rect && rect->rect == expected) {
      return rect;
    }
  }
  return nullptr;
}

inline const Color* SolidBrushColor(const Brush& brush) {
  return std::get_if<Color>(&brush.Get());
}

inline const Color* SolidFillColor(const pingcore::VisualFill& fill) {
  const auto* brush = std::get_if<Brush>(&fill.Get());
  return brush != nullptr ? SolidBrushColor(*brush) : nullptr;
}

inline const Color* SolidFillColor(const std::optional<VisualFill>& fill) {
  return fill.has_value() ? SolidFillColor(*fill) : nullptr;
}

inline const detail::MountedNode* FindMountedKind(const detail::MountedNode& node, detail::NodeKind kind) {
  if (node.kind == kind) {
    return &node;
  }
  for (const auto& child : node.children) {
    if (const detail::MountedNode* found = FindMountedKind(*child, kind)) {
      return found;
    }
  }
  return nullptr;
}

inline const detail::MountedNode* FindMountedText(const detail::MountedNode& node, std::string_view text) {
  if (node.text.PlainText() == text) {
    return &node;
  }
  for (const auto& child : node.children) {
    if (const detail::MountedNode* found = FindMountedText(*child, text)) {
      return found;
    }
  }
  return nullptr;
}

inline const Color* LayerFillColor(const std::optional<IndicationLayer>& layer) {
  return layer.has_value() ? SolidFillColor(layer->fill) : nullptr;
}

inline bool BrushIsColor(const Brush& brush, Color expected) {
  const Color* color = SolidBrushColor(brush);
  return color != nullptr && *color == expected;
}

inline const DrawRectCommand* FindRectWithColor(const FlattenedScene& scene, Color expected) {
  for (const auto& command : scene.Commands()) {
    const auto* rect = std::get_if<DrawRectCommand>(&command);
    if (rect && BrushIsColor(rect->brush, expected)) {
      return rect;
    }
  }
  return nullptr;
}

inline const DrawRectCommand* FindRectWithColor(const FlattenedScene& scene, const pingcore::VisualFill& expected) {
  const Color* color = SolidFillColor(expected);
  return color != nullptr ? FindRectWithColor(scene, *color) : nullptr;
}

inline std::optional<Rect> FindPresentedRectWithColor(
    const FlattenedScene& scene,
    Color expected,
    std::optional<Size> expected_size = std::nullopt
) {
  std::vector<Transform2D> transform_stack{Transform2D{}};
  for (const auto& command : scene.Commands()) {
    if (const auto* transform = std::get_if<PushTransformCommand>(&command)) {
      transform_stack.push_back(detail::ComposeTransform(transform_stack.back(), transform->transform));
      continue;
    }
    if (std::holds_alternative<pingcore::PopTransformCommand>(command)) {
      transform_stack.pop_back();
      continue;
    }
    const auto* rect = std::get_if<DrawRectCommand>(&command);
    if (rect && BrushIsColor(rect->brush, expected) &&
        (!expected_size.has_value() || Size{rect->rect.width, rect->rect.height} == *expected_size)) {
      return detail::TransformBounds(transform_stack.back(), rect->rect);
    }
    const auto* path = std::get_if<FillPathCommand>(&command);
    if (path && BrushIsColor(path->brush, expected) &&
        (!expected_size.has_value() || Size{path->path.Bounds().width, path->path.Bounds().height} == *expected_size)) {
      return detail::TransformBounds(transform_stack.back(), path->path.Bounds());
    }
  }
  return std::nullopt;
}

inline std::optional<Rect> FindPresentedRectWithColor(
    const FlattenedScene& scene,
    const pingcore::VisualFill& expected,
    std::optional<Size> expected_size = std::nullopt
) {
  const Color* color = SolidFillColor(expected);
  return color != nullptr ? FindPresentedRectWithColor(scene, *color, expected_size) : std::nullopt;
}

inline const DrawBorderCommand* FindBorderWithColor(const FlattenedScene& scene, Color expected) {
  for (const auto& command : scene.Commands()) {
    const auto* border = std::get_if<DrawBorderCommand>(&command);
    if (border && border->color == expected) {
      return border;
    }
  }
  return nullptr;
}

inline bool ContainsRect(const FlattenedScene& scene, Rect expected) {
  for (const auto& command : scene.Commands()) {
    const auto* rect = std::get_if<DrawRectCommand>(&command);
    if (rect && rect->rect == expected) {
      return true;
    }
  }
  return false;
}

inline std::optional<float> RectAlpha(const FlattenedScene& scene, Rect expected) {
  for (const auto& command : scene.Commands()) {
    const auto* rect = std::get_if<DrawRectCommand>(&command);
    if (rect && rect->rect == expected) {
      if (const Color* color = SolidBrushColor(rect->brush)) {
        return color->alpha;
      }
    }
  }
  return std::nullopt;
}

inline std::optional<Rect> FindPresentedTextRect(const FlattenedScene& scene, std::string_view expected) {
  std::vector<Transform2D> transform_stack{Transform2D{}};
  for (const PaintCommand& command : scene.Commands()) {
    if (const auto* transform = std::get_if<PushTransformCommand>(&command)) {
      transform_stack.push_back(detail::ComposeTransform(transform_stack.back(), transform->transform));
      continue;
    }
    if (std::holds_alternative<PopTransformCommand>(command)) {
      transform_stack.pop_back();
      continue;
    }
    const auto* text = std::get_if<DrawTextCommand>(&command);
    if (text && text->text.PlainText() == expected) {
      return detail::TransformBounds(transform_stack.back(), text->rect);
    }
  }
  return std::nullopt;
}

inline std::optional<Rect> FindPresentedStrokePathRect(const FlattenedScene& scene, Color expected) {
  std::vector<Transform2D> transform_stack{Transform2D{}};
  for (const PaintCommand& command : scene.Commands()) {
    if (const auto* transform = std::get_if<PushTransformCommand>(&command)) {
      transform_stack.push_back(detail::ComposeTransform(transform_stack.back(), transform->transform));
      continue;
    }
    if (std::holds_alternative<PopTransformCommand>(command)) {
      transform_stack.pop_back();
      continue;
    }
    const auto* path = std::get_if<StrokePathCommand>(&command);
    if (path && BrushIsColor(path->brush, expected)) {
      return detail::TransformBounds(transform_stack.back(), path->path.Bounds());
    }
  }
  return std::nullopt;
}

inline void InvokeClick(const pingcore::detail::MountedNode& node) {
  REQUIRE(pingcore::detail::EmitEvent<ViewEvents::Click>(node.event_bindings));
}

inline void ClickAt(UiWindow& runtime, Point position, std::int64_t pointer_id = 0) {
  runtime.HandlePointerEvent(
      PointerEvent{
          PointerEventType::Down,
          pointer_id,
          position,
      }
  );
  runtime.HandlePointerEvent(
      PointerEvent{
          PointerEventType::Up,
          pointer_id,
          position,
      }
  );
}

} // namespace pingcore::test
