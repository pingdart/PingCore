#include <gtest/gtest.h>
#include <pingdart/ui/box_constraints.h>
#include <pingdart/ui/layout_primitives.h>
#include <pingdart/ui/leaf_widgets.h>
#include <pingdart/ui/widget_tree.h>

using namespace pingdart::ui;

TEST(LayoutConstraintsTest, BoxConstraintsMathematicalContracts) {
    BoxConstraints c(10.0f, 100.0f, 20.0f, 200.0f);
    EXPECT_TRUE(c.is_normalized());
    EXPECT_FALSE(c.is_tight());
    EXPECT_TRUE(c.is_bounded());

    // Tight constraints
    auto tight = BoxConstraints::tight(Size{50.0f, 80.0f});
    EXPECT_TRUE(tight.is_tight());
    EXPECT_EQ(tight.min_width, 50.0f);
    EXPECT_EQ(tight.max_width, 50.0f);
    EXPECT_EQ(tight.min_height, 80.0f);
    EXPECT_EQ(tight.max_height, 80.0f);

    // Deflation
    EdgeInsets insets = EdgeInsets::from_ltrb(10.0f, 15.0f, 20.0f, 25.0f);
    EXPECT_EQ(insets.horizontal(), 30.0f);
    EXPECT_EQ(insets.vertical(), 40.0f);

    auto deflated = tight.deflate(insets);
    EXPECT_EQ(deflated.min_width, 20.0f);
    EXPECT_EQ(deflated.max_width, 20.0f);
    EXPECT_EQ(deflated.min_height, 40.0f);
    EXPECT_EQ(deflated.max_height, 40.0f);

    // Clamping / constrain
    EXPECT_EQ(c.constrain(Size{5.0f, 250.0f}), (Size{10.0f, 200.0f}));
    EXPECT_EQ(c.constrain(Size{50.0f, 50.0f}), (Size{50.0f, 50.0f}));
}

TEST(LayoutConstraintsTest, SizedBoxLayoutExecution) {
    WidgetTree tree;
    auto sized_box = std::make_shared<SizedBox>(
        150.0f, 75.0f,
        std::make_shared<Container>(BoxConstraints::infinity, BoxConstraints::infinity,
                                   Color::from_argb(255, 255, 0, 0))
    );
    tree.set_root_widget(sized_box);
    tree.flush_layout(Size{800.0f, 600.0f});

    auto* ro = tree.root_render_box();
    ASSERT_NE(ro, nullptr);
    EXPECT_EQ(ro->size().width, 150.0f);
    EXPECT_EQ(ro->size().height, 75.0f);
}

TEST(LayoutConstraintsTest, PaddingLayoutAndOffsets) {
    WidgetTree tree;
    EdgeInsets insets = EdgeInsets::from_ltrb(12.0f, 16.0f, 20.0f, 24.0f);

    auto padding_widget = std::make_shared<Padding>(
        insets,
        std::make_shared<SizedBox>(100.0f, 50.0f)
    );

    tree.set_root_widget(padding_widget);
    tree.flush_layout(Size{500.0f, 500.0f}, /*tight=*/false);

    auto* ro = dynamic_cast<RenderPadding*>(tree.root_render_box());
    ASSERT_NE(ro, nullptr);

    EXPECT_EQ(ro->size().width, 100.0f + insets.horizontal());
    EXPECT_EQ(ro->size().height, 50.0f + insets.vertical());

    auto* child = ro->child();
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->size().width, 100.0f);
    EXPECT_EQ(child->size().height, 50.0f);
    EXPECT_EQ(child->offset().dx, 12.0f);
    EXPECT_EQ(child->offset().dy, 16.0f);

    // Tight constraints layout: expands to fill window while child maintains its tight size
    tree.flush_layout(Size{500.0f, 500.0f}, /*tight=*/true);
    EXPECT_EQ(ro->size().width, 500.0f);
    EXPECT_EQ(ro->size().height, 500.0f);
    EXPECT_EQ(child->size().width, 100.0f);
    EXPECT_EQ(child->size().height, 50.0f);
}

TEST(LayoutConstraintsTest, CenterAndAlignmentOffsets) {
    WidgetTree tree;
    auto centered = std::make_shared<Center>(
        std::make_shared<SizedBox>(200.0f, 100.0f)
    );

    tree.set_root_widget(centered);
    tree.flush_layout(Size{800.0f, 600.0f}, /*tight=*/true);

    auto* ro = dynamic_cast<RenderPositionedBox*>(tree.root_render_box());
    ASSERT_NE(ro, nullptr);
    EXPECT_EQ(ro->size().width, 800.0f);
    EXPECT_EQ(ro->size().height, 600.0f);

    auto* child = ro->child();
    ASSERT_NE(child, nullptr);
    // (800 - 200) / 2 = 300, (600 - 100) / 2 = 250
    EXPECT_EQ(child->offset().dx, 300.0f);
    EXPECT_EQ(child->offset().dy, 250.0f);
}

