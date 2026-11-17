// tests/native/test_pkg122_native_modules.cpp
// Comprehensive test suite for PKG-122: Native Module System,
// Version-Aware Permissions, Asynchronous C ABI, and Lifecycle Safety
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
#include <chrono>

namespace fs = std::filesystem;

class Pkg122NativeModulesTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / ("pingcore_test_pkg122_" + std::to_string(std::time(nullptr)));
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

TEST_F(Pkg122NativeModulesTest, VersionAwareAndroidPermissions) {
    pingcore::PlatformScaffold::scaffold_android(test_dir_, "TestApp");
    fs::path manifest_path = test_dir_ / "platforms" / "android" / "app" / "src" / "main" / "AndroidManifest.xml";

    std::vector<pingcore::PermissionDescriptor> perms = {
        pingcore::PermissionManager::get_descriptor(pingcore::PermissionType::Camera),
        pingcore::PermissionManager::get_descriptor(pingcore::PermissionType::Bluetooth),
        pingcore::PermissionManager::get_descriptor(pingcore::PermissionType::Storage),
        pingcore::PermissionManager::get_descriptor(pingcore::PermissionType::Notifications)
    };

    bool ok = pingcore::PermissionManager::inject_android_permissions(manifest_path, perms);
    EXPECT_TRUE(ok);

    std::ifstream in(manifest_path);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Verify Camera feature requirement
    EXPECT_NE(content.find("<uses-feature android:name=\"android.hardware.camera.any\" android:required=\"false\" />"), std::string::npos);

    // Verify Bluetooth modern and legacy attributes
    EXPECT_NE(content.find("android:name=\"android.permission.BLUETOOTH_SCAN\" android:usesPermissionFlags=\"neverForLocation\""), std::string::npos);
    EXPECT_NE(content.find("android:name=\"android.permission.BLUETOOTH\" android:maxSdkVersion=\"30\""), std::string::npos);

    // Verify Storage modern and legacy attributes
    EXPECT_NE(content.find("android:name=\"android.permission.READ_MEDIA_IMAGES\""), std::string::npos);
    EXPECT_NE(content.find("android:name=\"android.permission.READ_EXTERNAL_STORAGE\" android:maxSdkVersion=\"32\""), std::string::npos);

    // Verify Notifications
    EXPECT_NE(content.find("android:name=\"android.permission.POST_NOTIFICATIONS\""), std::string::npos);
}

TEST_F(Pkg122NativeModulesTest, NativeBridgeAsyncInvocation) {
    static bool async_invoked = false;
    static uint64_t captured_id = 0;

    auto async_fn = [](uint64_t request_id, const char* method, const char* json_args, pingcore_async_invoke_cb_t cb, void* user_data) {
        (void)method;
        (void)json_args;
        async_invoked = true;
        captured_id = request_id;
        if (cb) {
            cb(request_id, PINGCORE_INVOKE_SUCCESS, "{\"photo\":\"base64_data_123\"}", user_data);
        }
    };

    pingcore_native_register_async_handlers(async_fn, nullptr);

    bool callback_executed = false;
    int received_status = -999;
    std::string received_result;

    uint64_t req_id = pingcore::invoke_native_async("camera.capture", "{\"quality\":90}", [&](int status, std::string res) {
        callback_executed = true;
        received_status = status;
        received_result = std::move(res);
    });

    EXPECT_GT(req_id, 0u);
    EXPECT_TRUE(async_invoked);
    EXPECT_EQ(captured_id, req_id);
    EXPECT_TRUE(callback_executed);
    EXPECT_EQ(received_status, PINGCORE_INVOKE_SUCCESS);
    EXPECT_NE(received_result.find("\"photo\":\"base64_data_123\""), std::string::npos);
}

TEST_F(Pkg122NativeModulesTest, NativeBridgeCancellation) {
    static uint64_t cancelled_id = 0;

    auto async_fn = [](uint64_t, const char*, const char*, pingcore_async_invoke_cb_t, void*) {
        // Asynchronous operation pending in background
    };

    auto cancel_fn = [](uint64_t request_id) {
        cancelled_id = request_id;
    };

    pingcore_native_register_async_handlers(async_fn, cancel_fn);

    bool callback_executed = false;
    int received_status = PINGCORE_INVOKE_SUCCESS;

    uint64_t req_id = pingcore::invoke_native_async("location.startTracking", "{}", [&](int status, std::string) {
        callback_executed = true;
        received_status = status;
    });

    EXPECT_GT(req_id, 0u);
    EXPECT_FALSE(callback_executed);

    // Cancel operation
    pingcore::cancel_native_operation(req_id);

    EXPECT_EQ(cancelled_id, req_id);
    EXPECT_TRUE(callback_executed);
    EXPECT_EQ(received_status, PINGCORE_INVOKE_CANCELLED);
}

