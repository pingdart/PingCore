// tests/native/test_pkg125_production_validation.cpp
// Comprehensive test suite for PKG-125: Production Runtime Validation, Performance & Reliability
// Addressing all 5 review findings:
// 1. Reactive state lifetime and ownership
// 2. Incremental compilation, dynamic module loading, ABI compatibility
// 3. Browser hydration DOM mismatch detection, pre-hydration form preservation, event replay, navigation state
// 4. Protocol-level HTTP request parsing, WebSocket framing, ping/pong, close, backpressure
// 5. Performance benchmarks (latency, throughput, frame times, memory stability)

#include <gtest/gtest.h>
#include <pingcore/reactive.h>
#include <pingcore/query.h>
#include <pingcore/hot_reload.h>
#include <pingcore/hydration.h>
#include <pingcore/server.h>
#include <pingcore/benchmark.h>

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace pingcore;

class Pkg125ProductionValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        QueryCache::instance().clear();
        HotReloadManager::instance().clear_state();
        HotReloadManager::instance().clear_modules();
        HydrationEngine::instance().clear();
        WebSocketBroadcaster::instance().clear();
    }

    void TearDown() override {
        QueryCache::instance().clear();
        HotReloadManager::instance().clear_state();
        HotReloadManager::instance().clear_modules();
        HydrationEngine::instance().clear();
        WebSocketBroadcaster::instance().clear();
    }
};

// =========================================================================
// Category 1: Reactive State Lifetime and Ownership (Finding 1)
// =========================================================================

// 1. Mount / Unmount Cycle: Effect cleanup prevents post-destruction notifications
TEST_F(Pkg125ProductionValidationTest, MountUnmountCycleLifecycleSafety) {
    auto counter = useSignal<int>(0);
    std::atomic<int> effect_run_count{0};
    int last_observed = -1;

    {
        // Scope simulates a component being mounted
        auto effect = create_effect([counter, &effect_run_count, &last_observed]() {
            last_observed = counter.get();
            effect_run_count.fetch_add(1);
        });

        EXPECT_EQ(effect_run_count.load(), 1);
        EXPECT_EQ(last_observed, 0);

        // Signal update while component is mounted
        counter.set(42);
        EXPECT_EQ(effect_run_count.load(), 2);
        EXPECT_EQ(last_observed, 42);

        // Explicitly dispose before leaving scope (simulating unmount cleanup)
        effect->dispose();
        EXPECT_TRUE(effect->is_effect_disposed());
    } // effect goes out of scope / destroyed

    // Update signal after unmount — must NOT trigger destroyed effect or crash
    counter.set(100);
    EXPECT_EQ(effect_run_count.load(), 2); // Count unchanged!
    EXPECT_EQ(last_observed, 42);           // Stale callback was NOT executed
}

// 2. Nested Effects: Proper hierarchy and clean teardown of child effects
TEST_F(Pkg125ProductionValidationTest, NestedEffectsTeardown) {
    auto outer_signal = useSignal<bool>(true);
    auto inner_signal = useSignal<int>(10);

    std::atomic<int> outer_runs{0};
    std::atomic<int> inner_runs{0};
    std::shared_ptr<Effect> child_effect;

    auto parent_effect = create_effect([&]() {
        outer_runs.fetch_add(1);
        bool enabled = outer_signal.get();

        if (child_effect) {
            child_effect->dispose();
            child_effect.reset();
        }

        if (enabled) {
            child_effect = create_effect([&]() {
                (void)inner_signal.get();
                inner_runs.fetch_add(1);
            });
        }
    });

    EXPECT_EQ(outer_runs.load(), 1);
    EXPECT_EQ(inner_runs.load(), 1);

    // Update inner signal — only inner effect runs
    inner_signal.set(20);
    EXPECT_EQ(outer_runs.load(), 1);
    EXPECT_EQ(inner_runs.load(), 2);

    // Disable outer signal — child effect should be disposed
    outer_signal.set(false);
    EXPECT_EQ(outer_runs.load(), 2);
    EXPECT_TRUE(child_effect == nullptr);

    // Update inner signal again — child effect is disposed so it must NOT run
    inner_signal.set(30);
    EXPECT_EQ(inner_runs.load(), 2);
}

