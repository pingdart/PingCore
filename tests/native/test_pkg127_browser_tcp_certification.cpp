// tests/native/test_pkg127_browser_tcp_certification.cpp
// Comprehensive test suite for PKG-127: Real Browser, TCP/TLS & Native Device Certification
// 1. Real Google Chrome Headless Browser Hydration & Screenshot Certification
// 2. Production TCP/TLS Server (TLS Record Layer, ClientHello, SNI, ALPN, Connection Limits & Drain)
// 3. HTTP/1.1 RFC 9112 Framing & Smuggling Hardening (Duplicate Identical CL, Overflow, Extensions & Trailers, Pipelining)
// 4. Hot Reload Stress Validation & Object-Lifetime Leases (ModuleOwned<T>, Quiescence Barriers)
// 5. Reactive Concurrency Hardening (Dynamic Dependency Switching, Depth Limits, Self-Cancelling Subscribers, Schedulers)
// 6. Reproducible Performance & Latency Benchmarks (p50, p90, p95, p99)

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

class Pkg127BrowserTcpCertificationTest : public ::testing::Test {
protected:
    void SetUp() override {
        QueryCache::instance().clear();
        HotReloadManager::instance().clear_state();
        HotReloadManager::instance().clear_modules();
        HydrationEngine::instance().clear();
        WebSocketBroadcaster::instance().clear();
        ReactiveScheduler::instance().clear();
        ModuleObjectTracker::instance().clear();
    }

    void TearDown() override {
        QueryCache::instance().clear();
        HotReloadManager::instance().clear_state();
        HotReloadManager::instance().clear_modules();
        HydrationEngine::instance().clear();
        WebSocketBroadcaster::instance().clear();
        ReactiveScheduler::instance().clear();
        ModuleObjectTracker::instance().clear();
    }
};

// =========================================================================
// Category 1: HTTP/1.1 RFC 9112 Framing, Smuggling Hardening & Pipelining
// =========================================================================

// 1. RFC 9112 §6.1: Duplicate Identical vs Conflicting Content-Length Handling
TEST_F(Pkg127BrowserTcpCertificationTest, DuplicateContentLengthRfc9112) {
    // Identical duplicate Content-Length headers -> Must collapse into single valid value
    std::string identical_cl_req =
        "POST /api/echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello";

    auto parsed_identical = parse_http_request(identical_cl_req);
    EXPECT_TRUE(parsed_identical.valid);
    EXPECT_EQ(parsed_identical.content_length, 5);
    EXPECT_EQ(parsed_identical.body, "hello");

    // Conflicting duplicate Content-Length headers -> MUST reject per RFC 9112 §6.1
    std::string conflicting_cl_req =
        "POST /api/echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "Content-Length: 10\r\n"
        "\r\n"
        "hello";

    auto parsed_conflicting = parse_http_request(conflicting_cl_req);
    EXPECT_FALSE(parsed_conflicting.valid);
    EXPECT_EQ(parsed_conflicting.status_code, 400);
    EXPECT_NE(parsed_conflicting.error.find("RFC 9112 request smuggling violation"), std::string::npos);
}

// 2. Content-Length Integer Overflow & Decimal Character Validation
TEST_F(Pkg127BrowserTcpCertificationTest, ContentLengthIntegerOverflowAndFormat) {
    // Integer overflow: value exceeds uint64_t / int64_t max
    std::string overflow_req =
        "POST /api/data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 18446744073709551616\r\n"
        "\r\n";

    auto parsed_overflow = parse_http_request(overflow_req);
    EXPECT_FALSE(parsed_overflow.valid);
    EXPECT_EQ(parsed_overflow.status_code, 400);

    // Non-numeric characters in Content-Length
    std::string non_numeric_req =
        "POST /api/data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 1234abc\r\n"
        "\r\n";

    auto parsed_non_numeric = parse_http_request(non_numeric_req);
    EXPECT_FALSE(parsed_non_numeric.valid);
    EXPECT_EQ(parsed_non_numeric.status_code, 400);

    // Exceeds max body size limit (10 MB) -> 413 Payload Too Large
    std::string oversized_req =
        "POST /api/data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 15000000\r\n"
        "\r\n";

    auto parsed_oversized = parse_http_request(oversized_req);
    EXPECT_FALSE(parsed_oversized.valid);
    EXPECT_EQ(parsed_oversized.status_code, 413);
}