TEST_F(Pkg122NativeModulesTest, NativeBridgeDuplicateDispatchProtection) {
    int callback_count = 0;

    uint64_t req_id = pingcore::invoke_native_async("test.op", "{}", [&](int, std::string) {
        callback_count++;
    });

    EXPECT_EQ(callback_count, 1);

    // Simulate duplicate response from native OS layer
    pingcore::NativeBridge::instance().notify_async_result(req_id, PINGCORE_INVOKE_SUCCESS, "{\"late\":true}");
    pingcore::NativeBridge::instance().notify_async_result(req_id, PINGCORE_INVOKE_SUCCESS, "{\"late2\":true}");

    // Must still be called exactly once!
    EXPECT_EQ(callback_count, 1);
}

TEST_F(Pkg122NativeModulesTest, NativeBridgeBufferCapacityAndStatusCodes) {
    auto invoke_fn = [](const char*, const char*, char* out_buf, size_t out_len) -> int {
        const char* resp = "{\"error\":\"Not found\"}";
        size_t len = strlen(resp);
        if (out_len <= len) {
            return PINGCORE_INVOKE_BUFFER_TOO_SMALL;
        }
        memcpy(out_buf, resp, len + 1);
        return PINGCORE_INVOKE_METHOD_NOT_FOUND;
    };

    pingcore_native_register_platform_handlers(nullptr, invoke_fn);

    char tiny_buf[5]{};
    int rc = pingcore_native_invoke_platform("unknown.method", "{}", tiny_buf, sizeof(tiny_buf));
    EXPECT_EQ(rc, PINGCORE_INVOKE_BUFFER_TOO_SMALL);

    char normal_buf[128]{};
    int rc2 = pingcore_native_invoke_platform("unknown.method", "{}", normal_buf, sizeof(normal_buf));
    EXPECT_EQ(rc2, PINGCORE_INVOKE_METHOD_NOT_FOUND);

    int rc_invalid = pingcore_native_invoke_platform("test", "{}", nullptr, 0);
    EXPECT_EQ(rc_invalid, PINGCORE_INVOKE_INVALID_ARGUMENTS);
}

TEST_F(Pkg122NativeModulesTest, NativeModuleCreationAndStructure) {
    bool ok = pingcore::NativeModuleManager::create_module(test_dir_, "camera", "camera");
    EXPECT_TRUE(ok);

    fs::path mod_dir = test_dir_ / "native" / "camera";
    EXPECT_TRUE(fs::exists(mod_dir / "module.yaml"));
    EXPECT_TRUE(fs::exists(mod_dir / "camera.h"));
    EXPECT_TRUE(fs::exists(mod_dir / "camera.cpp"));
    EXPECT_TRUE(fs::exists(mod_dir / "android" / "CameraModule.kt"));
    EXPECT_TRUE(fs::exists(mod_dir / "ios" / "CameraModule.swift"));
    EXPECT_TRUE(fs::exists(mod_dir / "macos" / "CameraModule.swift"));
    EXPECT_TRUE(fs::exists(mod_dir / "windows" / "CameraModule.cpp"));

    // Verify Kotlin and Swift adapters contain valid class names
    std::ifstream kt_in(mod_dir / "android" / "CameraModule.kt");
    std::string kt_code((std::istreambuf_iterator<char>(kt_in)), std::istreambuf_iterator<char>());
    EXPECT_NE(kt_code.find("class CameraModule"), std::string::npos);

    std::ifstream swift_in(mod_dir / "ios" / "CameraModule.swift");
    std::string swift_code((std::istreambuf_iterator<char>(swift_in)), std::istreambuf_iterator<char>());
    EXPECT_NE(swift_code.find("@objc public class CameraModule"), std::string::npos);
}

TEST_F(Pkg122NativeModulesTest, NativeModuleListing) {
    pingcore::NativeModuleManager::create_module(test_dir_, "camera", "camera");
    pingcore::NativeModuleManager::create_module(test_dir_, "location", "location");
    pingcore::NativeModuleManager::create_module(test_dir_, "notifications", "notifications");

    auto modules = pingcore::NativeModuleManager::list_modules(test_dir_);
    EXPECT_EQ(modules.size(), 3u);

    bool has_camera = false, has_location = false, has_notifs = false;
    for (const auto& m : modules) {
        if (m == "camera") has_camera = true;
        if (m == "location") has_location = true;
        if (m == "notifications") has_notifs = true;
    }
    EXPECT_TRUE(has_camera);
    EXPECT_TRUE(has_location);
    EXPECT_TRUE(has_notifs);
}
