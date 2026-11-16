#include <pingdart/ui/widget.h>
#include <pingdart/ui/element.h>

namespace pingdart::ui {

bool Widget::can_update(const Widget& old_widget, const Widget& new_widget) {
    return (old_widget.widget_type() == new_widget.widget_type()) &&
           keys_match(old_widget.key().get(), new_widget.key().get());
}

std::shared_ptr<Element> StatelessWidget::create_element() const {
    return std::make_shared<StatelessElement>(
        std::static_pointer_cast<const StatelessWidget>(shared_from_this()));
}

BuildContext& StateBase::context() noexcept {
    return *element_;
}

const Widget& StateBase::widget() const noexcept {
    return *widget_;
}

void StateBase::set_state(std::function<void()> fn) {
    if (!element_ || !element_->mounted()) {
        return; // Reject set_state after disposal or when unmounted
    }
    if (fn) {
        fn();
    }
    element_->mark_needs_build();
}

std::shared_ptr<Element> StatefulWidget::create_element() const {
    return std::make_shared<StatefulElement>(
        std::static_pointer_cast<const StatefulWidget>(shared_from_this()));
}

} // namespace pingdart::ui