// 3. RFC 9112 §7.1: Chunk Extensions & Trailers Validation
TEST_F(Pkg127BrowserTcpCertificationTest, ChunkExtensionsAndTrailersValidation) {
    // Valid chunk extension and trailer
    std::string valid_chunked =
        "POST /api/upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5;name=chunk1\r\nhello\r\n"
        "0\r\n"
        "Expires: Wed, 21 Oct 2026 07:28:00 GMT\r\n\r\n";

    auto parsed_valid = parse_http_request(valid_chunked);
    EXPECT_TRUE(parsed_valid.valid);
    EXPECT_EQ(parsed_valid.body, "hello");

    // Malformed chunk extension containing illegal control character
    std::string invalid_ext =
        "POST /api/upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5;bad=\x01\r\nhello\r\n"
        "0\r\n\r\n";

    auto parsed_invalid_ext = parse_http_request(invalid_ext);
    EXPECT_FALSE(parsed_invalid_ext.valid);
    EXPECT_EQ(parsed_invalid_ext.status_code, 400);

    // Forbidden trailer field (Content-Length inside chunk trailer) -> RFC 9112 violation
    std::string forbidden_trailer =
        "POST /api/upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\nhello\r\n"
        "0\r\n"
        "Content-Length: 5\r\n\r\n";

    auto parsed_forbidden = parse_http_request(forbidden_trailer);
    EXPECT_FALSE(parsed_forbidden.valid);
    EXPECT_EQ(parsed_forbidden.status_code, 400);
}

// 4. Pipelined HTTP Stream Parsing (Multiple Requests in Single Buffer)
TEST_F(Pkg127BrowserTcpCertificationTest, PipelinedHttpRequestStreamParsing) {
    std::string pipelined_stream =
        "GET /api/first HTTP/1.1\r\nHost: localhost\r\n\r\n"
        "POST /api/second HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nhello"
        "GET /api/third HTTP/1.1\r\nHost: localhost\r\n\r\n";

    auto result = parse_http_requests_pipelined(pipelined_stream);
    EXPECT_FALSE(result.incomplete);
    EXPECT_EQ(result.requests.size(), 3);
    EXPECT_EQ(result.requests[0].method, "GET");
    EXPECT_EQ(result.requests[0].path, "/api/first");
    EXPECT_EQ(result.requests[1].method, "POST");
    EXPECT_EQ(result.requests[1].path, "/api/second");
    EXPECT_EQ(result.requests[1].body, "hello");
    EXPECT_EQ(result.requests[2].method, "GET");
    EXPECT_EQ(result.requests[2].path, "/api/third");
    EXPECT_EQ(result.total_bytes_consumed, pipelined_stream.size());
}

// 5. Partial Read Handling in Streaming Sockets
TEST_F(Pkg127BrowserTcpCertificationTest, PartialReadStreamingHandling) {
    // Truncated header (missing \r\n\r\n)
    std::string partial_header = "GET /api/partial HTTP/1.1\r\nHost: localhost\r\n";
    auto res1 = parse_http_requests_pipelined(partial_header);
    EXPECT_TRUE(res1.incomplete);
    EXPECT_EQ(res1.requests.size(), 0);

    // Complete header but incomplete Content-Length body
    std::string partial_body = "POST /api/partial HTTP/1.1\r\nContent-Length: 10\r\n\r\n12345";
    auto res2 = parse_http_requests_pipelined(partial_body);
    EXPECT_TRUE(res2.incomplete);
    EXPECT_EQ(res2.requests.size(), 0);
}

// =========================================================================
// Category 2: Production TCP/TLS Server Protocols & Connection Management
// =========================================================================

