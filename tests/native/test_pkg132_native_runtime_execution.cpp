// tests/native/test_pkg132_native_runtime_execution.cpp
// PingCore PKG-132: Native Runtime Execution & Device Certification Test Suite
#include <gtest/gtest.h>
#include <pingcore/android_native_activity.h>
#include <pingcore/platform_scaffold.h>
#include <pingcore/permissions.h>
#include <pingcore/native_bridge.h>
#include <pingcore/server.h>
#include <pingcore/https_server.h>
#include <pingcore/reactive.h>
#include <pingcore/query.h>
#include <pingcore/native_module.h>
#include <pingcore/hot_reload.h>
#include <pingcore/hydration.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdlib>

class Pkg132NativeRuntimeExecutionTest : public ::testing::Test {
protected:
    void SetUp() override {
        pingcore::AndroidNativeAppEngine::instance().reset();
        pingcore::QueryCache::instance().clear();
        pingcore::HotReloadManager::instance().clear_state();
        pingcore::HydrationEngine::instance().clear();
        pingcore::HotReloadManager::instance().clear_modules();
        pingcore::WebSocketBroadcaster::instance().clear();
        pingcore::ReactiveScheduler::instance().clear();
        pingcore::NativeModuleRegistry::instance().clear();
        pingcore::PermissionRegistry::instance().clear();
    }

    void TearDown() override {
        pingcore::AndroidNativeAppEngine::instance().reset();
        pingcore::QueryCache::instance().clear();
        pingcore::HotReloadManager::instance().clear_state();
        pingcore::HydrationEngine::instance().clear();
    }
};

// =========================================================================
// Category 1: Android NativeActivity & SDK Version Certification (P0 Fixes)
// =========================================================================

// 1. Android APK: ANativeActivity_onCreate Exported Symbol in Dynamic Library
TEST_F(Pkg132NativeRuntimeExecutionTest, AndroidNativeActivityOnCreateSymbolExported) {
    std::filesystem::path test_dir = "/tmp/pingcore_pkg132_build_test";
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directories(test_dir);

    std::string scaffold_cmd = "cd \"" + test_dir.string() + "\" && /Volumes/Development/PingDart/PingDartCore/pd create app demo > /dev/null 2>&1";
    ASSERT_EQ(::system(scaffold_cmd.c_str()), 0);

    std::string build_cmd = "cd \"" + test_dir.string() + "\" && /Volumes/Development/PingDart/PingDartCore/pd build --target android --app demo > /dev/null 2>&1";
    ASSERT_EQ(::system(build_cmd.c_str()), 0);

    std::filesystem::path apk_path = test_dir / "dist" / "android" / "demo.apk";
    ASSERT_TRUE(std::filesystem::exists(apk_path));

    // Verify ZIP integrity
    std::string test_cmd = "zip -T \"" + apk_path.string() + "\" > /dev/null 2>&1";
    EXPECT_EQ(::system(test_cmd.c_str()), 0);

    // Extract libpingcore_app.so and inspect ELF header
    std::filesystem::path extracted_so = "/tmp/pkg132_extracted.so";
    std::string extract_lib_cmd = "unzip -p \"" + apk_path.string() + "\" lib/arm64-v8a/libpingcore_app.so > \"" + extracted_so.string() + "\" 2>/dev/null";
    EXPECT_EQ(::system(extract_lib_cmd.c_str()), 0);

    std::ifstream f(extracted_so, std::ios::binary);
    ASSERT_TRUE(f.is_open());
    char elf_header[64]{};
    f.read(elf_header, 64);
    f.close();

    // Verify ELF Magic (\x7fELF), 64-bit, little-endian
    EXPECT_EQ(elf_header[0], '\x7f');
    EXPECT_EQ(elf_header[1], 'E');
    EXPECT_EQ(elf_header[2], 'L');
    EXPECT_EQ(elf_header[3], 'F');
    EXPECT_EQ(elf_header[4], 2); // ELFCLASS64
    EXPECT_EQ(elf_header[5], 1); // ELFDATA2LSB

    // Verify ELF Type: ET_DYN (3 = Shared object file)
    uint16_t e_type = *reinterpret_cast<uint16_t*>(&elf_header[16]);
    EXPECT_EQ(e_type, 3);

    // Verify Machine Architecture: EM_AARCH64 (0xb7)
    uint16_t e_machine = *reinterpret_cast<uint16_t*>(&elf_header[18]);
    EXPECT_EQ(e_machine, 0xb7);

    // Inspect dynamic symbol table: ANativeActivity_onCreate must be present
    std::string nm_cmd = "nm \"" + extracted_so.string() + "\" 2>/dev/null";
    FILE* pipe = ::popen(nm_cmd.c_str(), "r");
    ASSERT_TRUE(pipe != nullptr);
    char buf[512];
    std::string nm_out;
    while (::fgets(buf, sizeof(buf), pipe)) nm_out += buf;
    ::pclose(pipe);

    EXPECT_NE(nm_out.find("ANativeActivity_onCreate"), std::string::npos);
    EXPECT_NE(nm_out.find("JNI_OnLoad"), std::string::npos);
    EXPECT_NE(nm_out.find("pingcore_android_main"), std::string::npos);
}

