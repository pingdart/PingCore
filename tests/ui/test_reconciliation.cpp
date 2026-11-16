#include <gtest/gtest.h>
#include <pingdart/ui/widget.h>
#include <pingdart/ui/element.h>
#include <pingdart/ui/layout_primitives.h>
#include <pingdart/ui/leaf_widgets.h>
#include <pingdart/ui/key.h>

using namespace pingdart::ui;

namespace {

class KeyedItemState;

class KeyedItemWidget : public StatefulWidget {
public:
    KeyedItemWidget(std::string id, int value, std::shared_ptr<const Key> key)
        : StatefulWidget(std::move(key)), id_(std::move(id)), value_(value) {}

    [[nodiscard]] std::string_view widget_type() const noexcept override { return "KeyedItemWidget"; }

    [[nodiscard]] std::unique_ptr<StateBase> create_state() const override;

    [[nodiscard]] const std::string& id() const noexcept { return id_; }
    [[nodiscard]] int value() const noexcept { return value_; }

private:
    std::string id_;
    int value_;
};

class KeyedItemState : public State<KeyedItemWidget> {
public:
    int local_state{0};
    int dispose_count{0};

    void init_state() override {
        local_state = widget().value();
    }

    void dispose() override {
        dispose_count++;
    }

    std::shared_ptr<Widget> build(BuildContext& context) override {
        (void)context;
        return std::make_shared<Text>(widget().id() + ":" + std::to_string(local_state));
    }
};

std::unique_ptr<StateBase> KeyedItemWidget::create_state() const {
    return std::make_unique<KeyedItemState>();
}

} // namespace

TEST(ReconciliationTest, KeyEqualityContracts) {
    StringKey k1("tab_profile");
    StringKey k2("tab_profile");
    StringKey k3("tab_settings");
    IntKey ik1(42);
    IntKey ik2(42);
    IntKey ik3(99);

    EXPECT_TRUE(k1.equals(k2));
    EXPECT_FALSE(k1.equals(k3));
    EXPECT_FALSE(k1.equals(ik1));

    EXPECT_TRUE(ik1.equals(ik2));
    EXPECT_FALSE(ik1.equals(ik3));

    auto pk1 = std::make_shared<StringKey>("item_1");
    auto pk2 = std::make_shared<StringKey>("item_1");
    auto pk3 = std::make_shared<StringKey>("item_2");

    EXPECT_TRUE(keys_match(pk1.get(), pk2.get()));
    EXPECT_FALSE(keys_match(pk1.get(), pk3.get()));
    EXPECT_FALSE(keys_match(pk1.get(), nullptr));
    EXPECT_TRUE(keys_match(nullptr, nullptr));
}

TEST(ReconciliationTest, SequentialUnkeyedDiffing) {
    auto row1 = std::make_shared<Row>(std::vector<std::shared_ptr<const Widget>>{
        std::make_shared<Text>("Item A"),
        std::make_shared<Text>("Item B")
    });

    auto elem = std::static_pointer_cast<MultiChildRenderObjectElement>(row1->create_element());
    elem->mount(nullptr);

    ASSERT_EQ(elem->children().size(), 2);
    auto child0_before = elem->children()[0];
    auto child1_before = elem->children()[1];

    auto row2 = std::make_shared<Row>(std::vector<std::shared_ptr<const Widget>>{
        std::make_shared<Text>("Updated A"),
        std::make_shared<Text>("Updated B")
    });

    elem->update(row2);

    ASSERT_EQ(elem->children().size(), 2);
    // Elements should be reused in place
    EXPECT_EQ(elem->children()[0], child0_before);
    EXPECT_EQ(elem->children()[1], child1_before);

    elem->unmount();
}

TEST(ReconciliationTest, KeyedReorderingPreservesElementState) {
    auto key_a = std::make_shared<StringKey>("A");
    auto key_b = std::make_shared<StringKey>("B");
    auto key_c = std::make_shared<StringKey>("C");

    auto initial_row = std::make_shared<Row>(std::vector<std::shared_ptr<const Widget>>{
        std::make_shared<KeyedItemWidget>("A", 100, key_a),
        std::make_shared<KeyedItemWidget>("B", 200, key_b),
        std::make_shared<KeyedItemWidget>("C", 300, key_c),
    });

    auto elem = std::static_pointer_cast<MultiChildRenderObjectElement>(initial_row->create_element());
    elem->mount(nullptr);

    ASSERT_EQ(elem->children().size(), 3);
    auto elem_a = elem->children()[0];
    auto elem_b = elem->children()[1];
    auto elem_c = elem->children()[2];

    auto* state_a = dynamic_cast<KeyedItemState*>(dynamic_cast<StatefulElement*>(elem_a.get())->state());
    auto* state_b = dynamic_cast<KeyedItemState*>(dynamic_cast<StatefulElement*>(elem_b.get())->state());
    auto* state_c = dynamic_cast<KeyedItemState*>(dynamic_cast<StatefulElement*>(elem_c.get())->state());

    ASSERT_NE(state_a, nullptr);
    ASSERT_NE(state_b, nullptr);
    ASSERT_NE(state_c, nullptr);

    // Modify local state on B
    state_b->local_state = 999;

    // Reorder: [C, A, B]
    auto reordered_row = std::make_shared<Row>(std::vector<std::shared_ptr<const Widget>>{
        std::make_shared<KeyedItemWidget>("C", 300, key_c),
        std::make_shared<KeyedItemWidget>("A", 100, key_a),
        std::make_shared<KeyedItemWidget>("B", 200, key_b),
    });

    elem->update(reordered_row);

    ASSERT_EQ(elem->children().size(), 3);
    EXPECT_EQ(elem->children()[0], elem_c);
    EXPECT_EQ(elem->children()[1], elem_a);
    EXPECT_EQ(elem->children()[2], elem_b);

    // Verify local state was preserved and NOT reset
    EXPECT_EQ(state_b->local_state, 999);
    EXPECT_EQ(state_b->dispose_count, 0);

    elem->unmount();
    EXPECT_EQ(state_b->dispose_count, 1);
}