// 6. Server Connection Limits & 503 Service Unavailable
TEST_F(Pkg127BrowserTcpCertificationTest, ServerConnectionLimits) {
    Server server;
    Server::ServerConfig cfg;
    cfg.max_connections = 2;
    server.set_server_config(cfg);

    EXPECT_TRUE(server.can_accept_connection());
    auto id1 = server.connections().add_connection("127.0.0.1:50001");
    EXPECT_TRUE(server.can_accept_connection());
    auto id2 = server.connections().add_connection("127.0.0.1:50002");
    EXPECT_FALSE(server.can_accept_connection()); // Max reached!

    server.connections().remove_connection(id1);
    EXPECT_TRUE(server.can_accept_connection());
    server.connections().remove_connection(id2);
}

// 7. Server Graceful Connection Drain
TEST_F(Pkg127BrowserTcpCertificationTest, GracefulServerConnectionDrain) {
    Server server;
    auto id = server.connections().add_connection("127.0.0.1:50003");
    EXPECT_EQ(server.connections().active_count(), 1);

    // In background, remove connection after 50ms
    std::thread worker([&server, id]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        server.connections().remove_connection(id);
    });

    server.drain(std::chrono::milliseconds(300));
    EXPECT_EQ(server.connections().active_count(), 0);

    if (worker.joinable()) worker.join();
}

// 8. TLS Record Header Validation
TEST_F(Pkg127BrowserTcpCertificationTest, TlsRecordHeaderValidation) {
    // Valid Handshake record: Type 22, TLS 1.2 (0x0303), Length 0x0020 (32 bytes)
    uint8_t valid_record[5] = { 0x16, 0x03, 0x03, 0x00, 0x20 };
    auto header = parse_tls_record_header(valid_record, sizeof(valid_record));
    EXPECT_TRUE(header.valid);
    EXPECT_EQ(header.type, TlsRecordHeader::ContentType::Handshake);
    EXPECT_EQ(header.legacy_version, 0x0303);
    EXPECT_EQ(header.length, 32);

    // Invalid ContentType (e.g. 0x01)
    uint8_t invalid_type[5] = { 0x01, 0x03, 0x03, 0x00, 0x20 };
    auto bad_header = parse_tls_record_header(invalid_type, sizeof(invalid_type));
    EXPECT_FALSE(bad_header.valid);
    EXPECT_NE(bad_header.error.find("Invalid TLS ContentType"), std::string::npos);

    // Oversized record length (> 18432)
    uint8_t oversized_record[5] = { 0x16, 0x03, 0x03, 0x50, 0x00 };
    auto oversized_header = parse_tls_record_header(oversized_record, sizeof(oversized_record));
    EXPECT_FALSE(oversized_header.valid);
    EXPECT_NE(oversized_header.error.find("Oversized TLS record"), std::string::npos);
}

