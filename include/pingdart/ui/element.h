#pragma once

#include "widget.h"
#include "render_object.h"
#include "render_object_widget.h"
#include <memory>
#include <vector>
#include <functional>

namespace pingdart::ui {

class WidgetTree;

class Element : public BuildContext, public std::enable_shared_from_this<Element> {
public:
    explicit Element(std::shared_ptr<const Widget> widget);
    ~Element() override;

    // BuildContext
    [[nodiscard]] Element& element() override { return *this; }
    [[nodiscard]] const Widget& widget() const override { return *widget_; }
    [[nodiscard]] RenderObject* find_render_object() const override;

    [[nodiscard]] Element* parent() const noexcept { return parent_; }
    [[nodiscard]] int depth() const noexcept { return depth_; }
    [[nodiscard]] bool dirty() const noexcept { return dirty_; }
    [[nodiscard]] bool mounted() const noexcept { return mounted_; }

    [[nodiscard]] WidgetTree* owner() const noexcept { return owner_; }
    void set_owner(WidgetTree* owner) noexcept { owner_ = owner; }

    virtual void mount(Element* parent);
    virtual void unmount();
    virtual void update(std::shared_ptr<const Widget> new_widget);
    virtual void perform_rebuild() = 0;

    void mark_needs_build();
    void rebuild();

    [[nodiscard]] virtual RenderObject* render_object() const { return nullptr; }
    virtual void visit_children(const std::function<void(Element&)>& visitor) = 0;

    std::shared_ptr<Element> update_child(std::shared_ptr<Element> current_child,
                                          std::shared_ptr<const Widget> new_widget);

    static std::shared_ptr<Element> inflate_widget(std::shared_ptr<const Widget> widget,
                                                   Element* parent);

protected:
    std::shared_ptr<const Widget> widget_;
    Element* parent_{nullptr};
    WidgetTree* owner_{nullptr};
    int depth_{0};
    bool dirty_{true};
    bool mounted_{false};
};

class ComponentElement : public Element {
public:
    using Element::Element;

    void mount(Element* parent) override;
    void perform_rebuild() override;
    virtual std::shared_ptr<Widget> build() = 0;
    void visit_children(const std::function<void(Element&)>& visitor) override;
    void unmount() override;
    [[nodiscard]] RenderObject* render_object() const override;

protected:
    std::shared_ptr<Element> child_;
};

class StatelessElement : public ComponentElement {
public:
    explicit StatelessElement(std::shared_ptr<const StatelessWidget> widget);
    std::shared_ptr<Widget> build() override;
};

class StatefulElement : public ComponentElement {
public:
    explicit StatefulElement(std::shared_ptr<const StatefulWidget> widget);
    ~StatefulElement() override;

    void mount(Element* parent) override;
    void update(std::shared_ptr<const Widget> new_widget) override;
    std::shared_ptr<Widget> build() override;
    void unmount() override;

    [[nodiscard]] StateBase* state() const noexcept { return state_.get(); }

private:
    std::unique_ptr<StateBase> state_;
};

class RenderObjectElement : public Element {
public:
    explicit RenderObjectElement(std::shared_ptr<const RenderObjectWidget> widget);
    ~RenderObjectElement() override;

    void mount(Element* parent) override;
    void unmount() override;
    void update(std::shared_ptr<const Widget> new_widget) override;
    void perform_rebuild() override;

    [[nodiscard]] RenderObject* render_object() const override { return render_object_.get(); }

    virtual void insert_render_object_child(RenderObject& child, int index) = 0;
    virtual void move_render_object_child(RenderObject& child, int old_index, int new_index) = 0;
    virtual void remove_render_object_child(RenderObject& child) = 0;

protected:
    void attach_render_object();
    void detach_render_object();

    std::unique_ptr<RenderObject> render_object_;
};

class LeafRenderObjectElement : public RenderObjectElement {
public:
    explicit LeafRenderObjectElement(std::shared_ptr<const LeafRenderObjectWidget> widget)
        : RenderObjectElement(widget) {}

    void insert_render_object_child(RenderObject& child, int index) override {
        (void)child;
        (void)index;
    }
    void move_render_object_child(RenderObject& child, int old_index, int new_index) override {
        (void)child;
        (void)old_index;
        (void)new_index;
    }
    void remove_render_object_child(RenderObject& child) override {
        (void)child;
    }
    void visit_children(const std::function<void(Element&)>& visitor) override {
        (void)visitor;
    }
};

class SingleChildRenderObjectElement : public RenderObjectElement {
public:
    explicit SingleChildRenderObjectElement(std::shared_ptr<const SingleChildRenderObjectWidget> widget);

    void mount(Element* parent) override;
    void update(std::shared_ptr<const Widget> new_widget) override;
    void perform_rebuild() override;
    void unmount() override;
    void visit_children(const std::function<void(Element&)>& visitor) override;

    void insert_render_object_child(RenderObject& child, int index) override;
    void move_render_object_child(RenderObject& child, int old_index, int new_index) override;
    void remove_render_object_child(RenderObject& child) override;

    [[nodiscard]] Element* child() const noexcept { return child_.get(); }

protected:
    std::shared_ptr<Element> child_;
};

class MultiChildRenderObjectElement : public RenderObjectElement {
public:
    explicit MultiChildRenderObjectElement(std::shared_ptr<const MultiChildRenderObjectWidget> widget);

    void mount(Element* parent) override;
    void update(std::shared_ptr<const Widget> new_widget) override;
    void perform_rebuild() override;
    void unmount() override;
    void visit_children(const std::function<void(Element&)>& visitor) override;

    void insert_render_object_child(RenderObject& child, int index) override;
    void move_render_object_child(RenderObject& child, int old_index, int new_index) override;
    void remove_render_object_child(RenderObject& child) override;

    [[nodiscard]] const std::vector<std::shared_ptr<Element>>& children() const noexcept {
        return children_;
    }

    static std::vector<std::shared_ptr<Element>> update_children_list(
        Element* parent,
        std::vector<std::shared_ptr<Element>>& old_children,
        const std::vector<std::shared_ptr<const Widget>>& new_widgets);

protected:
    std::vector<std::shared_ptr<Element>> children_;
};

} // namespace pingdart::ui
