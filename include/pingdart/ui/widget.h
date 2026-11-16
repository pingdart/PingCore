#pragma once

#include "key.h"
#include <memory>
#include <string_view>
#include <functional>

namespace pingdart::ui {

class Element;
class RenderObject;
class Widget;

class BuildContext {
public:
    virtual ~BuildContext() = default;
    [[nodiscard]] virtual Element& element() = 0;
    [[nodiscard]] virtual const Widget& widget() const = 0;
    [[nodiscard]] virtual RenderObject* find_render_object() const = 0;
};

class Widget : public std::enable_shared_from_this<Widget> {
public:
    explicit Widget(std::shared_ptr<const Key> key = nullptr) : key_(std::move(key)) {}
    virtual ~Widget() = default;

    [[nodiscard]] const std::shared_ptr<const Key>& key() const noexcept { return key_; }
    [[nodiscard]] virtual std::shared_ptr<Element> create_element() const = 0;
    [[nodiscard]] virtual std::string_view widget_type() const noexcept = 0;

    static bool can_update(const Widget& old_widget, const Widget& new_widget);

private:
    std::shared_ptr<const Key> key_;
};

class StatelessWidget : public Widget {
public:
    using Widget::Widget;
    [[nodiscard]] std::shared_ptr<Element> create_element() const override;
    virtual std::shared_ptr<Widget> build(BuildContext& context) const = 0;
};

class StateBase {
public:
    friend class StatefulElement;

    virtual ~StateBase() = default;

    [[nodiscard]] BuildContext& context() noexcept;
    [[nodiscard]] const Widget& widget() const noexcept;

    virtual void init_state() {}
    virtual void did_update_widget(const Widget& old_widget) { (void)old_widget; }
    virtual void dispose() {}
    virtual std::shared_ptr<Widget> build(BuildContext& context) = 0;

    void set_state(std::function<void()> fn);

protected:
    Element* element_{nullptr};
    const Widget* widget_{nullptr};
};

template <typename T>
class State : public StateBase {
public:
    [[nodiscard]] const T& widget() const noexcept {
        return *static_cast<const T*>(widget_);
    }
};

class StatefulWidget : public Widget {
public:
    using Widget::Widget;
    [[nodiscard]] std::shared_ptr<Element> create_element() const override;
    [[nodiscard]] virtual std::unique_ptr<StateBase> create_state() const = 0;
};

class RenderObjectWidget : public Widget {
public:
    using Widget::Widget;
    [[nodiscard]] virtual std::unique_ptr<RenderObject> create_render_object(BuildContext& context) const = 0;
    virtual void update_render_object(BuildContext& context, RenderObject& render_object) const = 0;
};

} // namespace pingdart::ui
