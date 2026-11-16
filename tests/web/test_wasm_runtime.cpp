#include <gtest/gtest.h>
#include "../../src/web/wasm/wasm_runtime.h"

using namespace pingdart::web;

TEST(WasmRuntimeTest, LifecycleAndHydrationIdentity) {
    WasmRuntime runtime;
    EXPECT_FALSE(runtime.is_initialized());

    runtime.initialize();
    EXPECT_TRUE(runtime.is_initialized());

    auto root = std::make_unique<MainWidget>(
        std::vector<std::unique_ptr<SemanticWidget>>{},
        "app_root"
    );
    runtime.set_root_widget(std::move(root));

    // Matching server ID verifies hydration
    EXPECT_TRUE(runtime.verify_hydration_identity("pd_root_k_app_root"));

    // Mismatched ID fails hydration verification
    EXPECT_FALSE(runtime.verify_hydration_identity("pd_root_k_wrong_root"));

    runtime.shutdown();
    EXPECT_FALSE(runtime.is_initialized());
}

TEST(WasmRuntimeTest, EventDispatchingAndActionBinding) {
    WasmRuntime runtime;
    runtime.initialize();

    std::string handled_target;
    std::string handled_event;
    bool action_invoked = false;

    runtime.register_action_handler("submit_btn_action", [&](const std::string& target_id, const std::string& event_type) {
        handled_target = target_id;
        handled_event = event_type;
        action_invoked = true;
    });

    // Dispatch DOM event from button
    bool dispatched = runtime.dispatch_dom_event("pd_0_2", "click", "submit_btn_action");
    EXPECT_TRUE(dispatched);
    EXPECT_TRUE(action_invoked);
    EXPECT_EQ(handled_target, "pd_0_2");
    EXPECT_EQ(handled_event, "click");
    EXPECT_EQ(runtime.dispatch_count(), 1u);

    // Unregistered action returns false
    bool unknown_dispatched = runtime.dispatch_dom_event("pd_0_3", "click", "non_existent_action");
    EXPECT_FALSE(unknown_dispatched);
}

TEST(WasmRuntimeTest, CAbiExportBoundary) {
    void* handle = pd_wasm_runtime_create();
    ASSERT_NE(handle, nullptr);

    int init_ok = pd_wasm_runtime_init(handle);
    EXPECT_EQ(init_ok, 1);

    pd_wasm_runtime_step(handle, 16.66);

    int dispatch_res = pd_wasm_runtime_dispatch_event(handle, "pd_btn", "click", nullptr);
    EXPECT_EQ(dispatch_res, 0); // No handler registered

    pd_wasm_runtime_destroy(handle);
}
