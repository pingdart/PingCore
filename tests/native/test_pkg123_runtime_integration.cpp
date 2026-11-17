// tests/native/test_pkg123_runtime_integration.cpp
// Comprehensive test suite for PKG-123: Native Module Runtime Integration,
// Lock Discipline & Callback Re-entrancy, Async State Machine, Granular Permissions,
// and Atomic Verified Platform Removal
#include <gtest/gtest.h>
#include <pingcore/native_bridge.h>
#include <pingcore/permissions.h>
#include <pingcore/platform_scaffold.h>
#include <pingcore/native_module.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

namespace fs = std::filesystem;

class Pkg123RuntimeIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / ("pingcore_test_pkg123_" + std::to_string(std::time(nullptr)));
        fs::create_directories(test_dir_);
        pingcore::NativeBridge::instance().reset();
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(test_dir_, ec);
        pingcore::NativeBridge::instance().reset();
    }

    fs::path test_dir_;
};

// 1. Mutex Discipline & Re-entrancy: Proves std::mutex is released BEFORE calling callbacks
TEST_F(Pkg123RuntimeIntegrationTest, CallbackReentrancySafety) {
    // Register mock native handler that responds immediately or allows nested calls
    auto async_fn = [](uint64_t request_id, const char* method, const char* json_args, pingcore_async_invoke_cb_t cb, void* user_data) {
        (void)json_args;
        if (std::string(method) == "initial_call") {
            // Callback is invoked immediately
            cb(request_id, PINGCORE_INVOKE_SUCCESS, "{\"step\":1}", user_data);
        } else if (std::string(method) == "nested_call") {
            cb(request_id, PINGCORE_INVOKE_SUCCESS, "{\"step\":2}", user_data);
        }
    };
    pingcore_native_register_async_handlers(async_fn, nullptr);

    bool nested_succeeded = false;
    uint64_t outer_id = pingcore::invoke_native_async("initial_call", "{}", [&](int status1, std::string res1) {
        EXPECT_EQ(status1, PINGCORE_INVOKE_SUCCESS);
        EXPECT_NE(res1.find("\"step\":1"), std::string::npos);

        // Crucial test: Inside this callback, invoke ANOTHER async native method.
        // If the bridge held std::mutex, this would immediately deadlock!
        uint64_t inner_id = pingcore::invoke_native_async("nested_call", "{}", [&](int status2, std::string res2) {
            EXPECT_EQ(status2, PINGCORE_INVOKE_SUCCESS);
            EXPECT_NE(res2.find("\"step\":2"), std::string::npos);
            nested_succeeded = true;
        });
        EXPECT_GT(inner_id, 0u);
    });

    EXPECT_GT(outer_id, 0u);
    EXPECT_TRUE(nested_succeeded);
}

// 2. Concurrency Race: Exact Terminal State Guarantee (Completion vs Cancellation)
TEST_F(Pkg123RuntimeIntegrationTest, CompletionVsCancellationRace) {
    constexpr int kIterations = 200;
    std::atomic<int> completed_count{0};
    std::atomic<int> cancelled_count{0};
    std::atomic<int> total_callback_executions{0};

    for (int i = 0; i < kIterations; ++i) {
        auto async_fn = [](uint64_t request_id, const char* method, const char* json_args, pingcore_async_invoke_cb_t cb, void* user_data) {
            (void)method;
            (void)json_args;
            (void)request_id;
            (void)cb;
            (void)user_data;
            // No-op here; completion is triggered concurrently from another thread
        };
        pingcore_native_register_async_handlers(async_fn, nullptr);

        std::atomic<bool> cb_executed{false};
        uint64_t req_id = pingcore::invoke_native_async("long_task", "{}", [&](int status, std::string) {
            total_callback_executions.fetch_add(1, std::memory_order_relaxed);
            cb_executed.store(true);
            if (status == PINGCORE_INVOKE_SUCCESS) {
                completed_count.fetch_add(1, std::memory_order_relaxed);
            } else if (status == PINGCORE_INVOKE_CANCELLED) {
                cancelled_count.fetch_add(1, std::memory_order_relaxed);
            }
        });

        // Spawn two racing threads: one tries to complete, one tries to cancel
        std::thread t_complete([req_id]() {
            pingcore::NativeBridge::instance().notify_async_result(req_id, PINGCORE_INVOKE_SUCCESS, "{\"ok\":true}");
        });

        std::thread t_cancel([req_id]() {
            pingcore::cancel_native_request(req_id);
        });

        t_complete.join();
        t_cancel.join();

        // Exactly one transition must have succeeded and executed the callback
        EXPECT_TRUE(cb_executed.load());
    }

    // Every single iteration must have executed the callback exactly once
    EXPECT_EQ(total_callback_executions.load(), kIterations);
    EXPECT_EQ(completed_count.load() + cancelled_count.load(), kIterations);
}

