// tests/ui/test_pkg138_reactive_runtime.cpp
// PingDartUI PKG-138 Reactive Runtime, Reconciliation, Events & Gestures Acceptance Suite
#include <gtest/gtest.h>
#include <pingdart/ui.h>
#include <string>
#include <vector>
#include <memory>
#include <atomic>

using namespace pingdart::ui;

// 1. Keyed Component Identity & State Preservation Across Reordering
TEST(Pkg138ReactiveRuntimeTest, KeyedReconciliationPreservesComponentStateOnReorder) {
    // Child A has persistent state
    auto child_a = Component("div", "Item A").key("item-A");
    auto child_b = Component("div", "Item B").key("item-B");

    auto initial_tree = Component("div", {child_a, child_b});
    auto root_instance = Reconciler::mount(initial_tree, nullptr, 0, "root");

    ASSERT_EQ(root_instance->children().size(), 2u);
    auto inst_a = root_instance->children()[0];
    auto inst_b = root_instance->children()[1];
    EXPECT_EQ(inst_a->key(), "item-A");
    EXPECT_EQ(inst_b->key(), "item-B");

    // Attach state to instance A
    auto state_slot_a = inst_a->get_or_create_state<int>("counter", 42);
    EXPECT_EQ(*state_slot_a, 42);

    // Reorder children: [B, A]
    auto reordered_tree = Component("div", {child_b, child_a});
    auto next_instance = Reconciler::reconcile(root_instance, reordered_tree, 0, "root");

    ASSERT_EQ(next_instance->children().size(), 2u);
    EXPECT_EQ(next_instance->children()[0]->key(), "item-B");
    EXPECT_EQ(next_instance->children()[1]->key(), "item-A");

    // Verify instance A was preserved, not recreated, and its state is intact
    EXPECT_EQ(next_instance->children()[1], inst_a);
    auto retained_slot = next_instance->children()[1]->get_or_create_state<int>("counter", 0);
    EXPECT_EQ(*retained_slot, 42);
}

// 2. React-like Hooks: useState & useSignal
TEST(Pkg138ReactiveRuntimeTest, UseStateAndUseSignalReactivity) {
    auto state = useState<int>(10);
    EXPECT_EQ(state.get(), 10);

    bool listener_called = false;
    state.subscribe([&listener_called](int new_val) {
        if (new_val == 20) listener_called = true;
    });

    state.set(20);
    EXPECT_EQ(state.get(), 20);
    EXPECT_TRUE(listener_called);

    // useSignal with string
    auto sig = useSignal<std::string>("Initial Signal");
    EXPECT_EQ(sig.get(), "Initial Signal");
    sig.set("Updated Signal");
    EXPECT_EQ(sig.get(), "Updated Signal");
}

// 3. Lifecycle Hooks: onMount, onUnmount & useEffect Deterministic Cleanup
TEST(Pkg138ReactiveRuntimeTest, LifecycleHooksAndDeterministicCleanup) {
    std::atomic<bool> mounted{false};
    std::atomic<bool> cleaned_up{false};

    auto inst = std::make_shared<ComponentInstance>("test-node", "div");
    inst->add_mount_callback([&mounted]() {
        mounted = true;
    });
    inst->add_cleanup_callback([&cleaned_up]() {
        cleaned_up = true;
    });

    inst->mount();
    EXPECT_TRUE(mounted);
    EXPECT_FALSE(cleaned_up);

    // Explicit unmount
    inst->unmount();
    EXPECT_TRUE(cleaned_up);
}

// 4. Shared Context Propagation: createContext & useContext
TEST(Pkg138ReactiveRuntimeTest, SharedContextPropagation) {
    struct UserSession {
        std::string username;
        std::string role;
    };

    auto UserContext = createContext<UserSession>({"guest", "anonymous"});
    EXPECT_EQ(useContext(UserContext).username, "guest");

    UserContext.provide({"dr_jane", "cardiologist"});
    EXPECT_EQ(useContext(UserContext).username, "dr_jane");
    EXPECT_EQ(useContext(UserContext).role, "cardiologist");
}