// 9. TLS ClientHello SNI and ALPN Protocol Negotiation Extraction
TEST_F(Pkg127BrowserTcpCertificationTest, TlsClientHelloSniAndAlpnExtraction) {
    // Construct a synthetic valid TLS 1.2 ClientHello with SNI "api.pingdart.com" and ALPN "h2", "http/1.1"
    std::vector<uint8_t> packet;

    // SNI extension payload
    std::string sni_host = "api.pingdart.com";
    std::vector<uint8_t> sni_ext;
    uint16_t sni_list_len = static_cast<uint16_t>(sni_host.size() + 3);
    sni_ext.push_back(static_cast<uint8_t>(sni_list_len >> 8));
    sni_ext.push_back(static_cast<uint8_t>(sni_list_len & 0xFF));
    sni_ext.push_back(0x00); // HostName type
    sni_ext.push_back(static_cast<uint8_t>(sni_host.size() >> 8));
    sni_ext.push_back(static_cast<uint8_t>(sni_host.size() & 0xFF));
    sni_ext.insert(sni_ext.end(), sni_host.begin(), sni_host.end());

    // ALPN extension payload: "h2", "http/1.1"
    std::vector<uint8_t> alpn_ext;
    std::vector<std::string> protos = { "h2", "http/1.1" };
    std::vector<uint8_t> alpn_list;
    for (const auto& p : protos) {
        alpn_list.push_back(static_cast<uint8_t>(p.size()));
        alpn_list.insert(alpn_list.end(), p.begin(), p.end());
    }
    uint16_t alpn_list_len = static_cast<uint16_t>(alpn_list.size());
    alpn_ext.push_back(static_cast<uint8_t>(alpn_list_len >> 8));
    alpn_ext.push_back(static_cast<uint8_t>(alpn_list_len & 0xFF));
    alpn_ext.insert(alpn_ext.end(), alpn_list.begin(), alpn_list.end());

    // Combined extensions
    std::vector<uint8_t> extensions;
    // SNI header (type 0x0000)
    extensions.push_back(0x00); extensions.push_back(0x00);
    extensions.push_back(static_cast<uint8_t>(sni_ext.size() >> 8));
    extensions.push_back(static_cast<uint8_t>(sni_ext.size() & 0xFF));
    extensions.insert(extensions.end(), sni_ext.begin(), sni_ext.end());
    // ALPN header (type 0x0010)
    extensions.push_back(0x00); extensions.push_back(0x10);
    extensions.push_back(static_cast<uint8_t>(alpn_ext.size() >> 8));
    extensions.push_back(static_cast<uint8_t>(alpn_ext.size() & 0xFF));
    extensions.insert(extensions.end(), alpn_ext.begin(), alpn_ext.end());

    // ClientHello body
    std::vector<uint8_t> hello_body;
    hello_body.push_back(0x03); hello_body.push_back(0x03); // client_version TLS 1.2
    for (int i = 0; i < 32; ++i) hello_body.push_back(0xAA); // random
    hello_body.push_back(0x00); // session_id len
    // Cipher suites: 2 bytes len, 0x1301 (TLS_AES_128_GCM_SHA256)
    hello_body.push_back(0x00); hello_body.push_back(0x02);
    hello_body.push_back(0x13); hello_body.push_back(0x01);
    // Compression methods: 1 byte len, 0x00 (null)
    hello_body.push_back(0x01); hello_body.push_back(0x00);
    // Extensions length + data
    uint16_t ext_total_len = static_cast<uint16_t>(extensions.size());
    hello_body.push_back(static_cast<uint8_t>(ext_total_len >> 8));
    hello_body.push_back(static_cast<uint8_t>(ext_total_len & 0xFF));
    hello_body.insert(hello_body.end(), extensions.begin(), extensions.end());

    // Handshake header: Type 1 (ClientHello), 3 bytes len
    std::vector<uint8_t> handshake;
    handshake.push_back(0x01);
    uint32_t hs_len = static_cast<uint32_t>(hello_body.size());
    handshake.push_back(static_cast<uint8_t>((hs_len >> 16) & 0xFF));
    handshake.push_back(static_cast<uint8_t>((hs_len >> 8) & 0xFF));
    handshake.push_back(static_cast<uint8_t>(hs_len & 0xFF));
    handshake.insert(handshake.end(), hello_body.begin(), hello_body.end());

    // TLS Record Header: Type 22, TLS 1.2 (0x0303), 2 bytes length
    uint16_t rec_len = static_cast<uint16_t>(handshake.size());
    packet.push_back(0x16);
    packet.push_back(0x03); packet.push_back(0x03);
    packet.push_back(static_cast<uint8_t>(rec_len >> 8));
    packet.push_back(static_cast<uint8_t>(rec_len & 0xFF));
    packet.insert(packet.end(), handshake.begin(), handshake.end());

    auto info = parse_tls_client_hello(packet.data(), packet.size());
    EXPECT_TRUE(info.valid);
    EXPECT_EQ(info.client_version, 0x0303);
    EXPECT_EQ(info.server_name, "api.pingdart.com");
    ASSERT_EQ(info.alpn_protocols.size(), 2);
    EXPECT_EQ(info.alpn_protocols[0], "h2");
    EXPECT_EQ(info.alpn_protocols[1], "http/1.1");
    ASSERT_EQ(info.cipher_suites.size(), 1);
    EXPECT_EQ(info.cipher_suites[0], 0x1301);
}