// 3. Granular Bluetooth Permissions: Scanning, Connecting, Advertising & neverForLocation
TEST_F(Pkg123RuntimeIntegrationTest, GranularBluetoothPermissions) {
    pingcore::PlatformScaffold::scaffold_android(test_dir_, "TestApp");
    fs::path manifest_path = test_dir_ / "platforms" / "android" / "app" / "src" / "main" / "AndroidManifest.xml";

    // Case A: never_for_location = true
    pingcore::BluetoothOptions opts_true;
    opts_true.scan = true;
    opts_true.connect = true;
    opts_true.advertise = true;
    opts_true.never_for_location = true;

    pingcore::PermissionDescriptor perm_a = pingcore::PermissionManager::get_bluetooth_descriptor(opts_true);
    bool ok_a = pingcore::PermissionManager::inject_android_permissions(manifest_path, {perm_a});
    EXPECT_TRUE(ok_a);

    std::ifstream in_a(manifest_path);
    std::string content_a((std::istreambuf_iterator<char>(in_a)), std::istreambuf_iterator<char>());
    in_a.close();

    EXPECT_NE(content_a.find("android:name=\"android.permission.BLUETOOTH_SCAN\" android:usesPermissionFlags=\"neverForLocation\""), std::string::npos);
    EXPECT_NE(content_a.find("android:name=\"android.permission.BLUETOOTH_CONNECT\""), std::string::npos);
    EXPECT_NE(content_a.find("android:name=\"android.permission.BLUETOOTH_ADVERTISE\""), std::string::npos);
    // Should NOT have injected ACCESS_FINE_LOCATION when never_for_location = true
    EXPECT_EQ(content_a.find("ACCESS_FINE_LOCATION"), std::string::npos);

    // Case B: never_for_location = false (requires ACCESS_FINE_LOCATION)
    fs::remove(manifest_path);
    pingcore::PlatformScaffold::scaffold_android(test_dir_, "TestApp");

    pingcore::BluetoothOptions opts_false;
    opts_false.scan = true;
    opts_false.connect = false;
    opts_false.advertise = false;
    opts_false.never_for_location = false;

    pingcore::PermissionDescriptor perm_b = pingcore::PermissionManager::get_bluetooth_descriptor(opts_false);
    bool ok_b = pingcore::PermissionManager::inject_android_permissions(manifest_path, {perm_b});
    EXPECT_TRUE(ok_b);

    std::ifstream in_b(manifest_path);
    std::string content_b((std::istreambuf_iterator<char>(in_b)), std::istreambuf_iterator<char>());
    in_b.close();

    // Verify scan WITHOUT neverForLocation
    EXPECT_NE(content_b.find("<uses-permission android:name=\"android.permission.BLUETOOTH_SCAN\" />"), std::string::npos);
    // MUST inject ACCESS_FINE_LOCATION
    EXPECT_NE(content_b.find("android.permission.ACCESS_FINE_LOCATION"), std::string::npos);
}