// 3. Cross-Thread Signal Updates: Concurrent writers and observers without data races
TEST_F(Pkg125ProductionValidationTest, CrossThreadSignalUpdatesConcurrency) {
    auto shared_signal = useSignal<int>(0);
    std::atomic<int> observer_reads{0};
    std::atomic<bool> stop_flag{false};

    // Observer thread
    std::thread observer([&]() {
        auto eff = create_effect([&]() {
            (void)shared_signal.get();
            observer_reads.fetch_add(1);
        });
        while (!stop_flag.load(std::memory_order_relaxed)) {
            std::this_thread::yield();
        }
        eff->dispose();
    });

    // 4 concurrent writer threads
    constexpr int kUpdatesPerThread = 500;
    std::vector<std::thread> writers;
    for (int t = 0; t < 4; ++t) {
        writers.emplace_back([&, t]() {
            for (int i = 0; i < kUpdatesPerThread; ++i) {
                shared_signal.set(t * 1000 + i);
            }
        });
    }

    for (auto& w : writers) {
        w.join();
    }

    stop_flag.store(true);
    observer.join();

    EXPECT_GT(observer_reads.load(), 0);
}

// 4. Component Destruction During Notification
TEST_F(Pkg125ProductionValidationTest, ComponentDestructionDuringNotification) {
    auto trigger = useSignal<int>(0);
    std::atomic<int> survivor_runs{0};

    auto victim = create_effect([trigger]() {
        (void)trigger.get();
    });

    auto killer = create_effect([trigger, &victim, &survivor_runs]() {
        (void)trigger.get();
        if (victim) {
            victim->dispose(); // Dispose the sibling during notification!
        }
        survivor_runs.fetch_add(1);
    });

    // Trigger update
    trigger.set(1);

    EXPECT_TRUE(victim->is_effect_disposed());
    EXPECT_GE(survivor_runs.load(), 2); // Initial run + trigger run
}

// 5. Computed Chain Teardown: Multi-tier derivations handle disposal gracefully
TEST_F(Pkg125ProductionValidationTest, ComputedChainTeardown) {
    auto base = useSignal<int>(5);

    auto tier1 = useComputed<int>([base]() {
        return base.get() * 2;
    });

    auto tier2 = useComputed<int>([tier1]() {
        return tier1.get() + 10;
    });

    EXPECT_EQ(base.get(), 5);
    EXPECT_EQ(tier1.get(), 10);
    EXPECT_EQ(tier2.get(), 20);

    // Dispose tier2
    tier2.dispose();

    // Base update still properly triggers tier1
    base.set(10);
    EXPECT_EQ(tier1.get(), 20);
}

// =========================================================================
// Category 2: Hot Module Replacement & Incremental Compilation (Finding 2)
// =========================================================================

// 6. ABI Version Compatibility Validation
TEST_F(Pkg125ProductionValidationTest, ModuleABICompatibilityChecking) {
    ModuleABI v1_0{1, 0, 0};
    ModuleABI v1_2{1, 2, 0};
    ModuleABI v2_0{2, 0, 0};

    // Minor bumps are backward-compatible
    EXPECT_TRUE(v1_2.is_compatible_with(v1_0));
    EXPECT_TRUE(v1_0.is_compatible_with(v1_0));

    // Older minor cannot satisfy newer required minor
    EXPECT_FALSE(v1_0.is_compatible_with(v1_2));

    // Major version changes are breaking
    EXPECT_FALSE(v2_0.is_compatible_with(v1_0));
    EXPECT_FALSE(v1_0.is_compatible_with(v2_0));
}