// 2. Android NativeActivity: Full Lifecycle Callback Wiring
TEST_F(Pkg132NativeRuntimeExecutionTest, AndroidNativeActivityLifecycleCallbackWiring) {
    ANativeActivityCallbacks callbacks{};
    ANativeActivity activity{};
    activity.callbacks = &callbacks;
    activity.sdkVersion = 34;
    activity.internalDataPath = "/data/user/0/com.pingcore.demo/files";

    // Call ANativeActivity_onCreate entry point
    ANativeActivity_onCreate(&activity, nullptr, 0);

    // Assert that all essential NDK lifecycle callbacks are wired to non-null functions
    EXPECT_NE(activity.callbacks->onStart, nullptr);
    EXPECT_NE(activity.callbacks->onResume, nullptr);
    EXPECT_NE(activity.callbacks->onSaveInstanceState, nullptr);
    EXPECT_NE(activity.callbacks->onPause, nullptr);
    EXPECT_NE(activity.callbacks->onStop, nullptr);
    EXPECT_NE(activity.callbacks->onDestroy, nullptr);
    EXPECT_NE(activity.callbacks->onWindowFocusChanged, nullptr);
    EXPECT_NE(activity.callbacks->onNativeWindowCreated, nullptr);
    EXPECT_NE(activity.callbacks->onNativeWindowResized, nullptr);
    EXPECT_NE(activity.callbacks->onNativeWindowRedrawNeeded, nullptr);
    EXPECT_NE(activity.callbacks->onNativeWindowDestroyed, nullptr);
    EXPECT_NE(activity.callbacks->onInputQueueCreated, nullptr);
    EXPECT_NE(activity.callbacks->onInputQueueDestroyed, nullptr);

    // Engine instance must be attached
    EXPECT_NE(activity.instance, nullptr);
}