// 10. Rejection of Truncated or Malformed TLS Handshake Records
TEST_F(Pkg127BrowserTcpCertificationTest, TlsClientHelloTruncationRejection) {
    // Record header claims 100 bytes payload, but only 10 bytes provided
    uint8_t truncated_packet[15] = { 0x16, 0x03, 0x03, 0x00, 0x64, 0x01, 0x00, 0x00, 0x60, 0x03, 0x03 };
    auto info = parse_tls_client_hello(truncated_packet, sizeof(truncated_packet));
    EXPECT_FALSE(info.valid);
    EXPECT_NE(info.error.find("Truncated TLS handshake"), std::string::npos);
}

// =========================================================================
// Category 3: Hot Reload Object-Lifetime Leases & Quiescence Validation
// =========================================================================

struct MockWidget {
    virtual ~MockWidget() = default;
    virtual std::string render() { return "<div>Widget v1</div>"; }
};

// 11. Module Object Lease Lifetime Retention Across Reload
TEST_F(Pkg127BrowserTcpCertificationTest, ModuleObjectLeaseRetentionAcrossUnload) {
    auto handle = std::make_shared<ModuleHandle>();
    handle->loaded = true;

    // While a ModuleOwned object is alive, active_leases > 0
    {
        ModuleLease lease(handle);
        auto widget = ModuleOwned<MockWidget>::create(std::move(lease));
        EXPECT_EQ(handle->active_leases.load(), 1);
        EXPECT_EQ(widget->render(), "<div>Widget v1</div>");

        // Quiescence barrier drain MUST time out while object is alive!
        bool drained = QuiescenceBarrier::drain_leases(handle, std::chrono::milliseconds(50));
        EXPECT_FALSE(drained);

        // Explicit reset frees the object and releases the lease
        widget.reset();
        EXPECT_EQ(handle->active_leases.load(), 0);
    }

    // Now quiescence barrier succeeds immediately
    bool drained = QuiescenceBarrier::drain_leases(handle, std::chrono::milliseconds(50));
    EXPECT_TRUE(drained);
}

// 12. Module Retirement State While Module-Owned Objects Live
TEST_F(Pkg127BrowserTcpCertificationTest, ModuleRetirementWhileObjectsLive) {
    auto handle = std::make_shared<ModuleHandle>();
    handle->loaded = true;

    ModuleLease lease(handle);
    auto widget = ModuleOwned<MockWidget>::create(std::move(lease));

    // Mark module as retiring (a new module is being loaded)
    handle->retiring.store(true);
    EXPECT_TRUE(handle->retiring.load());

    // Object can still safely execute its methods
    EXPECT_EQ(widget->render(), "<div>Widget v1</div>");

    // Track object in ModuleObjectTracker
    ModuleObjectTracker::instance().register_object("libapp_v1.so", widget.get());
    EXPECT_EQ(ModuleObjectTracker::instance().live_object_count("libapp_v1.so"), 1);

    ModuleObjectTracker::instance().unregister_object("libapp_v1.so", widget.get());
    EXPECT_EQ(ModuleObjectTracker::instance().live_object_count("libapp_v1.so"), 0);

    widget.reset();
    EXPECT_EQ(handle->active_leases.load(), 0);
}

// =========================================================================
// Category 4: Reactive Concurrency Hardening
// =========================================================================