TEST(LayoutConstraintsTest, FlexOnePassSolverWithExpanded) {
    WidgetTree tree;

    // Total width = 600. Non-flex item = 100. Remaining = 500.
    // Expanded flex=1 -> 1/5 of 500 = 100.
    // Expanded flex=4 -> 4/5 of 500 = 400.
    auto row = std::make_shared<Row>(
        std::vector<std::shared_ptr<const Widget>>{
            std::make_shared<SizedBox>(100.0f, 50.0f),
            std::make_shared<Expanded>(std::make_shared<SizedBox>(0.0f, 50.0f), 1),
            std::make_shared<Expanded>(std::make_shared<SizedBox>(0.0f, 50.0f), 4),
        },
        MainAxisAlignment::Start,
        CrossAxisAlignment::Center
    );

    tree.set_root_widget(row);
    tree.flush_layout(Size{600.0f, 100.0f}, /*tight=*/true);

    auto* ro = dynamic_cast<RenderFlex*>(tree.root_render_box());
    ASSERT_NE(ro, nullptr);
    EXPECT_EQ(ro->size().width, 600.0f);

    std::vector<RenderBox*> children;
    ro->visit_children([&](RenderObject& obj) {
        children.push_back(dynamic_cast<RenderBox*>(&obj));
    });

    ASSERT_EQ(children.size(), 3);
    EXPECT_EQ(children[0]->size().width, 100.0f);
    EXPECT_EQ(children[0]->offset().dx, 0.0f);

    EXPECT_EQ(children[1]->size().width, 100.0f);
    EXPECT_EQ(children[1]->offset().dx, 100.0f);

    EXPECT_EQ(children[2]->size().width, 400.0f);
    EXPECT_EQ(children[2]->offset().dx, 200.0f);

    // CrossAxisAlignment::Center on height 100 with child height 50 -> (100 - 50) / 2 = 25
    EXPECT_EQ(children[0]->offset().dy, 25.0f);
}

TEST(LayoutConstraintsTest, HitTestingNestedHierarchy) {
    WidgetTree tree;
    auto root = std::make_shared<Center>(
        std::make_shared<SizedBox>(
            100.0f, 100.0f,
            std::make_shared<Container>(100.0f, 100.0f, Color::from_argb(255, 0, 0, 255))
        )
    );
    tree.set_root_widget(root);
    tree.flush_layout(Size{400.0f, 400.0f});

    // Center is 400x400, child is at [150, 150] to [250, 250]
    auto result_inside = tree.hit_test(Offset{200.0f, 200.0f});
    EXPECT_FALSE(result_inside.empty());

    auto result_outside = tree.hit_test(Offset{50.0f, 50.0f});
    // Hits root Center container, but not child SizedBox/Container
    bool hit_sized_box = false;
    for (const auto& entry : result_outside.path()) {
        if (dynamic_cast<RenderConstrainedBox*>(entry.target)) {
            hit_sized_box = true;
        }
    }
    EXPECT_FALSE(hit_sized_box);
}

TEST(LayoutConstraintsTest, DisplayListGeneration) {
    WidgetTree tree;
    auto root = std::make_shared<Container>(
        200.0f, 100.0f,
        Color::from_argb(255, 15, 23, 42),
        EdgeInsets::all(8.0f),
        8.0f,
        std::make_shared<Text>("PingDart UI", 16.0f, Color::from_argb(255, 255, 255, 255))
    );

    tree.set_root_widget(root);
    tree.flush_layout(Size{800.0f, 600.0f});

    auto dl = tree.flush_paint();
    ASSERT_NE(dl, nullptr);
    EXPECT_GT(dl->op_count(), 0);
}

TEST(LayoutConstraintsTest, ExpandedUnderUnboundedMainAxisDoesNotProduceInfinityOrNaN) {
    WidgetTree tree;
    auto row = std::make_shared<Row>(std::vector<std::shared_ptr<const Widget>>{
        std::make_shared<SizedBox>(50.0f, 30.0f),
        std::make_shared<Expanded>(std::make_shared<SizedBox>(20.0f, 30.0f), 1)
    });

    tree.set_root_widget(row);
    auto* rb = tree.root_render_box();
    ASSERT_NE(rb, nullptr);

    // Layout under unbounded width constraint [0, infinity]
    BoxConstraints unbounded(0.0f, BoxConstraints::infinity, 0.0f, 100.0f);
    rb->layout(unbounded);

    EXPECT_FALSE(std::isinf(rb->size().width));
    EXPECT_FALSE(std::isnan(rb->size().width));
    EXPECT_FALSE(std::isinf(rb->size().height));
    EXPECT_FALSE(std::isnan(rb->size().height));
    EXPECT_GE(rb->size().width, 0.0f);
    EXPECT_GE(rb->size().height, 0.0f);
}
