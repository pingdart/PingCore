// tests/native/test_pkg124_fullstack_runtime.cpp
// Comprehensive test suite for PKG-124: Lightning-Fast Full-Stack Runtime,
// Reactivity Core, Full-Stack Query Engine, Async Server SSE/WebSockets,
// Browser Hydration & Incremental Hot Reload with State Preservation
#include <gtest/gtest.h>
#include <pingcore/reactive.h>
#include <pingcore/ui.h>
#include <pingcore/query.h>
#include <pingcore/hot_reload.h>
#include <pingcore/hydration.h>
#include <pingcore/server.h>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>

using namespace pingcore;

class Pkg124FullStackRuntimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        QueryCache::instance().clear();
        HotReloadManager::instance().clear_state();
        HydrationEngine::instance().clear();
        WebSocketBroadcaster::instance().clear();
    }

    void TearDown() override {
        QueryCache::instance().clear();
        HotReloadManager::instance().clear_state();
        HydrationEngine::instance().clear();
        WebSocketBroadcaster::instance().clear();
    }
};

// =========================================================================
// Category 1: Unit Tests (Reactivity, Query Cache, Server SSE & WebSockets)
// =========================================================================

// 1. Signals & Computeds Dependency Tracking
TEST_F(Pkg124FullStackRuntimeTest, SignalReactivityAndComputedTracking) {
    auto count = useSignal<int>(10);
    auto multiplier = useSignal<int>(2);

    // Computed derived value: count * multiplier
    auto product = useComputed<int>([count, multiplier]() {
        return count.get() * multiplier.get();
    });

    EXPECT_EQ(count.get(), 10);
    EXPECT_EQ(multiplier.get(), 2);
    EXPECT_EQ(product.get(), 20);

    // Update signal: product updates automatically
    count.set(15);
    EXPECT_EQ(product.get(), 30);

    multiplier.set(4);
    EXPECT_EQ(product.get(), 60);
}

// 2. Batched State Updates
TEST_F(Pkg124FullStackRuntimeTest, BatchedStateUpdates) {
    auto a = useSignal<int>(1);
    auto b = useSignal<int>(2);

    std::atomic<int> effect_run_count{0};
    auto effect = create_effect([a, b, &effect_run_count]() {
        (void)a.get();
        (void)b.get();
        effect_run_count.fetch_add(1);
    });

    // Initial run
    EXPECT_EQ(effect_run_count.load(), 1);

    // Run batch update: modifying both signals must only trigger effect once
    batch([&]() {
        a.set(10);
        b.set(20);
    });

    EXPECT_EQ(effect_run_count.load(), 2);
    EXPECT_EQ(a.get(), 10);
    EXPECT_EQ(b.get(), 20);
}

// 3. Controlled Reactive Side Effects (Effects)
TEST_F(Pkg124FullStackRuntimeTest, ControlledSideEffects) {
    auto username = useSignal<std::string>("Alice");
    std::string captured_greeting;

    auto effect = create_effect([username, &captured_greeting]() {
        captured_greeting = "Hello, " + username.get() + "!";
    });

    EXPECT_EQ(captured_greeting, "Hello, Alice!");

    username.set("Bob");
    EXPECT_EQ(captured_greeting, "Hello, Bob!");
}

// 4. Server-Side Events (SSE) & WebSocket Handshake / Broadcasting
TEST_F(Pkg124FullStackRuntimeTest, AsyncServerSseAndWebSockets) {
    // A. Server-Sent Events (SSE)
    Response sse_res;
    sse_res.sse("{\"user\":\"test\"}", "user_joined");

    EXPECT_EQ(sse_res.status_code(), 200);
    EXPECT_EQ(sse_res.headers().at("Content-Type"), "text/event-stream; charset=utf-8");
    EXPECT_NE(sse_res.body().find("event: user_joined\n"), std::string::npos);
    EXPECT_NE(sse_res.body().find("data: {\"user\":\"test\"}\n\n"), std::string::npos);

    // B. WebSocket Handshake Detection
    Request ws_req;
    ws_req.set_header("Upgrade", "websocket");
    ws_req.set_header("Connection", "Upgrade");
    EXPECT_TRUE(is_websocket_handshake(ws_req));

    Request non_ws_req;
    non_ws_req.set_header("Upgrade", "");
    EXPECT_FALSE(is_websocket_handshake(non_ws_req));

    // C. WebSocket Pub/Sub Real-Time Broadcast
    std::string received_payload;
    WebSocketBroadcaster::instance().subscribe("live_orders", [&](const std::string& msg) {
        received_payload = msg;
    });

    WebSocketBroadcaster::instance().broadcast("live_orders", "{\"orderId\":9912,\"status\":\"completed\"}");
    EXPECT_NE(received_payload.find("\"orderId\":9912"), std::string::npos);
}

// =========================================================================
// Category 2: Integration Tests (Fine-Grained UI, Hydration, Queries & Hot Reload)
// =========================================================================

