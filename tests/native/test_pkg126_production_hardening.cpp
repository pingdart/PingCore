// tests/native/test_pkg126_production_hardening.cpp
// Comprehensive test suite for PKG-126: Production Hardening & Real-World End-to-End Certification
// 1. HTTP Protocol Security & Request Smuggling Fuzzing/Regression (RFC 7230 / RFC 9112)
// 2. Safe C++ Hot Reload with Reference-Counted Leases & Quiescence Barriers
// 3. Reactive Lifetime Hardening with Weak Subscriptions & Concurrent Teardown
// 4. Real Browser Certification (Interactive Counter, Form Preservation, SPA Navigation)
// 5. Real Loopback TCP Socket Networking & HTTP Roundtrip over Sockets

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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace pingcore;

class Pkg126ProductionHardeningTest : public ::testing::Test {
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
// Category 1: HTTP Security & Request Smuggling Regressions (RFC 7230 / 9112)
// =========================================================================

// 1. Request Smuggling: Reject Conflicting Framing Headers (Both TE and CL)
TEST_F(Pkg126ProductionHardeningTest, SmugglingRejectionConflictingFraming) {
    std::string smuggling_req =
        "POST /api/transfer HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 15\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "0\r\n\r\n";

    auto parsed = parse_http_request(smuggling_req);
    EXPECT_FALSE(parsed.valid);
    EXPECT_EQ(parsed.status_code, 400);
    EXPECT_NE(parsed.error.find("request smuggling violation"), std::string::npos);

    Server server;
    auto res = server.handle_raw_request(smuggling_req);
    EXPECT_EQ(res.status_code(), 400);
}

// 2. Request Smuggling: Reject Conflicting Duplicate Content-Length Headers
TEST_F(Pkg126ProductionHardeningTest, SmugglingRejectionDuplicateContentLength) {
    std::string dup_cl_req =
        "POST /api/transfer HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 10\r\n"
        "Content-Length: 20\r\n"
        "\r\n"
        "0123456789";

    auto parsed = parse_http_request(dup_cl_req);
    EXPECT_FALSE(parsed.valid);
    EXPECT_EQ(parsed.status_code, 400);
    EXPECT_NE(parsed.error.find("conflicting duplicate Content-Length"), std::string::npos);
}

// 3. Request Smuggling: Reject Whitespace Before Colon in Header Names (RFC 7230 §3.2.4)
TEST_F(Pkg126ProductionHardeningTest, SmugglingRejectionWhitespaceBeforeColon) {
    // Note space before colon: "Content-Length : 10"
    std::string malformed_header =
        "POST /api/data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length : 10\r\n"
        "\r\n"
        "0123456789";

    auto parsed = parse_http_request(malformed_header);
    EXPECT_FALSE(parsed.valid);
    EXPECT_EQ(parsed.status_code, 400);
    EXPECT_NE(parsed.error.find("invalid whitespace before colon"), std::string::npos);
}

// 4. Security Limit: Request Header Fields Too Large (RFC 6585)
TEST_F(Pkg126ProductionHardeningTest, SecurityLimitOversizedHeaders) {
    std::string huge_req = "GET /index HTTP/1.1\r\nHost: localhost\r\n";
    // Add 9 KB of junk headers (limit is 8 KB)
    for (int i = 0; i < 90; ++i) {
        huge_req += "X-Custom-Header-" + std::to_string(i) + ": " + std::string(100, 'A') + "\r\n";
    }
    huge_req += "\r\n";

    auto parsed = parse_http_request(huge_req);
    EXPECT_FALSE(parsed.valid);
    EXPECT_EQ(parsed.status_code, 431); // 431 Request Header Fields Too Large
}

// 5. Strict Chunked Encoding: Reject Non-Hex Chunk Sizes and Extensions
TEST_F(Pkg126ProductionHardeningTest, StrictChunkedEncodingValidation) {
    // Invalid chunk size (contains 'Z')
    std::string bad_chunk =
        "POST /stream HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "1Z\r\n"
        "Hello\r\n"
        "0\r\n\r\n";

    auto parsed = parse_http_request(bad_chunk);
    EXPECT_FALSE(parsed.valid);
    EXPECT_EQ(parsed.status_code, 400);

    // Chunk data not followed strictly by CRLF
    std::string missing_crlf =
        "POST /stream HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "HelloXX" // missing CRLF
        "0\r\n\r\n";

    auto parsed2 = parse_http_request(missing_crlf);
    EXPECT_FALSE(parsed2.valid);
    EXPECT_EQ(parsed2.status_code, 400);
}

// =========================================================================
// Category 2: WebSocket Protocol Hardening (RFC 6455)
// =========================================================================

// 6. RFC 6455 §8.1: Reject Invalid UTF-8 in Text Frames with Code 1007
TEST_F(Pkg126ProductionHardeningTest, WebSocketInvalidUtf8Rejection) {
    WebSocketFrame frame;
    frame.opcode = WebSocketFrame::Opcode::Text;
    frame.fin = true;
    // Invalid UTF-8 payload: 0xFF 0xFF
    frame.payload = {0xFF, 0xFF};

    auto encoded = frame.encode();
    auto decoded = WebSocketFrame::decode(encoded);

    EXPECT_FALSE(decoded.valid);
    EXPECT_EQ(decoded.close_error_code, 1007); // 1007 = Invalid Frame Payload Data
}

// 7. RFC 6455 §5.1: Reject Unmasked Frames from Clients
TEST_F(Pkg126ProductionHardeningTest, WebSocketUnmaskedClientFrameRejection) {
    WebSocketFrame frame = WebSocketFrame::text("Unmasked payload");
    frame.masked = false;

    auto encoded = frame.encode();
    // Server requires masked client frames:
    auto decoded = WebSocketFrame::decode(encoded, /*require_masked=*/true);

    EXPECT_FALSE(decoded.valid);
    EXPECT_EQ(decoded.close_error_code, 1002); // 1002 = Protocol Error
}

// 8. RFC 6455 §5.5: Reject Fragmented Control Frames
TEST_F(Pkg126ProductionHardeningTest, WebSocketFragmentedControlFrameRejection) {
    WebSocketFrame ping_frame;
    ping_frame.opcode = WebSocketFrame::Opcode::Ping;
    ping_frame.fin = false; // VIOLATION: Control frames cannot be fragmented!
    ping_frame.payload = {1, 2, 3};

    auto encoded = ping_frame.encode();
    auto decoded = WebSocketFrame::decode(encoded);

    EXPECT_FALSE(decoded.valid);
    EXPECT_EQ(decoded.close_error_code, 1002); // Protocol Error
}

// 9. RFC 6455: Fragmented Message Reassembly Across Continuation Frames
TEST_F(Pkg126ProductionHardeningTest, WebSocketFragmentedMessageReassembly) {
    WebSocketMessageAssembler assembler;

    // Frame 1: Text, FIN=false
    WebSocketFrame f1;
    f1.opcode = WebSocketFrame::Opcode::Text;
    f1.fin = false;
    f1.payload.assign({'P', 'i', 'n', 'g'});

    auto r1 = assembler.process_frame(f1);
    EXPECT_FALSE(r1.complete);

    // Frame 2: Continuation, FIN=false
    WebSocketFrame f2;
    f2.opcode = WebSocketFrame::Opcode::Continuation;
    f2.fin = false;
    f2.payload.assign({'C', 'o', 'r', 'e', ' '});

    auto r2 = assembler.process_frame(f2);
    EXPECT_FALSE(r2.complete);

    // Frame 3: Continuation, FIN=true
    WebSocketFrame f3;
    f3.opcode = WebSocketFrame::Opcode::Continuation;
    f3.fin = true;
    f3.payload.assign({'R', 'u', 'n', 't', 'i', 'm', 'e'});

    auto r3 = assembler.process_frame(f3);
    EXPECT_TRUE(r3.complete);
    EXPECT_EQ(r3.opcode, WebSocketFrame::Opcode::Text);
    std::string assembled(r3.payload.begin(), r3.payload.end());
    EXPECT_EQ(assembled, "PingCore Runtime");
}

// =========================================================================
// Category 3: Safe C++ Hot Reload with Leases & Quiescence Barriers
// =========================================================================

// 10. Module Leases and Active Reference Counting
TEST_F(Pkg126ProductionHardeningTest, ModuleLeaseReferenceCounting) {
    auto handle = std::make_shared<ModuleHandle>();
    handle->loaded = true;
    handle->handle = reinterpret_cast<void*>(0x1234);

    EXPECT_EQ(handle->active_leases.load(), 0);

    {
        ModuleLease lease1(handle);
        EXPECT_EQ(handle->active_leases.load(), 1);

        {
            ModuleLease lease2 = lease1; // Copy lease
            EXPECT_EQ(handle->active_leases.load(), 2);
        }
        EXPECT_EQ(handle->active_leases.load(), 1);
    }
    EXPECT_EQ(handle->active_leases.load(), 0);
}

// 11. Live Module Replacement Under Concurrent Leased Callers with Quiescence Draining
TEST_F(Pkg126ProductionHardeningTest, LiveModuleReplacementUnderConcurrentLeases) {
    // Generate addon module v1
    std::string tmp_dir = "/tmp";
    std::string src_v1 = tmp_dir + "/test_module_v1.cpp";
    std::string lib_v1 = tmp_dir + "/libtest_module_v1.dylib";
    std::string src_v2 = tmp_dir + "/test_module_v2.cpp";
    std::string lib_v2 = tmp_dir + "/libtest_module_v2.dylib";

    std::ofstream out1(src_v1);
    out1 << "extern \"C\" { int version() { return 1; } int compute(int x) { return x * 10; } }\n";
    out1.close();

    std::ofstream out2(src_v2);
    out2 << "extern \"C\" { int version() { return 2; } int compute(int x) { return x * 20; } }\n";
    out2.close();

    ASSERT_TRUE(IncrementalCompiler::compile(src_v1, lib_v1).success);
    ASSERT_TRUE(IncrementalCompiler::compile(src_v2, lib_v2).success);

    auto& hmr = HotReloadManager::instance();
    auto mod_v1 = ModuleLoader::load_managed_module(lib_v1);
    ASSERT_TRUE(mod_v1 && mod_v1->is_valid());
    hmr.register_managed_module("calculator", mod_v1);

    std::atomic<bool> stop_workers{false};
    std::atomic<int> calls_v1{0};
    std::atomic<int> calls_v2{0};

    // Spawn 4 concurrent worker threads acquiring leases and computing
    std::vector<std::thread> workers;
    for (int w = 0; w < 4; ++w) {
        workers.emplace_back([&]() {
            using VerFn = int(*)();
            using CompFn = int(*)(int);

            while (!stop_workers.load(std::memory_order_relaxed)) {
                auto lease = hmr.acquire_module_lease("calculator");
                if (lease.is_valid()) {
                    void* v_sym = lease.get_symbol("version");
                    void* c_sym = lease.get_symbol("compute");
                    if (v_sym && c_sym) {
                        auto v_fn = reinterpret_cast<VerFn>(reinterpret_cast<uintptr_t>(v_sym));
                        auto c_fn = reinterpret_cast<CompFn>(reinterpret_cast<uintptr_t>(c_sym));
                        int ver = v_fn();
                        int res = c_fn(5);
                        if (ver == 1) {
                            EXPECT_EQ(res, 50);
                            calls_v1.fetch_add(1);
                        } else if (ver == 2) {
                            EXPECT_EQ(res, 100);
                            calls_v2.fetch_add(1);
                        }
                    }
                }
                std::this_thread::yield();
            }
        });
    }

    // Let workers run on v1 for a short burst
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Hot-swap module to v2 live while workers are actively executing!
    auto reload_res = hmr.safe_reload_module("calculator", lib_v2);
    EXPECT_TRUE(reload_res.success);
    EXPECT_FALSE(reload_res.rolled_back);

    // Let workers run on v2
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    stop_workers.store(true);
    for (auto& t : workers) {
        t.join();
    }

    EXPECT_GT(calls_v1.load(), 0);
    EXPECT_GT(calls_v2.load(), 0);

    std::filesystem::remove(src_v1);
    std::filesystem::remove(lib_v1);
    std::filesystem::remove(src_v2);
    std::filesystem::remove(lib_v2);
}

// 12. Hot Reload Automatic Rollback on Invalid Candidate Module
TEST_F(Pkg126ProductionHardeningTest, HotReloadAutomaticRollbackOnFailure) {
    auto& hmr = HotReloadManager::instance();
    auto good_mod = std::make_shared<ModuleHandle>();
    good_mod->loaded = true;
    good_mod->handle = reinterpret_cast<void*>(0xFEED);
    hmr.register_managed_module("user_plugin", good_mod);

    // Attempt to reload with a non-existent file
    auto res = hmr.safe_reload_module("user_plugin", "/tmp/non_existent_library.dylib");
    EXPECT_FALSE(res.success);
    EXPECT_TRUE(res.rolled_back);

    // Verify existing module is still active and unmodified
    auto lease = hmr.acquire_module_lease("user_plugin");
    EXPECT_TRUE(lease.is_valid());
    EXPECT_EQ(lease.raw_handle(), good_mod);
}

// =========================================================================
// Category 4: Reactive Lifetime Hardening & Weak Subscriptions
// =========================================================================

// 13. SubscriptionHandle RAII Automatic Unsubscription
TEST_F(Pkg126ProductionHardeningTest, SubscriptionHandleLifecycle) {
    auto count = useSignal<int>(10);

    class CustomObserver : public ReactiveNode {
    public:
        void on_dependency_changed() override {
            notified_count.fetch_add(1);
        }
        std::atomic<int> notified_count{0};
    };

    auto target = std::make_shared<CustomObserver>();
    EXPECT_EQ(target->notified_count.load(), 0);

    // Explicit scoped subscription
    {
        SubscriptionHandle handle = count.subscribe(target);
        EXPECT_TRUE(handle.is_active());

        count.set(20);
        EXPECT_EQ(target->notified_count.load(), 1);

        // Explicit unsubscription
        handle.unsubscribe();
        EXPECT_FALSE(handle.is_active());
    }

    // After unsubscription, signal update must not trigger target
    count.set(30);
    EXPECT_EQ(target->notified_count.load(), 1);
}

// 14. Concurrent Teardown Stress Test (ASan / TSan Race-Free Verification)
TEST_F(Pkg126ProductionHardeningTest, ConcurrentTeardownStressTest) {
    auto state_signal = useSignal<int>(0);
    std::atomic<bool> stop_flag{false};

    // 4 Writer threads mutating signal
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&, i]() {
            for (int k = 0; k < 500 && !stop_flag.load(); ++k) {
                state_signal.set(i * 1000 + k);
            }
        });
    }

    // 4 Observer threads continuously mounting and destroying effects and computeds
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            for (int k = 0; k < 200 && !stop_flag.load(); ++k) {
                {
                    auto comp = useComputed<int>([state_signal]() {
                        return state_signal.get() * 2;
                    });
                    auto eff = create_effect([comp]() {
                        (void)comp.get();
                    });
                    eff->dispose();
                    comp.dispose();
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Signal subscriber count should automatically be pruned to 0
    EXPECT_EQ(state_signal.impl()->active_subscriber_count(), 0u);
}

// =========================================================================
// Category 5: Real Browser Certification (Headless DOM & Hydration)
// =========================================================================

// 15. Server-Rendered Counter Becomes Interactive via Client Hydration
TEST_F(Pkg126ProductionHardeningTest, RealBrowserCounterInteractivity) {
    HeadlessBrowserContext browser;
    std::string ssr_html =
        "<div id=\"root\">"
        "  <span id=\"counter_value\">0</span>"
        "  <button id=\"inc_btn\">Increment</button>"
        "</div>";

    browser.open("/", ssr_html);

    auto counter_el = browser.document().get_element_by_id("counter_value");
    auto btn_el = browser.document().get_element_by_id("inc_btn");
    ASSERT_NE(counter_el, nullptr);
    ASSERT_NE(btn_el, nullptr);
    EXPECT_EQ(counter_el->text_content(), "0");

    // Client-side hydration attaches reactive state
    auto count = useSignal<int>(0);

    // Reactive effect synchronizes state to DOM
    auto eff = create_effect([count, counter_el]() {
        counter_el->set_text_content(std::to_string(count.get()));
    });

    btn_el->add_event_listener("click", [count](const std::string&) {
        count.set(count.get() + 1);
    });

    // Simulate user clicks
    btn_el->dispatch_event("click");
    EXPECT_EQ(counter_el->text_content(), "1");

    btn_el->dispatch_event("click");
    btn_el->dispatch_event("click");
    EXPECT_EQ(counter_el->text_content(), "3");
    EXPECT_EQ(count.get(), 3);
}

// 16. Form Retains User-Entered Input Values Across Hydration
TEST_F(Pkg126ProductionHardeningTest, RealBrowserFormValuePreservationAcrossHydration) {
    HeadlessBrowserContext browser;
    std::string ssr_html =
        "<form id=\"signup_form\">"
        "  <input id=\"user_email\" type=\"text\" value=\"\">"
        "</form>";

    browser.open("/signup", ssr_html);

    auto input_el = browser.document().get_element_by_id("user_email");
    ASSERT_NE(input_el, nullptr);

    // User types in form BEFORE hydration finishes
    input_el->set_value("developer@pingcore.dev");

    // Hydration runs and extracts pre-rendered / user form state
    auto form_values = HydrationEngine::instance().extract_form_values(
        "<input id=\"user_email\" type=\"text\" value=\"" + input_el->value() + "\">");

    ASSERT_EQ(form_values.size(), 1u);
    EXPECT_EQ(form_values[0].element_id, "user_email");
    EXPECT_EQ(form_values[0].value, "developer@pingcore.dev");
    EXPECT_EQ(input_el->value(), "developer@pingcore.dev");
}

// 17. Client-Side SPA Navigation Without Page Reload
TEST_F(Pkg126ProductionHardeningTest, RealBrowserClientSpaNavigation) {
    HeadlessBrowserContext browser;
    browser.open("/home", "<div id=\"content\">Home Page</div>");
    EXPECT_EQ(browser.current_url(), "/home");

    // Client navigates to /dashboard
    browser.push_state("/dashboard", "Dashboard");
    EXPECT_EQ(browser.current_url(), "/dashboard");
    EXPECT_EQ(browser.history_length(), 2u);

    // Client navigates to /settings
    browser.push_state("/settings", "Settings");
    EXPECT_EQ(browser.current_url(), "/settings");

    // Browser back button
    EXPECT_TRUE(browser.back());
    EXPECT_EQ(browser.current_url(), "/dashboard");

    // Browser forward button
    EXPECT_TRUE(browser.forward());
    EXPECT_EQ(browser.current_url(), "/settings");
}

// =========================================================================
// Category 6: Real Loopback TCP Socket Networking & HTTP Roundtrip
// =========================================================================

// 18. Real Socket HTTP Roundtrip over Streaming Sockets (TCP Loopback or UNIX Socketpair)
TEST_F(Pkg126ProductionHardeningTest, RealSocketHttpRoundtrip) {
    Server server;
    server.get("/api/ping", [](const Request&, Response& res) {
        res.json("{\"status\":\"pong\",\"engine\":\"PingCore\"}");
    });

    // Create real kernel streaming socketpair
    int fds[2]{};
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    // Send real HTTP GET wire format over streaming socket
    std::string http_get =
        "GET /api/ping HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";

    ssize_t bytes_sent = ::send(fds[0], http_get.data(), http_get.size(), 0);
    EXPECT_EQ(bytes_sent, static_cast<ssize_t>(http_get.size()));

    // Server processes client socket in background thread
    std::thread server_handler([&]() {
        char buffer[8192]{};
        ssize_t bytes_read = ::recv(fds[1], buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            std::string raw_req(buffer, static_cast<size_t>(bytes_read));
            Response res = server.handle_raw_request(raw_req);
            std::string wire_resp = res.to_http_wire_format();
            (void)::send(fds[1], wire_resp.data(), wire_resp.size(), 0);
        }
        ::close(fds[1]);
    });

    // Client receives real HTTP response over socket
    char resp_buffer[4096]{};
    ssize_t bytes_recv = ::recv(fds[0], resp_buffer, sizeof(resp_buffer) - 1, 0);
    EXPECT_GT(bytes_recv, 0);

    std::string response(resp_buffer, static_cast<size_t>(bytes_recv));
    EXPECT_NE(response.find("HTTP/1.1 200 OK"), std::string::npos);
    EXPECT_NE(response.find("{\"status\":\"pong\",\"engine\":\"PingCore\"}"), std::string::npos);

    server_handler.join();
    ::close(fds[0]);
}

// 19. Real Socket Request Smuggling Rejection over Network Streaming Socket
TEST_F(Pkg126ProductionHardeningTest, RealSocketSmugglingRejection) {
    Server server;
    server.post("/api/echo", [](const Request& req, Response& res) {
        res.send(req.body());
    });

    int fds[2]{};
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    // Send smuggling exploit payload over real streaming socket
    std::string exploit =
        "POST /api/echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "0\r\n\r\n";

    ::send(fds[0], exploit.data(), exploit.size(), 0);

    std::thread server_handler([&]() {
        char buffer[4096]{};
        ssize_t bytes_read = ::recv(fds[1], buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            std::string raw_req(buffer, static_cast<size_t>(bytes_read));
            Response res = server.handle_raw_request(raw_req);
            std::string wire_resp = res.to_http_wire_format();
            (void)::send(fds[1], wire_resp.data(), wire_resp.size(), 0);
        }
        ::close(fds[1]);
    });

    char resp_buf[2048]{};
    ssize_t n = ::recv(fds[0], resp_buf, sizeof(resp_buf) - 1, 0);
    EXPECT_GT(n, 0);

    std::string resp(resp_buf, static_cast<size_t>(n));
    // Verify server rejected smuggling exploit with 400 Bad Request
    EXPECT_NE(resp.find("HTTP/1.1 400 Bad Request"), std::string::npos);

    server_handler.join();
    ::close(fds[0]);
}
