#pragma once

#include "widget.h"
#include "element.h"
#include "render_object.h"
#include <pingdart/rhi/display_list.h>
#include <vector>
#include <memory>

namespace pingdart::ui {

class WidgetTree {
public:
    WidgetTree();
    ~WidgetTree();

    WidgetTree(const WidgetTree&) = delete;
    WidgetTree& operator=(const WidgetTree&) = delete;

    void set_root_widget(std::shared_ptr<const Widget> root);
    void schedule_build_for(std::shared_ptr<Element> element);
    void schedule_build_for(Element* element);
    void flush_build();
    void flush_layout(Size window_size, bool tight = false);
    [[nodiscard]] core::RefPtr<pingdart::rhi::DisplayList> flush_paint();
    [[nodiscard]] HitTestResult hit_test(Offset position);

    [[nodiscard]] Element* root_element() const noexcept { return root_element_.get(); }
    [[nodiscard]] RenderBox* root_render_box() const noexcept;

private:
    std::shared_ptr<const Widget> root_widget_;
    std::shared_ptr<Element> root_element_;
    std::vector<std::weak_ptr<Element>> dirty_elements_;
};

} // namespace pingdart::ui