// 13. Concurrent Dynamic Dependency Switching Stress Test
TEST_F(Pkg127BrowserTcpCertificationTest, ConcurrentDynamicDependencySwitching) {
    auto cond = useSignal<bool>(true);
    auto sigA = useSignal<int>(10);
    auto sigB = useSignal<int>(20);

    auto derived = useComputed<int>([=]() {
        return cond.get() ? sigA.get() : sigB.get();
    });

    EXPECT_EQ(derived.get(), 10);

    // Switch condition to false
    cond.set(false);
    EXPECT_EQ(derived.get(), 20);

    // Concurrent stress test: multiple threads mutating signals and condition
    std::atomic<bool> stop_flag{false};
    std::vector<std::thread> workers;

    for (int t = 0; t < 8; ++t) {
        workers.emplace_back([&, t]() {
            for (int i = 0; i < 200 && !stop_flag.load(); ++i) {
                if (t % 2 == 0) {
                    sigA.set(i);
                } else {
                    sigB.set(i * 2);
                }
                if (i % 10 == 0) {
                    cond.set(i % 20 == 0);
                }
            }
        });
    }

    for (auto& w : workers) {
        if (w.joinable()) w.join();
    }

    // After all mutations, computed value must match the final active branch
    int expected = cond.get() ? sigA.get() : sigB.get();
    EXPECT_EQ(derived.get(), expected);
}

// 14. Nested Cascaded Effects with Recursion Depth Limit
TEST_F(Pkg127BrowserTcpCertificationTest, NestedCascadedEffectsWithDepthLimit) {
    auto sigA = useSignal<int>(0);
    auto sigB = useSignal<int>(0);
    std::atomic<int> cascade_count{0};

    // Cascaded effect: updates SigB when SigA changes
    auto eff1 = create_effect([&]() {
        int a = sigA.get();
        cascade_count++;
        if (a < 5) {
            sigB.set(a + 1);
        }
    });

    // Cascaded effect: updates SigA when SigB changes
    auto eff2 = create_effect([&]() {
        int b = sigB.get();
        cascade_count++;
        if (b < 5) {
            sigA.set(b + 1);
        }
    });

    EXPECT_GT(cascade_count.load(), 0);
    EXPECT_LE(sigA.get(), 6);
    EXPECT_LE(sigB.get(), 6);
}

// 15. Self-Cancelling Reactive Subscriber During Dispatch
class SelfCancellingObserver : public ReactiveNode {
public:
    SelfCancellingObserver(Signal<int> sig) : sig_(sig) {}

    void set_handle(SubscriptionHandle handle) {
        handle_ = std::move(handle);
    }

    void on_dependency_changed() override {
        call_count++;
        // Self-cancel during notification dispatch
        handle_.unsubscribe();
    }

    void track_dependency(ReactiveNode*) override {}

    std::atomic<int> call_count{0};
private:
    Signal<int> sig_;
    SubscriptionHandle handle_;
};

TEST_F(Pkg127BrowserTcpCertificationTest, SelfCancellingSubscriberDuringDispatch) {
    auto count = useSignal<int>(0);
    auto observer = std::make_shared<SelfCancellingObserver>(count);
    auto handle = count.subscribe(observer);
    observer->set_handle(std::move(handle));

    // First mutation triggers observer, which self-cancels
    count.set(1);
    EXPECT_EQ(observer->call_count.load(), 1);

    // Second mutation MUST NOT trigger observer anymore
    count.set(2);
    EXPECT_EQ(observer->call_count.load(), 1);
}

// 16. ReactiveScheduler UI Thread Task Queue Dispatch
TEST_F(Pkg127BrowserTcpCertificationTest, ReactiveSchedulerDispatchQueue) {
    auto& scheduler = ReactiveScheduler::instance();
    std::vector<int> executed_order;

    scheduler.queue_task([&]() { executed_order.push_back(1); });
    scheduler.queue_task([&]() { executed_order.push_back(2); });
    scheduler.queue_task([&]() { executed_order.push_back(3); });

    EXPECT_EQ(scheduler.pending_task_count(), 3);

    size_t processed = scheduler.run_pending_tasks();
    EXPECT_EQ(processed, 3);
    EXPECT_EQ(scheduler.pending_task_count(), 0);

    ASSERT_EQ(executed_order.size(), 3);
    EXPECT_EQ(executed_order[0], 1);
    EXPECT_EQ(executed_order[1], 2);
    EXPECT_EQ(executed_order[2], 3);
}