// 5. useReducer State Machine
TEST(Pkg138ReactiveRuntimeTest, UseReducerStateTransitions) {
    struct Action {
        std::string type;
        int delta;
    };

    auto reducer = [](const int& state, const Action& act) -> int {
        if (act.type == "INCREMENT") return state + act.delta;
        if (act.type == "DECREMENT") return state - act.delta;
        if (act.type == "RESET") return 0;
        return state;
    };

    auto handle = useReducer<int, Action>(reducer, 100);
    EXPECT_EQ(handle.state(), 100);

    handle.dispatch({"INCREMENT", 50});
    EXPECT_EQ(handle.state(), 150);

    handle.dispatch({"DECREMENT", 25});
    EXPECT_EQ(handle.state(), 125);

    handle.dispatch({"RESET", 0});
    EXPECT_EQ(handle.state(), 0);
}

// 6. Event Dispatcher: Capture, Target & Bubbling with stopPropagation
TEST(Pkg138ReactiveRuntimeTest, EventPropagationCaptureAndBubbling) {
    std::vector<std::string> log;

    auto parent_node = std::make_shared<ComponentInstance>("parent", "div");
    auto child_node = std::make_shared<ComponentInstance>("child", "button");
    child_node->set_parent(parent_node);
    parent_node->children().push_back(child_node);

    // Parent capture listener
    parent_node->add_listener(EventType::Click, [&log](Event& /*e*/) {
        log.push_back("parent_capture");
    }, true);

    // Parent bubble listener
    parent_node->add_listener(EventType::Click, [&log](Event& /*e*/) {
        log.push_back("parent_bubble");
    }, false);

    // Child target listener
    child_node->add_listener(EventType::Click, [&log](Event& /*e*/) {
        log.push_back("child_target");
    }, false);

    PointerEvent evt(EventType::Click, 100.0f, 200.0f);
    EventDispatcher::dispatch(evt, child_node);

    // Execution order must be: Capture (parent) -> Target (child) -> Bubble (parent)
    ASSERT_EQ(log.size(), 3u);
    EXPECT_EQ(log[0], "parent_capture");
    EXPECT_EQ(log[1], "child_target");
    EXPECT_EQ(log[2], "parent_bubble");

    // Test stopPropagation in target phase
    log.clear();
    child_node->clear_listeners();
    child_node->add_listener(EventType::Click, [&log](Event& e) {
        log.push_back("child_target_stop");
        e.stopPropagation();
    }, false);

    PointerEvent evt2(EventType::Click, 100.0f, 200.0f);
    EventDispatcher::dispatch(evt2, child_node);

    // Should stop before bubbling to parent
    ASSERT_EQ(log.size(), 2u);
    EXPECT_EQ(log[0], "parent_capture");
    EXPECT_EQ(log[1], "child_target_stop");
}

// 7. Gesture Recognition: Tap, LongPress, Swipe & Pan
TEST(Pkg138ReactiveRuntimeTest, GestureEventHandlers) {
    bool tapped = false;
    bool long_pressed = false;
    Direction swiped_dir = Direction::None;
    float pan_x = 0.0f;

    auto comp = Component("div")
        .onTap([&tapped] { tapped = true; })
        .onLongPress([&long_pressed] { long_pressed = true; })
        .onSwipe([&swiped_dir](Direction dir) { swiped_dir = dir; })
        .onPan([&pan_x](float dx, float /*dy*/) { pan_x = dx; });

    comp.emit_tap();
    EXPECT_TRUE(tapped);

    comp.emit_long_press();
    EXPECT_TRUE(long_pressed);

    comp.emit_swipe(Direction::Left);
    EXPECT_EQ(swiped_dir, Direction::Left);

    comp.emit_pan(12.5f, 0.0f);
    EXPECT_FLOAT_EQ(pan_x, 12.5f);
}

// 8. Scheduler Batching & Dirty-Node Queue
TEST(Pkg138ReactiveRuntimeTest, SchedulerBatchedUpdatesAndDirtyQueue) {
    Scheduler::instance().reset();

    auto inst1 = std::make_shared<ComponentInstance>("node-1", "div");
    inst1->set_depth(1);

    auto inst2 = std::make_shared<ComponentInstance>("node-2", "div");
    inst2->set_depth(2);

    int update_count = 0;
    Scheduler::instance().set_update_handler([&update_count](std::shared_ptr<ComponentInstance>) {
        update_count++;
    });

    // Execute in batch: only 1 flush occurs
    Scheduler::instance().batch([inst1, inst2]() {
        Scheduler::instance().mark_dirty(inst1);
        Scheduler::instance().mark_dirty(inst2);
        Scheduler::instance().mark_dirty(inst1); // duplicate ignored
    });

    EXPECT_EQ(update_count, 2);
    EXPECT_EQ(Scheduler::instance().flush_count(), 1u);
}

