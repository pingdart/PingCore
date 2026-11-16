#pragma once

#include "widget.h"
#include "render_object.h"
#include "render_object_widget.h"
#include <vector>
#include <memory>

namespace pingdart::ui {

enum class Axis : uint8_t {
    Horizontal,
    Vertical
};

enum class MainAxisAlignment : uint8_t {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

enum class CrossAxisAlignment : uint8_t {
    Start,
    Center,
    End,
    Stretch
};

enum class FlexFit : uint8_t {
    Tight,
    Loose
};

// Base RenderBox for single-child containers
class RenderSingleChildBox : public RenderBox {
public:
    ~RenderSingleChildBox() override;

    [[nodiscard]] RenderBox* child() const noexcept { return child_; }

    void adopt_child(RenderObject* child) override;
    void drop_child(RenderObject* child) override;
    void visit_children(const std::function<void(RenderObject&)>& visitor) override;
    bool hit_test_children(HitTestResult& result, Offset position) override;
    void paint(PaintContext& context, Offset offset) override;

protected:
    RenderBox* child_{nullptr};
};

// RenderConstrainedBox
class RenderConstrainedBox : public RenderSingleChildBox {
public:
    explicit RenderConstrainedBox(BoxConstraints additional_constraints)
        : additional_constraints_(additional_constraints) {}

    void set_additional_constraints(BoxConstraints c) {
        if (additional_constraints_ == c) return;
        additional_constraints_ = c;
        mark_needs_layout();
    }

    [[nodiscard]] const BoxConstraints& additional_constraints() const noexcept {
        return additional_constraints_;
    }

    void perform_layout() override;

private:
    BoxConstraints additional_constraints_;
};

// SizedBox Widget
class SizedBox : public SingleChildRenderObjectWidget {
public:
    SizedBox(float width = BoxConstraints::infinity,
             float height = BoxConstraints::infinity,
             std::shared_ptr<const Widget> child = nullptr,
             std::shared_ptr<const Key> key = nullptr)
        : SingleChildRenderObjectWidget(std::move(child), std::move(key)),
          width_(width), height_(height) {}

    static std::shared_ptr<SizedBox> shrink(std::shared_ptr<const Widget> child = nullptr) {
        return std::make_shared<SizedBox>(0.0f, 0.0f, std::move(child));
    }

    static std::shared_ptr<SizedBox> expand(std::shared_ptr<const Widget> child = nullptr) {
        return std::make_shared<SizedBox>(BoxConstraints::infinity,
                                          BoxConstraints::infinity,
                                          std::move(child));
    }

    static std::shared_ptr<SizedBox> square(float dimension,
                                           std::shared_ptr<const Widget> child = nullptr) {
        return std::make_shared<SizedBox>(dimension, dimension, std::move(child));
    }

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "SizedBox"; }

    [[nodiscard]] std::unique_ptr<RenderObject> create_render_object(
        BuildContext& context) const override;
    void update_render_object(BuildContext& context, RenderObject& render_object) const override;

    [[nodiscard]] float width() const noexcept { return width_; }
    [[nodiscard]] float height() const noexcept { return height_; }

private:
    float width_;
    float height_;
};

// RenderPadding
class RenderPadding : public RenderSingleChildBox {
public:
    explicit RenderPadding(EdgeInsets padding) : padding_(padding) {}

    void set_padding(EdgeInsets p) {
        if (padding_ == p) return;
        padding_ = p;
        mark_needs_layout();
    }

    [[nodiscard]] const EdgeInsets& padding() const noexcept { return padding_; }

    void perform_layout() override;

private:
    EdgeInsets padding_;
};

// Padding Widget
class Padding : public SingleChildRenderObjectWidget {
public:
    Padding(EdgeInsets padding,
            std::shared_ptr<const Widget> child,
            std::shared_ptr<const Key> key = nullptr)
        : SingleChildRenderObjectWidget(std::move(child), std::move(key)),
          padding_(padding) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "Padding"; }

    [[nodiscard]] std::unique_ptr<RenderObject> create_render_object(
        BuildContext& context) const override;
    void update_render_object(BuildContext& context, RenderObject& render_object) const override;

    [[nodiscard]] const EdgeInsets& padding() const noexcept { return padding_; }

private:
    EdgeInsets padding_;
};

// RenderPositionedBox (Align / Center)
class RenderPositionedBox : public RenderSingleChildBox {
public:
    explicit RenderPositionedBox(Alignment alignment = Alignment::center())
        : alignment_(alignment) {}

    void set_alignment(Alignment a) {
        if (alignment_ == a) return;
        alignment_ = a;
        mark_needs_layout();
    }

    [[nodiscard]] Alignment alignment() const noexcept { return alignment_; }