// =========================================================================
// Category 5: Real Google Chrome Headless Browser Certification & Screenshot
// =========================================================================

// 17. Real Google Chrome Headless Hydration, Click Events & Form Preservation
TEST_F(Pkg127BrowserTcpCertificationTest, RealGoogleChromeHeadlessHydration) {
    RealBrowserReport report = run_headless_chrome_certification();

    EXPECT_TRUE(report.success);
    EXPECT_EQ(report.hydrated_count, 2);
    EXPECT_TRUE(report.form_input_preserved);
    EXPECT_TRUE(report.spa_navigation_success);
    EXPECT_EQ(report.current_route, "/dashboard");
    EXPECT_FALSE(report.screenshot_path.empty());
    EXPECT_TRUE(std::filesystem::exists(report.screenshot_path));
    EXPECT_GT(std::filesystem::file_size(report.screenshot_path), 1000);
}

// =========================================================================
// Category 6: Native Device Validation & Performance Benchmarks
// =========================================================================

// 18. Native Platform Scaffolding & Manifest Validation
TEST_F(Pkg127BrowserTcpCertificationTest, NativePlatformPackagingValidation) {
    // Verify Android native scaffold exists and is well-formed
    std::string android_manifest = "/Volumes/Development/PingDart/PingDartCore/platforms/android/app/src/main/AndroidManifest.xml";
    EXPECT_TRUE(std::filesystem::exists(android_manifest));
    std::ifstream am_file(android_manifest);
    std::string am_content((std::istreambuf_iterator<char>(am_file)), std::istreambuf_iterator<char>());
    EXPECT_NE(am_content.find("android.intent.action.MAIN"), std::string::npos);

    // Verify iOS Xcode project exists and is well-formed
    std::string ios_pbxproj = "/Volumes/Development/PingDart/PingDartCore/platforms/ios/PingCoreApp.xcodeproj/project.pbxproj";
    EXPECT_TRUE(std::filesystem::exists(ios_pbxproj));
    std::ifstream ios_file(ios_pbxproj);
    std::string ios_content((std::istreambuf_iterator<char>(ios_file)), std::istreambuf_iterator<char>());
    EXPECT_NE(ios_content.find("PBXProject"), std::string::npos);
}

// 19. Reproducible Performance & Latency Benchmarks (p50, p90, p95, p99)
TEST_F(Pkg127BrowserTcpCertificationTest, ReproduciblePerformanceLatencyBenchmarks) {
    Server server;
    server.get("/api/ping", [](const Request&, Response& res) {
        res.status(200).json("{\"status\":\"ok\"}");
    });

    constexpr size_t kIterations = 2000;
    std::vector<double> latencies_us;
    latencies_us.reserve(kIterations);

    std::string raw_req = "GET /api/ping HTTP/1.1\r\nHost: localhost\r\n\r\n";

    for (size_t i = 0; i < kIterations; ++i) {
        auto start = std::chrono::steady_clock::now();
        Response res = server.handle_raw_request(raw_req);
        auto end = std::chrono::steady_clock::now();
        double us = std::chrono::duration<double, std::micro>(end - start).count();
        latencies_us.push_back(us);
        EXPECT_EQ(res.status_code(), 200);
    }

    std::sort(latencies_us.begin(), latencies_us.end());

    double p50 = latencies_us[kIterations * 50 / 100];
    double p90 = latencies_us[kIterations * 90 / 100];
    double p95 = latencies_us[kIterations * 95 / 100];
    double p99 = latencies_us[kIterations * 99 / 100];

    EXPECT_LT(p50, 100.0);  // P50 < 100 microseconds
    EXPECT_LT(p90, 200.0);  // P90 < 200 microseconds
    EXPECT_LT(p95, 250.0);  // P95 < 250 microseconds
    EXPECT_LT(p99, 1000.0); // P99 < 1 millisecond
}