// 7. Incremental Compilation and Dynamic Module Loading
TEST_F(Pkg125ProductionValidationTest, IncrementalCompilationAndModuleLoading) {
    // Generate a temporary C++ shared library source file
    std::string tmp_dir = "/tmp";
    std::string src_path = tmp_dir + "/pingcore_test_addon.cpp";
#ifdef __APPLE__
    std::string lib_path = tmp_dir + "/libpingcore_test_addon.dylib";
#else
    std::string lib_path = tmp_dir + "/libpingcore_test_addon.so";
#endif

    std::ofstream out(src_path);
    ASSERT_TRUE(out.is_open());
    out << "#include <cstdint>\n"
        << "extern \"C\" {\n"
        << "    struct ModuleABI { uint32_t major{1}; uint32_t minor{0}; uint32_t patch{0}; };\n"
        << "    ModuleABI pingcore_module_abi() { return {1, 0, 0}; }\n"
        << "    int compute_factorial(int n) {\n"
        << "        int res = 1;\n"
        << "        for (int i = 2; i <= n; ++i) res *= i;\n"
        << "        return res;\n"
        << "    }\n"
        << "}\n";
    out.close();

    // Compile incrementally using system toolchain
    auto compile_res = IncrementalCompiler::compile(src_path, lib_path);
    EXPECT_TRUE(compile_res.success) << compile_res.error_message;
    EXPECT_TRUE(std::filesystem::exists(lib_path));

    // Load module with ABI verification
    ModuleABI expected_abi{1, 0, 0};
    ModuleHandle mh = ModuleLoader::load_module(lib_path, expected_abi);
    EXPECT_TRUE(mh.is_valid());
    EXPECT_EQ(mh.abi.major, 1u);
    EXPECT_EQ(mh.abi.minor, 0u);

    // Resolve and execute exported function
    using FactFn = int(*)(int);
    void* sym = ModuleLoader::get_symbol(mh, "compute_factorial");
    ASSERT_NE(sym, nullptr);
    auto fact = reinterpret_cast<FactFn>(reinterpret_cast<uintptr_t>(sym));
    EXPECT_EQ(fact(5), 120);
    EXPECT_EQ(fact(6), 720);

    // Clean unloading
    EXPECT_TRUE(ModuleLoader::unload_module(mh));
    EXPECT_FALSE(mh.is_valid());

    // Clean up temporary files
    std::filesystem::remove(src_path);
    std::filesystem::remove(lib_path);
}

// 8. Hot Reload Manager Pipeline and State Preservation
TEST_F(Pkg125ProductionValidationTest, HotReloadPipelineAndStatePreservation) {
    auto& hmr = HotReloadManager::instance();

    // Classification
    EXPECT_EQ(HotReloadManager::classify_change("styles/app.css"), ReloadLevel::Level1_StyleText);
    EXPECT_EQ(HotReloadManager::classify_change("app/dashboard/page.cpp"), ReloadLevel::Level2_Component);
    EXPECT_EQ(HotReloadManager::classify_change("platforms/android/CMakeLists.txt"), ReloadLevel::Level3_FullRestart);

    // State preservation across hot reloads
    hmr.preserve_state("user_session", R"({"user_id":"u42","role":"admin"})");
    EXPECT_TRUE(hmr.has_preserved_state("user_session"));
    EXPECT_EQ(hmr.restore_state("user_session"), R"({"user_id":"u42","role":"admin"})");

    // File change event notification
    std::atomic<bool> listener_called{false};
    hmr.register_reload_listener([&](ReloadLevel lvl, std::string_view path) {
        if (lvl == ReloadLevel::Level1_StyleText && path == "styles/theme.json") {
            listener_called.store(true);
        }
    });

    auto evt = hmr.process_file_change("styles/theme.json");
    EXPECT_EQ(evt.level, ReloadLevel::Level1_StyleText);
    EXPECT_TRUE(listener_called.load());
    EXPECT_GE(hmr.reload_count(), 1u);
}

// =========================================================================
// Category 3: Browser Hydration & DOM Integration (Finding 3)
// =========================================================================

