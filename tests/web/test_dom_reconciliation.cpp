#include <gtest/gtest.h>
#include <pingdart/web/semantic_widget.h>
#include <pingdart/web/hydration_registry.h>
#include "../../src/web/dom/mock_dom_adapter.h"
#include "../../src/web/dom/dom_reconciler.h"

using namespace pingdart::web;

TEST(DomReconciliationTest, SsrNodeReuseDuringHydration) {
    MockDomAdapter adapter;
    HydrationRegistry registry;

    // Simulate pre-rendered SSR DOM elements already in page
    DomNodeHandle ssr_main = adapter.create_element("main");
    adapter.set_attribute(ssr_main, "data-pd-id", "pd_root_k_main");

    DomNodeHandle ssr_heading = adapter.create_element("h1");
    adapter.set_attribute(ssr_heading, "data-pd-id", "pd_root_k_main_0");
    adapter.append_child(ssr_main, ssr_heading);

    DomNodeHandle ssr_button = adapter.create_element("button");
    adapter.set_attribute(ssr_button, "data-pd-id", "pd_root_k_main_1");
    adapter.append_child(ssr_main, ssr_button);

    // Register SSR elements in hydration registry
    registry.register_ssr_node("pd_root_k_main", ssr_main);
    registry.register_ssr_node("pd_root_k_main_0", ssr_heading);
    registry.register_ssr_node("pd_root_k_main_1", ssr_button);

    // Build matching client widget tree
    std::vector<std::unique_ptr<SemanticWidget>> children;
    children.push_back(std::make_unique<HeadingWidget>(1, "Find Doctors"));
    children.push_back(std::make_unique<ButtonWidget>("Book Consultation", "book_action"));
    MainWidget client_root(std::move(children), "main");

    // Perform hydration
    DomReconciler reconciler(adapter, registry);
    DomNodeHandle root_handle = reconciler.mount_or_hydrate(client_root, kInvalidDomNodeHandle);

    // Verify SSR nodes were REUSED, not recreated!
    EXPECT_EQ(root_handle, ssr_main);
    EXPECT_EQ(reconciler.nodes_reused_count(), 3u);
    EXPECT_EQ(reconciler.nodes_created_count(), 0u);
    EXPECT_TRUE(registry.is_hydrated(ssr_main));
    EXPECT_TRUE(registry.is_hydrated(ssr_button));

    // Verify event listeners were attached to the existing SSR button
    EXPECT_TRUE(adapter.has_listener(ssr_button, DomEventType::Click));
    EXPECT_EQ(reconciler.listeners_attached_count(), 1u);
}

TEST(DomReconciliationTest, FreshClientMountingWithoutSsr) {
    MockDomAdapter adapter;
    HydrationRegistry registry;

    std::vector<std::unique_ptr<SemanticWidget>> children;
    children.push_back(std::make_unique<ParagraphWidget>("Welcome"));
    MainWidget root(std::move(children), "fresh_root");

    DomReconciler reconciler(adapter, registry);
    DomNodeHandle root_handle = reconciler.mount_or_hydrate(root, kInvalidDomNodeHandle);

    EXPECT_TRUE(root_handle.is_valid());
    EXPECT_EQ(reconciler.nodes_reused_count(), 0u);
    EXPECT_EQ(reconciler.nodes_created_count(), 2u);
    EXPECT_EQ(adapter.get_tag_name(root_handle), "main");
}

TEST(DomReconciliationTest, UnmountCleansUpListenersAndNode) {
    MockDomAdapter adapter;
    HydrationRegistry registry;

    DomNodeHandle parent = adapter.create_element("div");
    DomNodeHandle button = adapter.create_element("button");
    adapter.append_child(parent, button);
    adapter.add_event_listener(button, DomEventType::Click);
    registry.track_listener(button, DomEventType::Click);

    EXPECT_TRUE(adapter.has_listener(button, DomEventType::Click));
    EXPECT_EQ(registry.active_listener_nodes_count(), 1u);

    DomReconciler reconciler(adapter, registry);
    reconciler.unmount(button, parent);

    // Button should be removed and listener released
    EXPECT_FALSE(adapter.has_listener(button, DomEventType::Click));
    EXPECT_EQ(registry.active_listener_nodes_count(), 0u);
    EXPECT_TRUE(adapter.get_children(parent).empty());
}
