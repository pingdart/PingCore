#pragma once

#include "widget.h"
#include "render_object.h"
#include "layout_primitives.h"
#include "focus_node.h"
#include "text_editing_controller.h"
#include <string>
#include <functional>
#include <memory>

namespace pingdart::ui {

// RenderParagraphBox
class RenderParagraphBox : public RenderBox {
public:
    RenderParagraphBox(std::string text,
                       std::string font_family = "sans-serif",
                       float font_size = 14.0f,
                       Color color = Color::from_argb(255, 255, 255, 255),
                       bool bold = false,
                       bool italic = false);

    void set_text(std::string text);
    void set_font_size(float size);
    void set_color(Color c);

    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    [[nodiscard]] float font_size() const noexcept { return font_size_; }
    [[nodiscard]] const Color& color() const noexcept { return color_; }

    void perform_layout() override;
    void paint(PaintContext& context, Offset offset) override;

private:
    std::string text_;
    std::string font_family_;
    float font_size_;
    Color color_;
    bool bold_;
    bool italic_;
};

// Text Widget
class Text : public LeafRenderObjectWidget {
public:
    explicit Text(std::string text,
                  float font_size = 14.0f,
                  Color color = Color::from_argb(255, 255, 255, 255),
                  std::string font_family = "sans-serif",
                  bool bold = false,
                  bool italic = false,
                  std::shared_ptr<const Key> key = nullptr)
        : LeafRenderObjectWidget(std::move(key)),
          text_(std::move(text)),
          font_size_(font_size),
          color_(color),
          font_family_(std::move(font_family)),
          bold_(bold),
          italic_(italic) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "Text"; }

    [[nodiscard]] std::unique_ptr<RenderObject> create_render_object(
        BuildContext& context) const override;
    void update_render_object(BuildContext& context, RenderObject& render_object) const override;

    [[nodiscard]] const std::string& text() const noexcept { return text_; }

private:
    std::string text_;
    float font_size_;
    Color color_;
    std::string font_family_;
    bool bold_;
    bool italic_;
};

// Button Widget
class Button : public StatefulWidget {
public:
    Button(std::shared_ptr<const Widget> child,
           std::function<void()> on_pressed,
           std::shared_ptr<FocusNode> focus_node = nullptr,
           Color background_color = Color::from_argb(255, 37, 99, 235),
           EdgeInsets padding = EdgeInsets::symmetric(10.0f, 16.0f),
           float border_radius = 6.0f,
           std::shared_ptr<const Key> key = nullptr);

    Button(std::string text,
           std::function<void()> on_pressed,
           std::shared_ptr<FocusNode> focus_node = nullptr,
           Color background_color = Color::from_argb(255, 37, 99, 235),
           EdgeInsets padding = EdgeInsets::symmetric(10.0f, 16.0f),
           float border_radius = 6.0f,
           std::shared_ptr<const Key> key = nullptr);

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "Button"; }

    [[nodiscard]] std::unique_ptr<StateBase> create_state() const override;

    [[nodiscard]] const std::shared_ptr<const Widget>& child() const noexcept { return child_; }
    [[nodiscard]] const std::function<void()>& on_pressed() const noexcept { return on_pressed_; }
    [[nodiscard]] const std::shared_ptr<FocusNode>& focus_node() const noexcept { return focus_node_; }
    [[nodiscard]] const Color& background_color() const noexcept { return background_color_; }
    [[nodiscard]] const EdgeInsets& padding() const noexcept { return padding_; }
    [[nodiscard]] float border_radius() const noexcept { return border_radius_; }

private:
    std::shared_ptr<const Widget> child_;
    std::function<void()> on_pressed_;
    std::shared_ptr<FocusNode> focus_node_;
    Color background_color_;
    EdgeInsets padding_;
    float border_radius_;
};

// TextField Widget
class TextField : public StatefulWidget {
public:
    TextField(std::shared_ptr<TextEditingController> controller = nullptr,
              std::shared_ptr<FocusNode> focus_node = nullptr,
              std::string placeholder = "",
              bool obscure_text = false,
              std::function<void(const std::string&)> on_changed = nullptr,
              std::function<void(const std::string&)> on_submitted = nullptr,
              std::shared_ptr<const Key> key = nullptr);

    TextField(std::string placeholder,
              std::string text = "",
              std::function<void(const std::string&)> on_changed = nullptr,
              std::shared_ptr<const Key> key = nullptr);

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "TextField"; }

    [[nodiscard]] std::unique_ptr<StateBase> create_state() const override;

    [[nodiscard]] const std::shared_ptr<TextEditingController>& controller() const noexcept { return controller_; }
    [[nodiscard]] const std::shared_ptr<FocusNode>& focus_node() const noexcept { return focus_node_; }
    [[nodiscard]] const std::string& placeholder() const noexcept { return placeholder_; }
    [[nodiscard]] bool obscure_text() const noexcept { return obscure_text_; }
    [[nodiscard]] const std::function<void(const std::string&)>& on_changed() const noexcept {
        return on_changed_;
    }
    [[nodiscard]] const std::function<void(const std::string&)>& on_submitted() const noexcept {
        return on_submitted_;
    }

private:
    std::shared_ptr<TextEditingController> controller_;
    std::shared_ptr<FocusNode> focus_node_;
    std::string placeholder_;
    bool obscure_text_{false};
    std::function<void(const std::string&)> on_changed_;
    std::function<void(const std::string&)> on_submitted_;
};

} // namespace pingdart::ui