TEST(ReconciliationTest, InsertionAndDeletionDisposesUnusedElements) {
    auto key_a = std::make_shared<StringKey>("A");
    auto key_b = std::make_shared<StringKey>("B");
    auto key_c = std::make_shared<StringKey>("C");
    auto key_d = std::make_shared<StringKey>("D");

    auto list_v1 = std::make_shared<Column>(std::vector<std::shared_ptr<const Widget>>{
        std::make_shared<KeyedItemWidget>("A", 1, key_a),
        std::make_shared<KeyedItemWidget>("B", 2, key_b),
        std::make_shared<KeyedItemWidget>("C", 3, key_c),
    });

    auto elem = std::static_pointer_cast<MultiChildRenderObjectElement>(list_v1->create_element());
    elem->mount(nullptr);

    auto elem_b = elem->children()[1];
    auto* state_b = dynamic_cast<KeyedItemState*>(dynamic_cast<StatefulElement*>(elem_b.get())->state());
    ASSERT_NE(state_b, nullptr);

    // Replace B with D: [A, D, C]
    auto list_v2 = std::make_shared<Column>(std::vector<std::shared_ptr<const Widget>>{
        std::make_shared<KeyedItemWidget>("A", 1, key_a),
        std::make_shared<KeyedItemWidget>("D", 4, key_d),
        std::make_shared<KeyedItemWidget>("C", 3, key_c),
    });

    elem->update(list_v2);

    ASSERT_EQ(elem->children().size(), 3);
    EXPECT_EQ(state_b->dispose_count, 1); // B unmounted and disposed

    auto elem_d = elem->children()[1];
    auto* state_d = dynamic_cast<KeyedItemState*>(dynamic_cast<StatefulElement*>(elem_d.get())->state());
    ASSERT_NE(state_d, nullptr);
    EXPECT_EQ(state_d->local_state, 4);

    elem->unmount();
}

TEST(ReconciliationTest, SameKeyDifferentWidgetTypeReplacesElement) {
    auto key = std::make_shared<StringKey>("unique_slot");

    auto row_v1 = std::make_shared<Row>(std::vector<std::shared_ptr<const Widget>>{
        std::make_shared<KeyedItemWidget>("WidgetType1", 42, key)
    });

    auto elem = std::static_pointer_cast<MultiChildRenderObjectElement>(row_v1->create_element());
    elem->mount(nullptr);

    auto child_v1 = elem->children()[0];
    auto* state_v1 = dynamic_cast<KeyedItemState*>(dynamic_cast<StatefulElement*>(child_v1.get())->state());
    ASSERT_NE(state_v1, nullptr);

    // Replace with Text having the exact same key
    auto row_v2 = std::make_shared<Row>(std::vector<std::shared_ptr<const Widget>>{
        std::make_shared<Text>("WidgetType2", 14.0f, Color::from_argb(255, 255, 255, 255), "sans-serif", false, false, key)
    });

    elem->update(row_v2);

    ASSERT_EQ(elem->children().size(), 1);
    auto child_v2 = elem->children()[0];
    // Element must be replaced because widget types differ
    EXPECT_NE(child_v1, child_v2);
    // Old state must have been disposed
    EXPECT_EQ(state_v1->dispose_count, 1);

    elem->unmount();
}

TEST(ReconciliationTest, DisposedStateRejectsSetState) {
    auto key = std::make_shared<StringKey>("target");
    auto widget = std::make_shared<KeyedItemWidget>("Target", 10, key);
    auto elem = std::static_pointer_cast<StatefulElement>(widget->create_element());
    elem->mount(nullptr);

    auto* state = dynamic_cast<KeyedItemState*>(elem->state());
    ASSERT_NE(state, nullptr);

    // Unmount element
    elem->unmount();
    EXPECT_FALSE(elem->mounted());

    // set_state called after disposal must be safely rejected
    state->set_state([state]() {
        state->local_state = 9999;
    });

    EXPECT_FALSE(elem->dirty());
}

TEST(ReconciliationTest, DuplicateSiblingKeysRejectedDeterministically) {
    auto dup_key = std::make_shared<StringKey>("duplicate");

    auto row = std::make_shared<Row>(std::vector<std::shared_ptr<const Widget>>{
        std::make_shared<KeyedItemWidget>("Item1", 1, dup_key),
        std::make_shared<KeyedItemWidget>("Item2", 2, dup_key)
    });

    auto elem = std::static_pointer_cast<MultiChildRenderObjectElement>(row->create_element());
    // Enforce sibling-key uniqueness: must throw std::invalid_argument deterministically
    EXPECT_THROW(elem->mount(nullptr), std::invalid_argument);
}