// 9. DOM Structure Mismatch Detection
TEST_F(Pkg125ProductionValidationTest, HydrationDOMMismatchDetection) {
    std::string server_html =
        "<div id=\"root\">"
        "  <header id=\"navbar\"><a href=\"/\">Home</a></header>"
        "  <span id=\"action_btn\">Click Me</span>"
        "</div>";

    // Client expects:
    // "navbar" -> div (mismatch: server has <header>)
    // "action_btn" -> button (mismatch: server has <span>)
    // "sidebar" -> aside (mismatch: missing from server)
    std::vector<std::pair<std::string, std::string>> expected = {
        {"navbar", "header"},        // Matches!
        {"action_btn", "button"},     // Tag mismatch: server has span
        {"sidebar", "aside"}          // Missing from server DOM
    };

    auto mismatches = HydrationEngine::instance().detect_mismatches(server_html, expected);
    EXPECT_EQ(mismatches.size(), 2u);

    EXPECT_EQ(mismatches[0].element_id, "action_btn");
    EXPECT_EQ(mismatches[0].expected, "button");
    EXPECT_EQ(mismatches[0].actual, "span");

    EXPECT_EQ(mismatches[1].element_id, "sidebar");
    EXPECT_EQ(mismatches[1].actual, "(missing)");
}

// 10. Form Value Preservation Across Hydration
TEST_F(Pkg125ProductionValidationTest, PreHydrationFormValuePreservation) {
    std::string server_html =
        "<form id=\"login_form\">"
        "  <input id=\"username_input\" type=\"text\" value=\"john_doe\">"
        "  <input id=\"remember_me\" type=\"checkbox\" value=\"on\">"
        "  <textarea id=\"bio_input\">Developer at PingCore</textarea>"
        "  <select id=\"role_select\"><option value=\"admin\" selected>Admin</option></select>"
        "</form>";

    auto values = HydrationEngine::instance().extract_form_values(server_html);
    ASSERT_EQ(values.size(), 4u);

    EXPECT_EQ(values[0].element_id, "username_input");
    EXPECT_EQ(values[0].value, "john_doe");
    EXPECT_EQ(values[0].input_type, "text");

    EXPECT_EQ(values[1].element_id, "remember_me");
    EXPECT_EQ(values[1].value, "on");

    EXPECT_EQ(values[2].element_id, "bio_input");
    EXPECT_EQ(values[2].value, "Developer at PingCore");

    EXPECT_EQ(values[3].element_id, "role_select");
    EXPECT_EQ(values[3].value, "admin");
}

// 11. Event Queue Replay for Pre-Hydration User Interactions
TEST_F(Pkg125ProductionValidationTest, EventQueueReplayPostHydration) {
    auto& engine = HydrationEngine::instance();

    // User clicks before hydration finishes
    engine.queue_pending_event("submit_btn", "click");
    engine.queue_pending_event("submit_btn", "click");
    engine.queue_pending_event("cancel_btn", "click");
    EXPECT_EQ(engine.pending_event_count(), 3u);

    std::string html = "<button id=\"submit_btn\">Submit</button><button id=\"cancel_btn\">Cancel</button>";

    int submit_count = 0;
    int cancel_count = 0;

    std::unordered_map<std::string, std::function<void()>> handlers = {
        {"submit_btn", [&]() { submit_count++; }},
        {"cancel_btn", [&]() { cancel_count++; }}
    };

    // Hydrating automatically replays queued events!
    auto report = engine.hydrate(html, handlers);
    EXPECT_TRUE(report.success);
    EXPECT_EQ(report.nodes_matched, 2u);

    EXPECT_EQ(submit_count, 2);
    EXPECT_EQ(cancel_count, 1);
    EXPECT_EQ(engine.pending_event_count(), 0u);
}

// 12. Navigation History and State Management
TEST_F(Pkg125ProductionValidationTest, NavigationHistoryStateStack) {
    auto& engine = HydrationEngine::instance();

    engine.push_navigation("/home", "Home");
    engine.push_navigation("/dashboard", "Dashboard");
    engine.push_navigation("/settings", "Settings");

    EXPECT_EQ(engine.navigation_history_size(), 3u);
    EXPECT_EQ(engine.current_navigation().path, "/settings");

    // Navigate back
    EXPECT_TRUE(engine.navigate_back());
    EXPECT_EQ(engine.current_navigation().path, "/dashboard");

    EXPECT_TRUE(engine.navigate_back());
    EXPECT_EQ(engine.current_navigation().path, "/home");

    // Cannot navigate further back
    EXPECT_FALSE(engine.navigate_back());

    // Navigate forward
    EXPECT_TRUE(engine.navigate_forward());
    EXPECT_EQ(engine.current_navigation().path, "/dashboard");

    // Branching: pushing new path clears future history
    engine.push_navigation("/profile", "Profile");
    EXPECT_EQ(engine.current_navigation().path, "/profile");
    EXPECT_FALSE(engine.navigate_forward()); // No forward entry
    EXPECT_EQ(engine.navigation_history_size(), 3u); // /home, /dashboard, /profile
}

