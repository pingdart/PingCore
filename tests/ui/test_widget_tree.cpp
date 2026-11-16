#include <gtest/gtest.h>
#include <pingdart/ui/widget.h>
#include <pingdart/ui/element.h>
#include <pingdart/ui/widget_tree.h>
#include <pingdart/ui/layout_primitives.h>
#include <pingdart/ui/leaf_widgets.h>

using namespace pingdart::ui;

namespace {

class DummyStateless : public StatelessWidget {
public:
    explicit DummyStateless(std::string label, std::shared_ptr<const Key> key = nullptr)
        : StatelessWidget(std::move(key)), label_(std::move(label)) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "DummyStateless"; }

    std::shared_ptr<Widget> build(BuildContext& context) const override {
        (void)context;
        return std::make_shared<Text>(label_);
    }

private:
    std::string label_;
};

class CounterWidget : public StatefulWidget {
public:
    explicit CounterWidget(int initial_count,
                           std::shared_ptr<int> dispose_counter = nullptr,
                           std::shared_ptr<const Key> key = nullptr)
        : StatefulWidget(std::move(key)),
          initial_count_(initial_count),
          dispose_counter_(std::move(dispose_counter)) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "CounterWidget"; }

    [[nodiscard]] std::unique_ptr<StateBase> create_state() const override;

    [[nodiscard]] int initial_count() const noexcept { return initial_count_; }
    [[nodiscard]] const std::shared_ptr<int>& dispose_counter() const noexcept { return dispose_counter_; }

private:
    int initial_count_;
    std::shared_ptr<int> dispose_counter_;
};

class CounterState : public State<CounterWidget> {
public:
    int init_calls{0};
    int update_calls{0};
    int dispose_calls{0};
    int count{0};

    void init_state() override {
        init_calls++;
        count = widget().initial_count();
    }

    void did_update_widget(const Widget& old_widget) override {
        (void)old_widget;
        update_calls++;
    }

    void dispose() override {
        dispose_calls++;
        if (widget().dispose_counter()) {
            (*widget().dispose_counter())++;
        }
    }

    void increment() {
        set_state([this]() {
            count++;
        });
    }

    std::shared_ptr<Widget> build(BuildContext& context) override {
        (void)context;
        return std::make_shared<Text>("Count: " + std::to_string(count));
    }
};

std::unique_ptr<StateBase> CounterWidget::create_state() const {
    return std::make_unique<CounterState>();
}

} // namespace

TEST(WidgetTreeTest, MountsAndBuildsStatelessWidget) {
    WidgetTree tree;
    auto root = std::make_shared<DummyStateless>("Hello PingDart");
    tree.set_root_widget(root);

    ASSERT_NE(tree.root_element(), nullptr);
    EXPECT_TRUE(tree.root_element()->mounted());
    EXPECT_FALSE(tree.root_element()->dirty());

    auto* ro = tree.root_render_box();
    ASSERT_NE(ro, nullptr);
}

TEST(WidgetTreeTest, LifecycleStatefulWidget) {
    WidgetTree tree;
    auto dispose_count = std::make_shared<int>(0);
    auto counter = std::make_shared<CounterWidget>(10, dispose_count);
    tree.set_root_widget(counter);

    auto* root_elem = tree.root_element();
    ASSERT_NE(root_elem, nullptr);
    auto* stateful_elem = dynamic_cast<StatefulElement*>(root_elem);
    ASSERT_NE(stateful_elem, nullptr);

    auto* state = dynamic_cast<CounterState*>(stateful_elem->state());
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->init_calls, 1);
    EXPECT_EQ(state->count, 10);
    EXPECT_EQ(state->update_calls, 0);

    // Increment count using set_state
    state->increment();
    EXPECT_EQ(state->count, 11);
    EXPECT_TRUE(stateful_elem->dirty());

    // Flush build
    tree.flush_build();
    EXPECT_FALSE(stateful_elem->dirty());

    // Update with new widget of same type
    auto updated_counter = std::make_shared<CounterWidget>(20, dispose_count);
    tree.set_root_widget(updated_counter);
    EXPECT_EQ(state->update_calls, 1);
    // Count remains 11 because state persisted
    EXPECT_EQ(state->count, 11);

    // Unmount tree
    tree.set_root_widget(nullptr);
    EXPECT_EQ(*dispose_count, 1);
    EXPECT_EQ(tree.root_element(), nullptr);
}

TEST(WidgetTreeTest, SelectiveRebuildScheduling) {
    WidgetTree tree;
    auto counter = std::make_shared<CounterWidget>(0);
    tree.set_root_widget(counter);

    auto* stateful_elem = dynamic_cast<StatefulElement*>(tree.root_element());
    ASSERT_NE(stateful_elem, nullptr);
    auto* state = dynamic_cast<CounterState*>(stateful_elem->state());
    ASSERT_NE(state, nullptr);

    EXPECT_FALSE(stateful_elem->dirty());

    tree.schedule_build_for(stateful_elem);
    state->increment();
    EXPECT_TRUE(stateful_elem->dirty());

    tree.flush_build();
    EXPECT_FALSE(stateful_elem->dirty());
    EXPECT_EQ(state->count, 1);
}