// 5. Fine-Grained Reactive Widget Invalidation (Text([count] {...}))
TEST_F(Pkg124FullStackRuntimeTest, FineGrainedReactiveTextWidget) {
    auto count = useSignal<int>(0);

    // Construct Text widget with reactive evaluator lambda
    Text reactive_text([count]() {
        return "Count: " + std::to_string(count.get());
    });

    EXPECT_TRUE(reactive_text.is_reactive());
    EXPECT_EQ(reactive_text.current_text(), "Count: 0");

    // Mutate state signal directly
    count.set(1);
    EXPECT_EQ(reactive_text.current_text(), "Count: 1");

    count.set(42);
    EXPECT_EQ(reactive_text.current_text(), "Count: 42");
}

// 6. Reactive Server Data: useQuery Caching & Invalidation
TEST_F(Pkg124FullStackRuntimeTest, ReactiveQueryCachingAndInvalidation) {
    std::atomic<int> fetch_call_count{0};

    auto query = useQuery<std::string>("users_list", [&fetch_call_count]() -> std::string {
        int call = fetch_call_count.fetch_add(1);
        return "{\"count\":" + std::to_string(call + 1) + "}";
    });

    EXPECT_TRUE(query->is_success());
    EXPECT_FALSE(query->loading());
    EXPECT_EQ(query->data(), "{\"count\":1}");
    EXPECT_EQ(fetch_call_count.load(), 1);

    // Invalidate query from server event or mutation
    invalidate_query("users_list");

    // Verify refetch occurred
    EXPECT_EQ(fetch_call_count.load(), 2);
    EXPECT_EQ(query->data(), "{\"count\":2}");
}

// 7. Browser Hydration: Embedded State Extraction & DOM Event Attachment
TEST_F(Pkg124FullStackRuntimeTest, BrowserHydrationAndDomReconciliation) {
    std::string server_html =
        "<!DOCTYPE html><html><body>"
        "<div id=\"app-root\">"
        "  <h1 id=\"title\">PingCore FullStack</h1>"
        "  <button id=\"counter_btn\">Increase</button>"
        "</div>"
        "<script id=\"__PINGCORE_DATA__\" type=\"application/json\">{\"count\":100,\"theme\":\"dark\"}</script>"
        "</body></html>";

    // A. Extract SSR embedded JSON state
    std::string embedded_state = HydrationEngine::extract_embedded_state(server_html);
    EXPECT_NE(embedded_state.find("\"count\":100"), std::string::npos);
    EXPECT_NE(embedded_state.find("\"theme\":\"dark\""), std::string::npos);

    // B. Hydrate interactive event listeners
    std::atomic<bool> button_clicked{false};
    HydrationReport report = HydrationEngine::instance().hydrate(server_html, {
        {"counter_btn", [&button_clicked]() {
            button_clicked.store(true);
        }}
    });

    EXPECT_TRUE(report.success);
    EXPECT_EQ(report.nodes_matched, 1u);
    EXPECT_EQ(report.listeners_attached, 1u);

    // C. Trigger event on hydrated element
    bool triggered = HydrationEngine::instance().trigger_event("counter_btn");
    EXPECT_TRUE(triggered);
    EXPECT_TRUE(button_clicked.load());
}

// 8. Incremental Hot Reload: Change Classification & State Preservation
TEST_F(Pkg124FullStackRuntimeTest, HotReloadClassificationAndStatePreservation) {
    // A. Classification of 3 reload levels
    EXPECT_EQ(HotReloadManager::classify_change("styles/theme.css"), ReloadLevel::Level1_StyleText);
    EXPECT_EQ(HotReloadManager::classify_change("public/logo.png"), ReloadLevel::Level1_StyleText);

    EXPECT_EQ(HotReloadManager::classify_change("app/dashboard/page.cpp"), ReloadLevel::Level2_Component);
    EXPECT_EQ(HotReloadManager::classify_change("components/Navbar.cpp"), ReloadLevel::Level2_Component);

    EXPECT_EQ(HotReloadManager::classify_change("platforms/android/build.gradle.kts"), ReloadLevel::Level3_FullRestart);
    EXPECT_EQ(HotReloadManager::classify_change("native/camera.cpp"), ReloadLevel::Level3_FullRestart);
    EXPECT_EQ(HotReloadManager::classify_change("pingcore.yaml"), ReloadLevel::Level3_FullRestart);

    // B. State Preservation across Fast Refresh
    HotReloadManager::instance().preserve_state("counter_signal_state", "{\"count\":77}");
    EXPECT_TRUE(HotReloadManager::instance().has_preserved_state("counter_signal_state"));

    std::string restored = HotReloadManager::instance().restore_state("counter_signal_state");
    EXPECT_EQ(restored, "{\"count\":77}");

    // C. Process file change event
    auto event = HotReloadManager::instance().process_file_change("app/page.cpp");
    EXPECT_EQ(event.level, ReloadLevel::Level2_Component);
    EXPECT_EQ(HotReloadManager::instance().reload_count(), 1u);
}