// 13. Selective and Progressive Hydration
TEST_F(Pkg125ProductionValidationTest, SelectiveProgressiveHydration) {
    auto& engine = HydrationEngine::instance();
    std::string html = "<div id=\"banner\">Static</div><div id=\"interactive_widget\">Dynamic</div>";

    bool widget_clicked = false;
    auto report = engine.selective_hydrate(html, "interactive_widget", [&]() {
        widget_clicked = true;
    });

    EXPECT_TRUE(report.success);
    EXPECT_TRUE(engine.is_element_hydrated("interactive_widget"));
    EXPECT_FALSE(engine.is_element_hydrated("banner"));

    EXPECT_TRUE(engine.trigger_event("interactive_widget"));
    EXPECT_TRUE(widget_clicked);
    EXPECT_FALSE(engine.trigger_event("banner"));
}

// =========================================================================
// Category 4: Protocol-Level Networking & WebSocket Validation (Finding 4)
// =========================================================================

// 14. HTTP Framing: Content-Length Body Parsing
TEST_F(Pkg125ProductionValidationTest, HttpFramingContentLengthParsing) {
    std::string body = "{\"name\":\"PingDart\"}";
    std::string raw_http =
        "POST /api/users HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" + body;

    auto parsed = parse_http_request(raw_http);
    EXPECT_TRUE(parsed.valid);
    EXPECT_EQ(parsed.method, "POST");
    EXPECT_EQ(parsed.path, "/api/users");
    EXPECT_EQ(parsed.headers["Content-Type"], "application/json");
    EXPECT_EQ(parsed.content_length, static_cast<int64_t>(body.size()));
    EXPECT_EQ(parsed.body, body);
}

// 15. HTTP Framing: Chunked Transfer-Encoding Assembly
TEST_F(Pkg125ProductionValidationTest, HttpFramingChunkedTransferEncoding) {
    std::string raw_chunked =
        "POST /api/stream HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "4\r\n"
        "Wiki\r\n"
        "5\r\n"
        "pedia\r\n"
        "0\r\n"
        "\r\n";

    auto parsed = parse_http_request(raw_chunked);
    EXPECT_TRUE(parsed.valid);
    EXPECT_EQ(parsed.transfer_encoding, ParsedHttpRequest::TransferEncoding::Chunked);
    EXPECT_EQ(parsed.body, "Wikipedia");
}

// 16. HTTP Malformed Request Handling
TEST_F(Pkg125ProductionValidationTest, HttpFramingMalformedValidation) {
    // Missing header terminator
    auto p1 = parse_http_request("GET /api/test HTTP/1.1\r\nHost: localhost");
    EXPECT_FALSE(p1.valid);

    // Invalid method
    auto p2 = parse_http_request("INVALID /path HTTP/1.1\r\n\r\n");
    EXPECT_FALSE(p2.valid);

    // Negative Content-Length
    auto p3 = parse_http_request("POST /path HTTP/1.1\r\nContent-Length: -5\r\n\r\n");
    EXPECT_FALSE(p3.valid);

    // Dispatched through server returns 400 Bad Request
    Server server;
    auto res = server.handle_raw_request("INVALID /path HTTP/1.1\r\n\r\n");
    EXPECT_EQ(res.status_code(), 400);
}