    void perform_layout() override;

private:
    Alignment alignment_;
};

// Align Widget
class Align : public SingleChildRenderObjectWidget {
public:
    Align(Alignment alignment,
          std::shared_ptr<const Widget> child,
          std::shared_ptr<const Key> key = nullptr)
        : SingleChildRenderObjectWidget(std::move(child), std::move(key)),
          alignment_(alignment) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "Align"; }

    [[nodiscard]] std::unique_ptr<RenderObject> create_render_object(
        BuildContext& context) const override;
    void update_render_object(BuildContext& context, RenderObject& render_object) const override;

    [[nodiscard]] Alignment alignment() const noexcept { return alignment_; }

private:
    Alignment alignment_;
};

// Center Widget
class Center : public Align {
public:
    explicit Center(std::shared_ptr<const Widget> child,
                    std::shared_ptr<const Key> key = nullptr)
        : Align(Alignment::center(), std::move(child), std::move(key)) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "Center"; }
};

// RenderDecoratedBox
class RenderDecoratedBox : public RenderSingleChildBox {
public:
    RenderDecoratedBox(Color color, float border_radius = 0.0f)
        : color_(color), border_radius_(border_radius) {}

    void set_color(Color c) {
        if (color_ == c) return;
        color_ = c;
        mark_needs_paint();
    }

    void set_border_radius(float r) {
        if (border_radius_ == r) return;
        border_radius_ = r;
        mark_needs_paint();
    }

    void perform_layout() override;
    void paint(PaintContext& context, Offset offset) override;

private:
    Color color_;
    float border_radius_;
};

// DecoratedBox Widget
class DecoratedBox : public SingleChildRenderObjectWidget {
public:
    DecoratedBox(Color color,
                 float border_radius = 0.0f,
                 std::shared_ptr<const Widget> child = nullptr,
                 std::shared_ptr<const Key> key = nullptr)
        : SingleChildRenderObjectWidget(std::move(child), std::move(key)),
          color_(color), border_radius_(border_radius) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "DecoratedBox"; }

    [[nodiscard]] std::unique_ptr<RenderObject> create_render_object(
        BuildContext& context) const override;
    void update_render_object(BuildContext& context, RenderObject& render_object) const override;

private:
    Color color_;
    float border_radius_;
};

// Container (StatelessWidget composing DecoratedBox, Padding, SizedBox)
class Container : public StatelessWidget {
public:
    Container(float width = BoxConstraints::infinity,
              float height = BoxConstraints::infinity,
              Color color = Color::from_rgba_f(0.0f, 0.0f, 0.0f, 0.0f),
              EdgeInsets padding = EdgeInsets::zero(),
              float border_radius = 0.0f,
              std::shared_ptr<const Widget> child = nullptr,
              std::shared_ptr<const Key> key = nullptr)
        : StatelessWidget(std::move(key)),
          width_(width), height_(height), color_(color),
          padding_(padding), border_radius_(border_radius),
          child_(std::move(child)) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "Container"; }

    std::shared_ptr<Widget> build(BuildContext& context) const override;

private:
    float width_;
    float height_;
    Color color_;
    EdgeInsets padding_;
    float border_radius_;
    std::shared_ptr<const Widget> child_;
};

// Flexible Widget (for Row/Column children)
class Flexible : public StatelessWidget {
public:
    Flexible(int flex,
             FlexFit fit,
             std::shared_ptr<const Widget> child,
             std::shared_ptr<const Key> key = nullptr)
        : StatelessWidget(std::move(key)),
          flex_(flex), fit_(fit), child_(std::move(child)) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "Flexible"; }

    std::shared_ptr<Widget> build(BuildContext& context) const override;

    [[nodiscard]] int flex() const noexcept { return flex_; }
    [[nodiscard]] FlexFit fit() const noexcept { return fit_; }
    [[nodiscard]] const std::shared_ptr<const Widget>& child() const noexcept { return child_; }

private:
    int flex_;
    FlexFit fit_;
    std::shared_ptr<const Widget> child_;
};

class Expanded : public Flexible {
public:
    explicit Expanded(std::shared_ptr<const Widget> child,
                      int flex = 1,
                      std::shared_ptr<const Key> key = nullptr)
        : Flexible(flex, FlexFit::Tight, std::move(child), std::move(key)) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "Expanded"; }
};

// RenderFlex
class RenderFlex : public RenderBox {
public:
    RenderFlex(Axis direction = Axis::Horizontal,
               MainAxisAlignment main_axis_alignment = MainAxisAlignment::Start,
               CrossAxisAlignment cross_axis_alignment = CrossAxisAlignment::Start);
    ~RenderFlex() override;