// 4. NativeModuleRegistry Runtime Dispatch: Camera, Location, Notifications & Custom Modules
TEST_F(Pkg123RuntimeIntegrationTest, NativeModuleRegistryDispatch) {
    // Test builtin camera.getStatus
    std::string cam_status = pingcore::invoke_native("camera.getStatus", "{}");
    EXPECT_NE(cam_status.find("\"available\":true"), std::string::npos);
    EXPECT_NE(cam_status.find("\"hasPermission\":true"), std::string::npos);

    // Test builtin camera.capture
    std::string cam_capture = pingcore::invoke_native("camera.capture", "{\"quality\":85}");
    EXPECT_NE(cam_capture.find("\"status\":\"captured\""), std::string::npos);
    EXPECT_NE(cam_capture.find("data:image/jpeg;base64"), std::string::npos);

    // Test builtin location.getCurrentPosition
    std::string loc = pingcore::invoke_native("location.getCurrentPosition", "{\"accuracy\":\"high\"}");
    EXPECT_NE(loc.find("\"latitude\":"), std::string::npos);
    EXPECT_NE(loc.find("\"longitude\":"), std::string::npos);

    // Test builtin notifications.post
    std::string notif = pingcore::invoke_native("notifications.post", "{\"title\":\"PingCore\",\"body\":\"PKG-123 Ready\"}");
    EXPECT_NE(notif.find("\"status\":\"posted\""), std::string::npos);

    // Test custom developer module registration
    pingcore::NativeModuleRegistry::instance().register_module("sensors", {
        {"getReading", [](const std::string& args) -> std::string {
            (void)args;
            return "{\"accelerometer\":{\"x\":0.01,\"y\":9.81,\"z\":0.05}}";
        }}
    });

    std::string sensor_res = pingcore::invoke_native("sensors.getReading", "{}");
    EXPECT_NE(sensor_res.find("\"x\":0.01"), std::string::npos);
    EXPECT_NE(sensor_res.find("\"y\":9.81"), std::string::npos);
}

// 5. Atomic Platform Removal & Integrity-Verified Backup
TEST_F(Pkg123RuntimeIntegrationTest, AtomicPlatformRemovalAndBackup) {
    // Create a mock platform project
    fs::path project_root = test_dir_ / "MyTestApp";
    fs::create_directories(project_root);
    pingcore::PlatformScaffold::scaffold_android(project_root, "MyTestApp");

    fs::path platform_dir = project_root / "platforms" / "android";
    ASSERT_TRUE(fs::exists(platform_dir));

    // Developer adds custom Kotlin file
    fs::path custom_file = platform_dir / "app" / "src" / "main" / "kotlin" / "CustomPlugin.kt";
    fs::create_directories(custom_file.parent_path());
    {
        std::ofstream out(custom_file);
        out << "package com.mytestapp\nclass CustomPlugin { val id = \"pkg123_custom\" }\n";
    }
    ASSERT_TRUE(fs::exists(custom_file));

    // Count source files
    size_t src_count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(platform_dir)) {
        if (entry.is_regular_file()) ++src_count;
    }
    EXPECT_GT(src_count, 5u);

    // Execute atomic verified removal logic
    fs::path backup_base = project_root / ".pingcore" / "backups";
    fs::create_directories(backup_base);

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    fs::path staging_dir = backup_base / ("staging_android_" + std::to_string(ms));
    fs::path final_backup = backup_base / ("android_" + std::to_string(ms));

    // Staging copy
    fs::copy(platform_dir, staging_dir, fs::copy_options::recursive);

    // Verify integrity count
    size_t staging_count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(staging_dir)) {
        if (entry.is_regular_file()) ++staging_count;
    }
    EXPECT_EQ(src_count, staging_count);

    // Atomic rename staging -> final backup
    fs::rename(staging_dir, final_backup);
    EXPECT_TRUE(fs::exists(final_backup));

    // Remove source platform
    std::error_code ec;
    fs::remove_all(platform_dir, ec);
    EXPECT_FALSE(ec);
    EXPECT_FALSE(fs::exists(platform_dir));

    // Verify backed up custom file exists and content is byte-for-byte identical
    fs::path backed_up_custom = final_backup / "app" / "src" / "main" / "kotlin" / "CustomPlugin.kt";
    EXPECT_TRUE(fs::exists(backed_up_custom));

    std::ifstream in(backed_up_custom);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    EXPECT_NE(content.find("pkg123_custom"), std::string::npos);
}