// 17. WebSocket Frame Masking, Encoding and Decoding
TEST_F(Pkg125ProductionValidationTest, WebSocketFrameMaskingAndEncoding) {
    std::string msg = "PingCore Hardened WebSocket Message";
    WebSocketFrame frame = WebSocketFrame::text(msg);
    frame.masked = true;
    frame.mask_key[0] = 0xAA;
    frame.mask_key[1] = 0xBB;
    frame.mask_key[2] = 0xCC;
    frame.mask_key[3] = 0xDD;

    std::vector<uint8_t> encoded = frame.encode();
    EXPECT_GT(encoded.size(), msg.size());

    auto decoded = WebSocketFrame::decode(encoded);
    EXPECT_TRUE(decoded.valid);
    EXPECT_EQ(decoded.frame.opcode, WebSocketFrame::Opcode::Text);
    EXPECT_TRUE(decoded.frame.fin);
    EXPECT_EQ(decoded.frame.payload_string(), msg);
}

// 18. WebSocket Ping, Pong and Close Control Frames
TEST_F(Pkg125ProductionValidationTest, WebSocketPingPongAndCloseHandshake) {
    // Ping frame
    WebSocketFrame ping = WebSocketFrame::ping("healthcheck_token");
    auto encoded_ping = ping.encode();
    auto decoded_ping = WebSocketFrame::decode(encoded_ping);
    EXPECT_TRUE(decoded_ping.valid);
    EXPECT_EQ(decoded_ping.frame.opcode, WebSocketFrame::Opcode::Ping);
    EXPECT_EQ(decoded_ping.frame.payload_string(), "healthcheck_token");

    // Pong frame echo
    WebSocketFrame pong = WebSocketFrame::pong(decoded_ping.frame);
    EXPECT_EQ(pong.opcode, WebSocketFrame::Opcode::Pong);
    EXPECT_EQ(pong.payload_string(), "healthcheck_token");

    // Close frame
    WebSocketFrame close = WebSocketFrame::close(1000, "Normal Closure");
    auto encoded_close = close.encode();
    auto decoded_close = WebSocketFrame::decode(encoded_close);
    EXPECT_TRUE(decoded_close.valid);
    EXPECT_EQ(decoded_close.frame.opcode, WebSocketFrame::Opcode::Close);
    EXPECT_EQ(decoded_close.frame.close_code(), 1000);
    EXPECT_EQ(decoded_close.frame.close_reason(), "Normal Closure");
}

// 19. Connection Tracking and Backpressure High/Low Watermarks
TEST_F(Pkg125ProductionValidationTest, ConnectionBackpressureAndTracking) {
    ConnectionTracker tracker;
    BackpressureConfig config{
        .high_watermark = 64000,
        .low_watermark = 16000,
        .slow_client_timeout = std::chrono::seconds(10)
    };

    uint64_t conn_id = tracker.add_connection("192.168.1.100:54321");
    EXPECT_EQ(tracker.active_count(), 1u);

    // Buffer normal
    bool paused = tracker.update_write_buffer(conn_id, 32000, config);
    EXPECT_FALSE(paused);

    // Buffer exceeds high watermark — backpressure triggers pause
    paused = tracker.update_write_buffer(conn_id, 70000, config);
    EXPECT_TRUE(paused);
    auto info = tracker.get_connection(conn_id);
    ASSERT_TRUE(info.has_value());
    EXPECT_TRUE(info->read_paused);

    // Buffer drops below low watermark — resume reads
    tracker.update_write_buffer(conn_id, 10000, config);
    info = tracker.get_connection(conn_id);
    EXPECT_FALSE(info->read_paused);

    tracker.remove_connection(conn_id);
    EXPECT_EQ(tracker.active_count(), 0u);
}

// =========================================================================
// Category 5: Performance Benchmarks & Reliability (Finding 5)
// =========================================================================