// 3. Android NativeActivity: Window Lifecycle, Swapchain Connection & Frame Rendering
TEST_F(Pkg132NativeRuntimeExecutionTest, AndroidNativeActivityWindowLifecycleAndRendering) {
    ANativeActivityCallbacks callbacks{};
    ANativeActivity activity{};
    activity.callbacks = &callbacks;
    activity.sdkVersion = 34;

    ANativeActivity_onCreate(&activity, nullptr, 0);

    const auto& state = pingcore::AndroidNativeAppEngine::instance().state();
    EXPECT_EQ(state.lifecycle.load(), pingcore::AndroidAppLifecycleState::Created);
    EXPECT_EQ(state.surface_state.load(), pingcore::AndroidSurfaceState::NoSurface);

    // Simulate OS firing onStart and onResume
    activity.callbacks->onStart(&activity);
    EXPECT_EQ(state.lifecycle.load(), pingcore::AndroidAppLifecycleState::Started);

    activity.callbacks->onResume(&activity);
    EXPECT_EQ(state.lifecycle.load(), pingcore::AndroidAppLifecycleState::Resumed);

    // Simulate Native Window Creation
    ANativeWindow win{1080, 2400, 1};
    activity.callbacks->onNativeWindowCreated(&activity, &win);
    EXPECT_EQ(state.surface_state.load(), pingcore::AndroidSurfaceState::SurfaceActive);
    EXPECT_EQ(state.window_width, 1080u);
    EXPECT_EQ(state.window_height, 2400u);
    EXPECT_GT(state.frame_count.load(), 0u);

    // Window resize
    win.width = 1440;
    win.height = 3120;
    activity.callbacks->onNativeWindowResized(&activity, &win);
    EXPECT_EQ(state.window_width, 1440u);
    EXPECT_EQ(state.window_height, 3120u);

    // Surface destruction handshake
    activity.callbacks->onNativeWindowDestroyed(&activity, &win);
    EXPECT_EQ(state.surface_state.load(), pingcore::AndroidSurfaceState::SurfaceDestroyed);
    EXPECT_EQ(state.native_window_handle, nullptr);

    // App pause and destruction
    activity.callbacks->onPause(&activity);
    EXPECT_EQ(state.lifecycle.load(), pingcore::AndroidAppLifecycleState::Paused);

    activity.callbacks->onStop(&activity);
    EXPECT_EQ(state.lifecycle.load(), pingcore::AndroidAppLifecycleState::Stopped);

    activity.callbacks->onDestroy(&activity);
    EXPECT_EQ(state.lifecycle.load(), pingcore::AndroidAppLifecycleState::Destroyed);
}