    void set_direction(Axis d) {
        if (direction_ == d) return;
        direction_ = d;
        mark_needs_layout();
    }

    void set_main_axis_alignment(MainAxisAlignment a) {
        if (main_axis_alignment_ == a) return;
        main_axis_alignment_ = a;
        mark_needs_layout();
    }

    void set_cross_axis_alignment(CrossAxisAlignment a) {
        if (cross_axis_alignment_ == a) return;
        cross_axis_alignment_ = a;
        mark_needs_layout();
    }

    void set_flex_data(const std::vector<int>& flexes, const std::vector<FlexFit>& fits);

    void adopt_child(RenderObject* child) override;
    void drop_child(RenderObject* child) override;
    void visit_children(const std::function<void(RenderObject&)>& visitor) override;
    bool hit_test_children(HitTestResult& result, Offset position) override;
    void paint(PaintContext& context, Offset offset) override;
    void perform_layout() override;

private:
    Axis direction_;
    MainAxisAlignment main_axis_alignment_;
    CrossAxisAlignment cross_axis_alignment_;
    std::vector<RenderBox*> children_;
    std::vector<int> flexes_;
    std::vector<FlexFit> fits_;
};

// Flex Widget
class Flex : public MultiChildRenderObjectWidget {
public:
    Flex(Axis direction,
         MainAxisAlignment main_axis_alignment,
         CrossAxisAlignment cross_axis_alignment,
         std::vector<std::shared_ptr<const Widget>> children,
         std::shared_ptr<const Key> key = nullptr)
        : MultiChildRenderObjectWidget(std::move(children), std::move(key)),
          direction_(direction),
          main_axis_alignment_(main_axis_alignment),
          cross_axis_alignment_(cross_axis_alignment) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "Flex"; }

    [[nodiscard]] std::unique_ptr<RenderObject> create_render_object(
        BuildContext& context) const override;
    void update_render_object(BuildContext& context, RenderObject& render_object) const override;

    [[nodiscard]] Axis direction() const noexcept { return direction_; }
    [[nodiscard]] MainAxisAlignment main_axis_alignment() const noexcept { return main_axis_alignment_; }
    [[nodiscard]] CrossAxisAlignment cross_axis_alignment() const noexcept { return cross_axis_alignment_; }

private:
    Axis direction_;
    MainAxisAlignment main_axis_alignment_;
    CrossAxisAlignment cross_axis_alignment_;
};

class Row : public Flex {
public:
    Row(std::vector<std::shared_ptr<const Widget>> children,
        MainAxisAlignment main_axis_alignment = MainAxisAlignment::Start,
        CrossAxisAlignment cross_axis_alignment = CrossAxisAlignment::Start,
        std::shared_ptr<const Key> key = nullptr)
        : Flex(Axis::Horizontal, main_axis_alignment, cross_axis_alignment,
               std::move(children), std::move(key)) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "Row"; }
};

class Column : public Flex {
public:
    Column(std::vector<std::shared_ptr<const Widget>> children,
           MainAxisAlignment main_axis_alignment = MainAxisAlignment::Start,
           CrossAxisAlignment cross_axis_alignment = CrossAxisAlignment::Start,
           std::shared_ptr<const Key> key = nullptr)
        : Flex(Axis::Vertical, main_axis_alignment, cross_axis_alignment,
               std::move(children), std::move(key)) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "Column"; }
};

// RenderGestureDetector
class RenderGestureDetector : public RenderSingleChildBox {
public:
    explicit RenderGestureDetector(std::function<void()> on_tap = nullptr)
        : on_tap_(std::move(on_tap)) {}

    void set_on_tap(std::function<void()> on_tap) { on_tap_ = std::move(on_tap); }
    [[nodiscard]] const std::function<void()>& on_tap() const noexcept { return on_tap_; }

    void trigger_tap() {
        if (on_tap_) on_tap_();
    }

    void perform_layout() override;

private:
    std::function<void()> on_tap_;
};

// GestureDetector Widget
class GestureDetector : public SingleChildRenderObjectWidget {
public:
    GestureDetector(std::shared_ptr<const Widget> child,
                    std::function<void()> on_tap = nullptr,
                    std::shared_ptr<const Key> key = nullptr)
        : SingleChildRenderObjectWidget(std::move(child), std::move(key)),
          on_tap_(std::move(on_tap)) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "GestureDetector"; }

    [[nodiscard]] std::unique_ptr<RenderObject> create_render_object(
        BuildContext& context) const override;
    void update_render_object(BuildContext& context, RenderObject& render_object) const override;

    [[nodiscard]] const std::function<void()>& on_tap() const noexcept { return on_tap_; }

private:
    std::function<void()> on_tap_;
};

} // namespace pingdart::ui
