#pragma once

#include "widget.h"
#include <vector>

namespace pingdart::ui {

class LeafRenderObjectWidget : public RenderObjectWidget {
public:
    using RenderObjectWidget::RenderObjectWidget;

    [[nodiscard]] std::shared_ptr<Element> create_element() const override;
};

class SingleChildRenderObjectWidget : public RenderObjectWidget {
public:
    SingleChildRenderObjectWidget(std::shared_ptr<const Widget> child,
                                  std::shared_ptr<const Key> key = nullptr)
        : RenderObjectWidget(std::move(key)), child_(std::move(child)) {}

    [[nodiscard]] const std::shared_ptr<const Widget>& child() const noexcept {
        return child_;
    }

    [[nodiscard]] std::shared_ptr<Element> create_element() const override;

protected:
    std::shared_ptr<const Widget> child_;
};

class MultiChildRenderObjectWidget : public RenderObjectWidget {
public:
    MultiChildRenderObjectWidget(std::vector<std::shared_ptr<const Widget>> children,
                                 std::shared_ptr<const Key> key = nullptr)
        : RenderObjectWidget(std::move(key)), children_(std::move(children)) {}

    [[nodiscard]] const std::vector<std::shared_ptr<const Widget>>& children() const noexcept {
        return children_;
    }

    [[nodiscard]] std::shared_ptr<Element> create_element() const override;

protected:
    std::vector<std::shared_ptr<const Widget>> children_;
};

} // namespace pingdart::ui