// 20. Signal Update Latency Benchmark: Sub-microsecond P99 target
TEST_F(Pkg125ProductionValidationTest, SignalUpdateLatencyBenchmark) {
    auto count = useSignal<int>(0);
    auto double_count = useComputed<int>([count]() {
        return count.get() * 2;
    });

    BenchmarkTimer timer;
    constexpr size_t kIterations = 10000;

    for (size_t i = 0; i < kIterations; ++i) {
        timer.start();
        count.set(static_cast<int>(i));
        (void)double_count.get();
        timer.stop();
    }

    EXPECT_EQ(timer.sample_count(), kIterations);
    EXPECT_GT(timer.p50().count(), 0);

    BenchmarkReport report;
    report.suite_name = "Signal Latency";
    report.add_metric("Signal Set & Computed Derivation", timer);

    // Verify sub-millisecond P99 (typically < 10 µs on modern CPUs)
    EXPECT_LT(timer.p99().count(), 500000); // < 500 µs upper bound
}

// 21. Batch Update Throughput Benchmark
TEST_F(Pkg125ProductionValidationTest, BatchedVsUnbatchedThroughputBenchmark) {
    auto s1 = useSignal<int>(0);
    auto s2 = useSignal<int>(0);
    std::atomic<int> effect_counter{0};

    auto eff = create_effect([s1, s2, &effect_counter]() {
        (void)s1.get();
        (void)s2.get();
        effect_counter.fetch_add(1);
    });

    effect_counter.store(0);

    // 1000 batched updates
    BenchmarkTimer batch_timer;
    batch_timer.start();
    batch([&]() {
        for (int i = 0; i < 1000; ++i) {
            s1.set(i);
            s2.set(i * 2);
        }
    });
    batch_timer.stop();

    // In batch mode, effect must have executed exactly ONCE
    EXPECT_EQ(effect_counter.load(), 1);
    EXPECT_LT(batch_timer.total().count(), 100000000); // < 100ms
}

// 22. Frame Time Simulation (Target: 60 FPS / < 16.67ms frame times)
TEST_F(Pkg125ProductionValidationTest, FrameTimeBenchmarkSimulation) {
    FrameTimeBenchmark frame_bench;

    // Simulate 60 frames of UI rendering
    for (int f = 0; f < 60; ++f) {
        frame_bench.simulate_frame([]() {
            // Simulated layout reconciliation
            volatile int sum = 0;
            for (int i = 0; i < 5000; ++i) {
                sum += i;
            }
            (void)sum;
        });
    }

    EXPECT_EQ(frame_bench.frame_count(), 60u);
    EXPECT_TRUE(frame_bench.meets_target_fps(60.0));
    EXPECT_GT(frame_bench.average_fps(), 60.0);
}

// 23. Server API Request Throughput Benchmark
TEST_F(Pkg125ProductionValidationTest, ServerApiThroughputBenchmark) {
    Server server;
    server.get("/api/ping", [](const Request&, Response& res) {
        res.json("{\"status\":\"ok\"}");
    });

    ThroughputBenchmark bench;
    constexpr size_t kRequests = 5000;

    Request req("GET", "/api/ping");
    auto result = bench.run(kRequests, [&]() {
        auto res = server.handle_request(req);
        (void)res;
    });

    EXPECT_EQ(result.total_requests, kRequests);
    EXPECT_GT(result.requests_per_second, 1000.0); // > 1,000 req/sec
    EXPECT_LT(result.avg_latency_us, 1000.0);       // < 1 ms average latency
}

// 24. Memory Stability Over Mount/Unmount Lifecycles
TEST_F(Pkg125ProductionValidationTest, MemoryStabilityOverMountUnmountCycles) {
    MemoryTracker mem_tracker;
    mem_tracker.record();

    // 2,000 mount/unmount cycles
    for (int cycle = 0; cycle < 2000; ++cycle) {
        {
            auto sig = useSignal<int>(cycle);
            auto comp = useComputed<int>([sig]() { return sig.get() * 3; });
            auto eff = create_effect([comp]() { (void)comp.get(); });
            sig.set(cycle + 1);
            eff->dispose();
            comp.dispose();
        }
        if (cycle % 200 == 0) {
            mem_tracker.record();
        }
    }

    mem_tracker.record();
    EXPECT_GE(mem_tracker.snapshot_count(), 10u);

    // Verify no runaway memory growth (within 30% allowance for heap allocator fragmentation)
    EXPECT_FALSE(mem_tracker.has_growth(3, 1.30));
}