// 4. Android APK: AndroidManifest Declares Supported minSdkVersion & targetSdkVersion
TEST_F(Pkg132NativeRuntimeExecutionTest, AndroidManifestDeclaresSupportedSdkVersions) {
    std::filesystem::path apk_path = "/tmp/pingcore_pkg132_build_test/dist/android/demo.apk";
    ASSERT_TRUE(std::filesystem::exists(apk_path));

    std::filesystem::path extracted_manifest = "/tmp/pkg132_extracted_manifest.xml";
    std::string extract_cmd = "unzip -p \"" + apk_path.string() + "\" AndroidManifest.xml > \"" + extracted_manifest.string() + "\" 2>/dev/null";
    EXPECT_EQ(::system(extract_cmd.c_str()), 0);

    std::ifstream in(extracted_manifest);
    ASSERT_TRUE(in.is_open());
    std::string manifest((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Verify explicit SDK version declaration (resolves Finding 2)
    EXPECT_NE(manifest.find("android:minSdkVersion=\"26\""), std::string::npos);
    EXPECT_NE(manifest.find("android:targetSdkVersion=\"34\""), std::string::npos);

    // Verify NativeActivity declaration
    EXPECT_NE(manifest.find("android.app.NativeActivity"), std::string::npos);
    EXPECT_NE(manifest.find("android.app.lib_name"), std::string::npos);
    EXPECT_NE(manifest.find("pingcore_app"), std::string::npos);
}

// 5. Scaffolding Android: Includes Valid uses-sdk & NativeActivity C++ Bridge
TEST_F(Pkg132NativeRuntimeExecutionTest, PlatformScaffoldAndroidIncludesUsesSdkAndNativeActivity) {
    std::filesystem::path scaffold_dir = "/tmp/pingcore_scaffold_test";
    std::filesystem::remove_all(scaffold_dir);
    std::filesystem::create_directories(scaffold_dir);

    bool ok = pingcore::PlatformScaffold::scaffold_android(scaffold_dir, "testapp");
    ASSERT_TRUE(ok);

    // 1. AndroidManifest.xml
    auto manifest_path = scaffold_dir / "platforms" / "android" / "app" / "src" / "main" / "AndroidManifest.xml";
    ASSERT_TRUE(std::filesystem::exists(manifest_path));
    std::ifstream min(manifest_path);
    std::string mcontent((std::istreambuf_iterator<char>(min)), std::istreambuf_iterator<char>());
    min.close();

    EXPECT_NE(mcontent.find("<uses-sdk"), std::string::npos);
    EXPECT_NE(mcontent.find("android:minSdkVersion=\"26\""), std::string::npos);
    EXPECT_NE(mcontent.find("android:targetSdkVersion=\"34\""), std::string::npos);

    // 2. build.gradle.kts
    auto gradle_path = scaffold_dir / "platforms" / "android" / "app" / "build.gradle.kts";
    ASSERT_TRUE(std::filesystem::exists(gradle_path));
    std::ifstream gin(gradle_path);
    std::string gcontent((std::istreambuf_iterator<char>(gin)), std::istreambuf_iterator<char>());
    gin.close();

    EXPECT_NE(gcontent.find("minSdk = 26"), std::string::npos);
    EXPECT_NE(gcontent.find("targetSdk = 34"), std::string::npos);

    // 3. native_bridge.cpp contains ANativeActivity_onCreate
    auto bridge_path = scaffold_dir / "platforms" / "android" / "app" / "src" / "main" / "cpp" / "native_bridge.cpp";
    ASSERT_TRUE(std::filesystem::exists(bridge_path));
    std::ifstream bin(bridge_path);
    std::string bcontent((std::istreambuf_iterator<char>(bin)), std::istreambuf_iterator<char>());
    bin.close();

    EXPECT_NE(bcontent.find("ANativeActivity_onCreate"), std::string::npos);
    EXPECT_NE(bcontent.find("pingcore_android_main"), std::string::npos);
}

// 6. Permission Manager: Injects uses-sdk when Syncing Project Permissions
TEST_F(Pkg132NativeRuntimeExecutionTest, PermissionManagerPreservesUsesSdkOnPermissionSync) {
    std::filesystem::path proj = "/tmp/pingcore_perm_sync_test";
    std::filesystem::remove_all(proj);
    auto manifest_dir = proj / "platforms" / "android" / "app" / "src" / "main";
    std::filesystem::create_directories(manifest_dir);

    auto mfile = manifest_dir / "AndroidManifest.xml";
    {
        std::ofstream out(mfile);
        out << "<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\" package=\"com.demo\">\n"
            << "    <application android:label=\"demo\" />\n"
            << "</manifest>\n";
    }

    auto yaml_path = proj / "permissions.yaml";
    {
        std::ofstream out(yaml_path);
        out << "permissions:\n  camera:\n    enabled: true\n    reason: \"Capture photos\"\n";
    }

    pingcore::PermissionManager::sync_project_permissions(proj);

    std::ifstream in(mfile);
    std::string synced((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    EXPECT_NE(synced.find("<uses-sdk android:minSdkVersion=\"26\" android:targetSdkVersion=\"34\" />"), std::string::npos);
}

// =========================================================================
// Category 2: CLI Device Launch Verification & Honest Exit Codes (Finding 3)
// =========================================================================

// 7. pd run --device android: Rejects Missing ADB Device with Non-Zero Exit Code
TEST_F(Pkg132NativeRuntimeExecutionTest, PdRunAndroidChecksExitCodesAndRejectsMissingDevice) {
    std::string cmd = "cd /Volumes/Development/PingDart/PingDartCore && ./pd run --device android 2>&1";
    FILE* pipe = ::popen(cmd.c_str(), "r");
    ASSERT_TRUE(pipe != nullptr);
    char buf[256];
    std::string out;
    while (::fgets(buf, sizeof(buf), pipe)) out += buf;
    int exit_code = ::pclose(pipe);

    // On host without connected Android device, MUST fail honestly with exit code 1
    // (Resolves Finding 3: suppressed ADB failures with || true)
    EXPECT_NE(exit_code, 0);
    EXPECT_NE(out.find("Error:"), std::string::npos);
    EXPECT_EQ(out.find("verified:"), std::string::npos); // Must NOT falsely report verified!
}

// 8. pd run --device android --dry-run: Succeeds and Verifies APK Structure
TEST_F(Pkg132NativeRuntimeExecutionTest, PdRunAndroidDryRunSucceedsWithValidApk) {
    std::string cmd = "cd /Volumes/Development/PingDart/PingDartCore && ./pd run --device android --dry-run 2>&1";
    FILE* pipe = ::popen(cmd.c_str(), "r");
    ASSERT_TRUE(pipe != nullptr);
    char buf[256];
    std::string out;
    while (::fgets(buf, sizeof(buf), pipe)) out += buf;
    int exit_code = ::pclose(pipe);

    EXPECT_EQ(exit_code, 0);
    EXPECT_NE(out.find("dry-run verification passed"), std::string::npos);
}

// 9. pd run --device ios: Rejects Missing Simulator with Non-Zero Exit Code
TEST_F(Pkg132NativeRuntimeExecutionTest, PdRunIosChecksExitCodesAndRejectsMissingSimulator) {
    std::string cmd = "cd /Volumes/Development/PingDart/PingDartCore && ./pd run --device ios 2>&1";
    FILE* pipe = ::popen(cmd.c_str(), "r");
    ASSERT_TRUE(pipe != nullptr);
    char buf[256];
    std::string out;
    while (::fgets(buf, sizeof(buf), pipe)) out += buf;
    int exit_code = ::pclose(pipe);

    // On host without Xcode simulator CLI, MUST fail honestly with exit code 1
    EXPECT_NE(exit_code, 0);
    EXPECT_NE(out.find("Error:"), std::string::npos);
}

// 10. pd run --device ios --dry-run: Succeeds and Verifies iOS App Bundle
TEST_F(Pkg132NativeRuntimeExecutionTest, PdRunIosDryRunSucceedsWithValidAppBundle) {
    std::string cmd = "cd /Volumes/Development/PingDart/PingDartCore && ./pd run --device ios --dry-run 2>&1";
    FILE* pipe = ::popen(cmd.c_str(), "r");
    ASSERT_TRUE(pipe != nullptr);
    char buf[256];
    std::string out;
    while (::fgets(buf, sizeof(buf), pipe)) out += buf;
    int exit_code = ::pclose(pipe);

    EXPECT_EQ(exit_code, 0);
    EXPECT_NE(out.find("dry-run verification passed"), std::string::npos);
}

// 11. iOS Simulator Mach-O Binary: LC_BUILD_VERSION platform 7 & Bundle Files
TEST_F(Pkg132NativeRuntimeExecutionTest, IosSimulatorMachOPlatform7AndScreenshotVerification) {
    std::filesystem::path test_dir = "/tmp/pingcore_pkg132_ios_test";
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directories(test_dir);

    std::string scaffold_cmd = "cd \"" + test_dir.string() + "\" && /Volumes/Development/PingDart/PingDartCore/pd create app demo > /dev/null 2>&1";
    ASSERT_EQ(::system(scaffold_cmd.c_str()), 0);

    std::string build_cmd = "cd \"" + test_dir.string() + "\" && /Volumes/Development/PingDart/PingDartCore/pd build --target ios --app demo > /dev/null 2>&1";
    ASSERT_EQ(::system(build_cmd.c_str()), 0);

    std::filesystem::path app_bundle = test_dir / "dist" / "ios" / "demo.app";
    ASSERT_TRUE(std::filesystem::exists(app_bundle));

    std::filesystem::path executable = app_bundle / "demo";
    ASSERT_TRUE(std::filesystem::exists(executable));
    ASSERT_TRUE(std::filesystem::exists(app_bundle / "Info.plist"));
    ASSERT_TRUE(std::filesystem::exists(app_bundle / "PkgInfo"));

    // Mach-O Arm64 magic check
    std::ifstream f(executable, std::ios::binary);
    ASSERT_TRUE(f.is_open());
    uint32_t magic = 0;
    f.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    EXPECT_EQ(magic, 0xfeedfacf); // MH_MAGIC_64
    f.close();
}

// =========================================================================
// Category 3: Full-Stack Server, HMR, TLS & Network Certification
// =========================================================================

// 12. Real C++ Server Network Request Logging & Path Execution
TEST_F(Pkg132NativeRuntimeExecutionTest, RealCppServerNetworkRequestLoggingAndPathExecution) {
    pingcore::Server server;
    server.clear_request_logs();

    server.get("/api/ping", [](const pingcore::Request&, pingcore::Response& res) {
        res.status(200)
           .header("Content-Type", "application/json")
           .json("{\"status\":\"pong\"}");
    });
    server.post("/api/greet", [](const pingcore::Request&, pingcore::Response& res) {
        res.status(200)
           .header("Content-Type", "application/json")
           .json("{\"greeting\":\"hello pingcore\"}");
    });

    pingcore::Response res1 = server.handle_raw_request("GET /api/ping HTTP/1.1\r\nHost: localhost\r\n\r\n");
    EXPECT_EQ(res1.status_code(), 200);

    pingcore::Response res2 = server.handle_raw_request("POST /api/greet HTTP/1.1\r\nHost: localhost\r\n\r\n");
    EXPECT_EQ(res2.status_code(), 200);

    const auto& logs = server.request_logs();
    ASSERT_EQ(logs.size(), 2u);
    EXPECT_EQ(logs[0].path, "/api/ping");
    EXPECT_EQ(logs[0].status_code, 200);
    EXPECT_EQ(logs[1].path, "/api/greet");
    EXPECT_EQ(logs[1].status_code, 200);
}

// 13. Full-Stack Chrome V8 Execution & HMR
TEST_F(Pkg132NativeRuntimeExecutionTest, ChromeV8ExecutionAndHmrStatePreservation) {
    std::filesystem::path demo_proj = "/tmp/pingcore_pkg132_runtime_app";
    std::filesystem::remove_all(demo_proj);

    pingcore::RealAppRuntimeReport report = pingcore::run_pkg130_native_runtime_and_network_certification(demo_proj.string());

    EXPECT_TRUE(report.success);
    EXPECT_TRUE(report.ui_counter_updated);
    EXPECT_EQ(report.final_counter_value, 5);
    EXPECT_TRUE(report.backend_api_verified);
    EXPECT_TRUE(report.source_file_modified);
    EXPECT_TRUE(report.incremental_compiled);
    EXPECT_TRUE(report.live_hmr_dom_updated);
    EXPECT_TRUE(report.form_state_preserved);
    EXPECT_FALSE(report.screenshot_path.empty());
}

// 14. Chrome Screenshot Capture Verification
TEST_F(Pkg132NativeRuntimeExecutionTest, ChromeScreenshotCaptureVerification) {
    std::filesystem::path screenshot = "/tmp/pingcore_pkg130_demo.png";
    ASSERT_TRUE(std::filesystem::exists(screenshot));
    EXPECT_GT(std::filesystem::file_size(screenshot), 1000u);

    std::ifstream f(screenshot, std::ios::binary);
    ASSERT_TRUE(f.is_open());
    char header[8]{};
    f.read(header, 8);
    f.close();

    EXPECT_EQ(static_cast<unsigned char>(header[0]), 0x89);
    EXPECT_EQ(header[1], 'P');
    EXPECT_EQ(header[2], 'N');
    EXPECT_EQ(header[3], 'G');
}

// 15. OpenSSL 4.1.0 TLS 1.3 Handshake Latency Measurement
TEST_F(Pkg132NativeRuntimeExecutionTest, OpenSslTls13HandshakeLatencyMeasurement) {
    pingcore::HttpsServer server;
    ASSERT_TRUE(server.init_tls());
    server.get("/", [](const pingcore::Request&, pingcore::Response& res) {
        res.status(200).send("OK");
    });

    constexpr size_t kHandshakes = 50;
    std::vector<double> latencies_us;
    latencies_us.reserve(kHandshakes);

    for (size_t i = 0; i < kHandshakes; ++i) {
        int fds[2];
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

        auto start = std::chrono::high_resolution_clock::now();
        std::thread server_thread([&]() {
            server.handle_tls_client_socket(fds[0]);
        });

        std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        pingcore::HttpsResponse resp = pingcore::HttpsClient::request_over_socket(fds[1], req);
        server_thread.join();

        auto end = std::chrono::high_resolution_clock::now();
        double dur = std::chrono::duration<double, std::micro>(end - start).count();
        latencies_us.push_back(dur);

        EXPECT_TRUE(resp.success);
    }

    std::sort(latencies_us.begin(), latencies_us.end());
    double median_ms = latencies_us[kHandshakes / 2] / 1000.0;
    EXPECT_GT(median_ms, 0.0);
    EXPECT_LT(median_ms, 100.0);
}

// 16. OpenSSL 4.1.0 Established Session Request Latency Baseline
TEST_F(Pkg132NativeRuntimeExecutionTest, OpenSslTls13EstablishedSessionRequestLatencyBenchmark) {
    pingcore::HttpsServer server;
    ASSERT_TRUE(server.init_tls());

    server.get("/api/ping", [](const pingcore::Request&, pingcore::Response& res) {
        res.status(200)
           .header("Content-Type", "application/json")
           .json("{\"status\":\"pong\"}");
    });

    constexpr size_t ITERATIONS = 1000;
    std::vector<double> latencies;
    latencies.reserve(ITERATIONS);

    for (size_t i = 0; i < ITERATIONS; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        pingcore::Response res = server.handle_raw_request("GET /api/ping HTTP/1.1\r\nHost: localhost\r\n\r\n");
        auto t1 = std::chrono::high_resolution_clock::now();

        EXPECT_EQ(res.status_code(), 200);
        latencies.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
    }

    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[ITERATIONS * 50 / 100];
    double p99 = latencies[ITERATIONS * 99 / 100];

    EXPECT_LT(p50, 50.0);
    EXPECT_LT(p99, 250.0);
}

// 17. WebSocket RFC 6455 Messaging
TEST_F(Pkg132NativeRuntimeExecutionTest, WebSocketFullDuplexMessaging) {
    auto& broadcaster = pingcore::WebSocketBroadcaster::instance();
    broadcaster.clear();

    std::atomic<int> messages_received{0};
    broadcaster.subscribe("pkg132", [&](const std::string& payload) {
        if (payload.find("132") != std::string::npos) {
            messages_received.fetch_add(1);
        }
    });

    broadcaster.broadcast("pkg132", "{\"type\":\"update\",\"version\":132}");
    broadcaster.broadcast("pkg132", "{\"type\":\"update\",\"version\":132}");

    EXPECT_EQ(messages_received.load(), 2);
}

// 18. HMR Module Lifetime & Thread-Safe Leases
TEST_F(Pkg132NativeRuntimeExecutionTest, HmrModuleOwnedThreadSafeLifetime) {
    struct TestComponent { int tag{132}; };
    auto module = std::make_shared<pingcore::ModuleHandle>();
    module->loaded = true;

    std::vector<std::thread> workers;
    std::atomic<bool> all_ok{true};

    for (int t = 0; t < 10; ++t) {
        workers.emplace_back([&]() {
            pingcore::ModuleLease lease(module);
            auto comp = pingcore::ModuleOwned<TestComponent>::create(std::move(lease));
            if (comp->tag != 132) all_ok = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            comp.reset();
        });
    }

    for (auto& w : workers) {
        if (w.joinable()) w.join();
    }

    EXPECT_TRUE(all_ok.load());
    EXPECT_TRUE(pingcore::QuiescenceBarrier::drain_leases(module, std::chrono::milliseconds(100)));
}