// 9. Form API: Explicit Names, Types & Secure Submission
TEST(Pkg138ReactiveRuntimeTest, FormExplicitFieldNamesAndSubmission) {
    FormData submitted_data;
    bool submitted = false;

    auto form = Form({
        FormField("Email", Input("email", "email", "Email Address", "test@hospital.org")),
        FormField("Password", Input("password", "password", "Password", "SecretPass123")),
        Button("Submit").attr("type", "submit")
    }).onSubmit([&submitted_data, &submitted](const FormData& data) {
        submitted_data = data;
        submitted = true;
    });

    FormData test_input;
    test_input.set("email", "test@hospital.org");
    test_input.set("password", "SecretPass123");

    form.emit_submit(test_input);
    EXPECT_TRUE(submitted);
    EXPECT_EQ(submitted_data.get("email"), "test@hospital.org");
    EXPECT_EQ(submitted_data.get("password"), "SecretPass123");
}

// 10. DataGrid: In-Memory Sorting, Filtering, and Pagination
TEST(Pkg138ReactiveRuntimeTest, DataGridSortingFilteringPagination) {
    std::vector<std::unordered_map<std::string, std::string>> sample = {
        {{"id", "1"}, {"name", "Dr. Bob"}, {"dept", "Neurology"}},
        {{"id", "2"}, {"name", "Dr. Alice"}, {"dept", "Cardiology"}},
        {{"id", "3"}, {"name", "Dr. Charlie"}, {"dept", "Cardiology"}}
    };

    auto grid = DataGrid()
        .columns({
            ColumnDef("id", "ID", true),
            ColumnDef("name", "Name", true),
            ColumnDef("dept", "Dept", true)
        })
        .data(sample)
        .sortBy("name", true)  // Sort A->Z: Alice, Bob, Charlie
        .filter("Cardiology")  // Only Cardiology: Alice, Charlie
        .pagination(1);        // 1 per page

    Component comp = grid.build();
    std::string html = render_to_html(comp);

    // Dr. Alice must be present (page 1)
    EXPECT_NE(html.find("Dr. Alice"), std::string::npos);
    // Dr. Bob should NOT be present (filtered out)
    EXPECT_EQ(html.find("Dr. Bob"), std::string::npos);
}

// 11. Memory Stress Test: 10,000 Mount/Unmount Cycles
TEST(Pkg138ReactiveRuntimeTest, LifecycleStressTenThousandCycles) {
    for (int i = 0; i < 10000; ++i) {
        auto vnode = Card({
            Heading("Stress Item " + std::to_string(i)),
            Text("Testing memory stability").color("#2563EB"),
            Button("Action").onClick([] {})
        });

        auto inst = Reconciler::mount(vnode, nullptr, 0, "stress-root");
        ASSERT_NE(inst, nullptr);
        ASSERT_EQ(inst->children().size(), 3u);

        // Immediate deterministic unmount & disposal
        inst->unmount();
    }
    // Verified 10,000 cycles completed with clean teardown
    SUCCEED();
}

// 12. Security Sanitization Hardening Regression
TEST(Pkg138ReactiveRuntimeTest, SecuritySanitizationHardening) {
    // Malicious URL schemes
    EXPECT_EQ(sanitize_url("javascript:alert(1)"), "#blocked-unsafe-url");
    EXPECT_EQ(sanitize_url("vbscript:msgbox(1)"), "#blocked-unsafe-url");
    EXPECT_EQ(sanitize_url("data:text/html,<script>"), "#blocked-unsafe-url");
    EXPECT_EQ(sanitize_url("file:///etc/passwd"), "#blocked-unsafe-url");

    // Safe URLs allowed
    EXPECT_EQ(sanitize_url("https://pingdart.dev/api"), "https://pingdart.dev/api");
    EXPECT_EQ(sanitize_url("/dashboard/patients"), "/dashboard/patients");

    // CSS value sanitization
    EXPECT_EQ(sanitize_css_value("expression(alert(1))"), "inherit");
    EXPECT_EQ(sanitize_css_value("behavior:url(x.htc)"), "inherit");
    EXPECT_EQ(sanitize_css_value("#2563EB"), "#2563EB");
}
