// tests/native/test_pkg121_native_platforms.cpp
// Comprehensive test suite for PKG-121: Native Platform Scaffolding,
// Permission Management, and Native Bridge C ABI SDK
#include <gtest/gtest.h>
#include <pingcore/native_bridge.h>
#include <pingcore/permissions.h>
#include <pingcore/platform_scaffold.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class Pkg121NativePlatformsTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / ("pingcore_test_pkg121_" + std::to_string(std::time(nullptr)));
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

TEST_F(Pkg121NativePlatformsTest, AndroidPlatformScaffoldStructure) {
    bool ok = pingcore::PlatformScaffold::scaffold_android(test_dir_, "TestApp");
    EXPECT_TRUE(ok);

    fs::path android_root = test_dir_ / "platforms" / "android";
    EXPECT_TRUE(fs::exists(android_root / "build.gradle.kts"));
    EXPECT_TRUE(fs::exists(android_root / "settings.gradle.kts"));
    EXPECT_TRUE(fs::exists(android_root / "gradle.properties"));
    EXPECT_TRUE(fs::exists(android_root / "gradlew"));
    EXPECT_TRUE(fs::exists(android_root / "gradle" / "wrapper" / "gradle-wrapper.properties"));
    EXPECT_TRUE(fs::exists(android_root / "app" / "build.gradle.kts"));
    EXPECT_TRUE(fs::exists(android_root / "app" / "proguard-rules.pro"));
    EXPECT_TRUE(fs::exists(android_root / "app" / "src" / "main" / "AndroidManifest.xml"));
    EXPECT_TRUE(fs::exists(android_root / "app" / "src" / "main" / "java" / "com" / "pingcore" / "app" / "MainActivity.kt"));
    EXPECT_TRUE(fs::exists(android_root / "app" / "src" / "main" / "java" / "com" / "pingcore" / "app" / "PingCoreBridge.kt"));
    EXPECT_TRUE(fs::exists(android_root / "app" / "src" / "main" / "java" / "com" / "pingcore" / "app" / "native" / "Permissions.kt"));
    EXPECT_TRUE(fs::exists(android_root / "app" / "src" / "main" / "cpp" / "native_bridge.cpp"));
    EXPECT_TRUE(fs::exists(android_root / "app" / "src" / "main" / "cpp" / "CMakeLists.txt"));
    EXPECT_TRUE(fs::exists(android_root / "app" / "src" / "main" / "res" / "values" / "strings.xml"));

    // Check file contents
    std::ifstream main_kt(android_root / "app" / "src" / "main" / "java" / "com" / "pingcore" / "app" / "MainActivity.kt");
    std::string main_content((std::istreambuf_iterator<char>(main_kt)), std::istreambuf_iterator<char>());
    EXPECT_NE(main_content.find("registerForActivityResult"), std::string::npos);
    EXPECT_NE(main_content.find("System.loadLibrary(\"pingcore_app\")"), std::string::npos);

    std::ifstream bridge_kt(android_root / "app" / "src" / "main" / "java" / "com" / "pingcore" / "app" / "PingCoreBridge.kt");
    std::string bridge_content((std::istreambuf_iterator<char>(bridge_kt)), std::istreambuf_iterator<char>());
    EXPECT_NE(bridge_content.find("external fun nativeNotifyPermissionResult"), std::string::npos);

    std::ifstream cpp_bridge(android_root / "app" / "src" / "main" / "cpp" / "native_bridge.cpp");
    std::string cpp_content((std::istreambuf_iterator<char>(cpp_bridge)), std::istreambuf_iterator<char>());
    EXPECT_NE(cpp_content.find("pingcore_native_register_platform_handlers"), std::string::npos);
    EXPECT_NE(cpp_content.find("JNI_OnLoad"), std::string::npos);
}

TEST_F(Pkg121NativePlatformsTest, IOSPlatformScaffoldStructure) {
    bool ok = pingcore::PlatformScaffold::scaffold_ios(test_dir_, "TestApp");
    EXPECT_TRUE(ok);

    fs::path ios_root = test_dir_ / "platforms" / "ios";
    EXPECT_TRUE(fs::exists(ios_root / "MyApp.xcodeproj" / "project.pbxproj"));
    EXPECT_TRUE(fs::exists(ios_root / "MyApp" / "AppDelegate.swift"));
    EXPECT_TRUE(fs::exists(ios_root / "MyApp" / "SceneDelegate.swift"));
    EXPECT_TRUE(fs::exists(ios_root / "MyApp" / "Info.plist"));
    EXPECT_TRUE(fs::exists(ios_root / "MyApp" / "MyApp.entitlements"));
    EXPECT_TRUE(fs::exists(ios_root / "MyApp" / "NativeBridge.mm"));
    EXPECT_TRUE(fs::exists(ios_root / "MyApp" / "Native" / "Permissions.swift"));
    EXPECT_TRUE(fs::exists(ios_root / "MyApp" / "Assets.xcassets" / "AppIcon.appiconset" / "Contents.json"));
    EXPECT_TRUE(fs::exists(ios_root / "MyApp" / "Base.lproj" / "LaunchScreen.storyboard"));
    EXPECT_TRUE(fs::exists(ios_root / "ExportOptions.plist"));

    // Check file contents
    std::ifstream pbx(ios_root / "MyApp.xcodeproj" / "project.pbxproj");
    std::string pbx_content((std::istreambuf_iterator<char>(pbx)), std::istreambuf_iterator<char>());
    EXPECT_NE(pbx_content.find("CLANG_CXX_LANGUAGE_STANDARD = \"c++20\""), std::string::npos);

    std::ifstream app_swift(ios_root / "MyApp" / "AppDelegate.swift");
    std::string app_content((std::istreambuf_iterator<char>(app_swift)), std::istreambuf_iterator<char>());
    EXPECT_NE(app_content.find("PingCoreNativeBridgeSetup()"), std::string::npos);

    std::ifstream native_mm(ios_root / "MyApp" / "NativeBridge.mm");
    std::string mm_content((std::istreambuf_iterator<char>(native_mm)), std::istreambuf_iterator<char>());
    EXPECT_NE(mm_content.find("AVCaptureDevice requestAccessForMediaType"), std::string::npos);
    EXPECT_NE(mm_content.find("pingcore_native_register_platform_handlers"), std::string::npos);
}

TEST_F(Pkg121NativePlatformsTest, DesktopAndWebPlatformScaffold) {
    EXPECT_TRUE(pingcore::PlatformScaffold::scaffold_macos(test_dir_, "TestApp"));
    EXPECT_TRUE(pingcore::PlatformScaffold::scaffold_windows(test_dir_, "TestApp"));
    EXPECT_TRUE(pingcore::PlatformScaffold::scaffold_linux(test_dir_, "TestApp"));
    EXPECT_TRUE(pingcore::PlatformScaffold::scaffold_web(test_dir_, "TestApp"));

    EXPECT_TRUE(fs::exists(test_dir_ / "platforms" / "macos" / "main.mm"));
    EXPECT_TRUE(fs::exists(test_dir_ / "platforms" / "windows" / "main.cpp"));
    EXPECT_TRUE(fs::exists(test_dir_ / "platforms" / "linux" / "main.cpp"));
    EXPECT_TRUE(fs::exists(test_dir_ / "platforms" / "web" / "index.html"));
}

TEST_F(Pkg121NativePlatformsTest, DeclarativePermissionsParsingAndSerialization) {
    std::string yaml_input =
R"(name: TestApp
version: 1.0.0

permissions:
  camera:
    enabled: true
    reason: "Capture profile photos"
  microphone:
    enabled: true
    reason: "Record audio notes"
  location:
    enabled: false
    reason: "Find stores"
)";

    auto perms = pingcore::PermissionManager::parse_yaml(yaml_input);
    ASSERT_EQ(perms.size(), 3u);

    EXPECT_EQ(perms[0].type, pingcore::PermissionType::Camera);
    EXPECT_TRUE(perms[0].enabled);
    EXPECT_EQ(perms[0].reason, "Capture profile photos");

    EXPECT_EQ(perms[1].type, pingcore::PermissionType::Microphone);
    EXPECT_TRUE(perms[1].enabled);
    EXPECT_EQ(perms[1].reason, "Record audio notes");

    EXPECT_EQ(perms[2].type, pingcore::PermissionType::Location);
    EXPECT_FALSE(perms[2].enabled);

    // Serialization
    std::string serialized = pingcore::PermissionManager::serialize_yaml(perms);
    EXPECT_NE(serialized.find("camera:"), std::string::npos);
    EXPECT_NE(serialized.find("Capture profile photos"), std::string::npos);
}

TEST_F(Pkg121NativePlatformsTest, AndroidManifestPermissionInjection) {
    pingcore::PlatformScaffold::scaffold_android(test_dir_, "TestApp");
    fs::path manifest_path = test_dir_ / "platforms" / "android" / "app" / "src" / "main" / "AndroidManifest.xml";

    std::vector<pingcore::PermissionDescriptor> perms = {
        pingcore::PermissionManager::get_descriptor(pingcore::PermissionType::Camera),
        pingcore::PermissionManager::get_descriptor(pingcore::PermissionType::Microphone)
    };

    bool ok = pingcore::PermissionManager::inject_android_permissions(manifest_path, perms);
    EXPECT_TRUE(ok);

    std::ifstream in(manifest_path);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    EXPECT_NE(content.find("<uses-permission android:name=\"android.permission.CAMERA\" />"), std::string::npos);
    EXPECT_NE(content.find("<uses-permission android:name=\"android.permission.RECORD_AUDIO\" />"), std::string::npos);

    // Re-inject: assert duplicate is NOT added!
    pingcore::PermissionManager::inject_android_permissions(manifest_path, perms);
    std::ifstream in2(manifest_path);
    std::string content2((std::istreambuf_iterator<char>(in2)), std::istreambuf_iterator<char>());
    in2.close();

    size_t first = content2.find("android.permission.CAMERA");
    size_t second = content2.find("android.permission.CAMERA", first + 1);
    EXPECT_EQ(second, std::string::npos); // Exactly one occurrence
}

TEST_F(Pkg121NativePlatformsTest, IOSInfoPlistUsageDescriptionInjection) {
    pingcore::PlatformScaffold::scaffold_ios(test_dir_, "TestApp");
    fs::path plist_path = test_dir_ / "platforms" / "ios" / "MyApp" / "Info.plist";

    std::vector<pingcore::PermissionDescriptor> perms = {
        pingcore::PermissionManager::get_descriptor(pingcore::PermissionType::Camera),
        pingcore::PermissionManager::get_descriptor(pingcore::PermissionType::Microphone)
    };

    bool ok = pingcore::PermissionManager::inject_ios_plist_permissions(plist_path, perms);
    EXPECT_TRUE(ok);

    std::ifstream in(plist_path);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    EXPECT_NE(content.find("<key>NSCameraUsageDescription</key>"), std::string::npos);
    EXPECT_NE(content.find("<key>NSMicrophoneUsageDescription</key>"), std::string::npos);

    // Re-inject: assert duplicate is NOT added!
    pingcore::PermissionManager::inject_ios_plist_permissions(plist_path, perms);
    std::ifstream in2(plist_path);
    std::string content2((std::istreambuf_iterator<char>(in2)), std::istreambuf_iterator<char>());
    in2.close();

    size_t first = content2.find("NSCameraUsageDescription");
    size_t second = content2.find("NSCameraUsageDescription", first + 1);
    EXPECT_EQ(second, std::string::npos);
}

TEST_F(Pkg121NativePlatformsTest, NonDestructiveUpdatesPreserveDeveloperEdits) {
    pingcore::PlatformScaffold::scaffold_android(test_dir_, "TestApp");
    fs::path main_kt = test_dir_ / "platforms" / "android" / "app" / "src" / "main" / "java" / "com" / "pingcore" / "app" / "MainActivity.kt";

    // Simulate developer adding custom code
    std::string custom_marker = "// DEVELOPER_CUSTOM_CODE_PRESERVED_12345";
    {
        std::ofstream out(main_kt, std::ios::app);
        out << "\n" << custom_marker << "\n";
    }

    // Run scaffold_android again
    bool ok = pingcore::PlatformScaffold::scaffold_android(test_dir_, "TestApp");
    EXPECT_TRUE(ok);

    // Verify developer modification was preserved
    std::ifstream in(main_kt);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find(custom_marker), std::string::npos);
}

TEST_F(Pkg121NativePlatformsTest, NativeBridgeCABIRegistrationAndInvocation) {
    static bool platform_called = false;
    static std::string last_method;

    auto req_fn = [](uint64_t request_id, pingcore_native_perm_t perm, pingcore_perm_cb_t cb, void* user_data) {
        (void)perm;
        if (cb) cb(request_id, perm, PINGCORE_PERM_STATUS_GRANTED, user_data);
    };

    auto invoke_fn = [](const char* method, const char* json_args, char* out_buf, size_t out_len) -> int {
        (void)json_args;
        platform_called = true;
        last_method = method;
        const char* resp = "{\"battery\":98,\"charging\":true}";
        strncpy(out_buf, resp, out_len - 1);
        out_buf[out_len - 1] = '\0';
        return 0;
    };

    pingcore_native_register_platform_handlers(req_fn, invoke_fn);

    // Call from C++ API
    std::string result = pingcore::invoke_native("getBatteryStatus", "{}");
    EXPECT_TRUE(platform_called);
    EXPECT_EQ(last_method, "getBatteryStatus");
    EXPECT_NE(result.find("\"battery\":98"), std::string::npos);

    // Call from C ABI raw API
    char out_buffer[256]{};
    int rc = pingcore_native_invoke_platform("getBatteryStatus", "{}", out_buffer, sizeof(out_buffer));
    EXPECT_EQ(rc, 0);
    EXPECT_NE(std::string(out_buffer).find("\"battery\":98"), std::string::npos);
}

TEST_F(Pkg121NativePlatformsTest, NativeBridgePermissionCallbacks) {
    static pingcore_native_perm_t requested_perm = PINGCORE_PERM_BLUETOOTH;
    static pingcore_perm_cb_t stored_cb = nullptr;
    static void* stored_user_data = nullptr;
    static uint64_t stored_req_id = 0;

    auto req_fn = [](uint64_t request_id, pingcore_native_perm_t perm, pingcore_perm_cb_t cb, void* user_data) {
        stored_req_id = request_id;
        requested_perm = perm;
        stored_cb = cb;
        stored_user_data = user_data;
    };

    pingcore_native_register_platform_handlers(req_fn, nullptr);

    bool callback_executed = false;
    pingcore::PermissionStatus reported_status = pingcore::PermissionStatus::Denied;

    uint64_t req_id = pingcore::request_permission(pingcore::PermissionType::Camera, [&](pingcore::PermissionStatus status) {
        callback_executed = true;
        reported_status = status;
    });

    EXPECT_GT(req_id, 0u);
    EXPECT_EQ(stored_req_id, req_id);
    EXPECT_EQ(requested_perm, PINGCORE_PERM_CAMERA);
    EXPECT_NE(stored_cb, nullptr);

    // Platform asynchronously invokes stored callback
    stored_cb(stored_req_id, requested_perm, PINGCORE_PERM_STATUS_GRANTED, stored_user_data);

    EXPECT_TRUE(callback_executed);
    EXPECT_EQ(reported_status, pingcore::PermissionStatus::Granted);
}

TEST_F(Pkg121NativePlatformsTest, ProjectPermissionSynchronization) {
    pingcore::PlatformScaffold::scaffold_all(test_dir_, "TestApp", {"android", "ios"});

    fs::path yaml_path = test_dir_ / "pingcore.yaml";
    std::ofstream yf(yaml_path);
    yf << "name: TestApp\nversion: 1.0.0\n\n";
    yf.close();

    // Add permissions to YAML
    pingcore::PermissionManager::add_permission_to_yaml(yaml_path, pingcore::PermissionType::Camera, "Scan QR items");
    pingcore::PermissionManager::add_permission_to_yaml(yaml_path, pingcore::PermissionType::Location, "Store locator");

    // Sync to projects
    bool ok = pingcore::PermissionManager::sync_project_permissions(test_dir_);
    EXPECT_TRUE(ok);

    // Check Android
    fs::path manifest_path = test_dir_ / "platforms" / "android" / "app" / "src" / "main" / "AndroidManifest.xml";
    std::ifstream in_m(manifest_path);
    std::string m_content((std::istreambuf_iterator<char>(in_m)), std::istreambuf_iterator<char>());
    EXPECT_NE(m_content.find("android.permission.CAMERA"), std::string::npos);
    EXPECT_NE(m_content.find("android.permission.ACCESS_FINE_LOCATION"), std::string::npos);

    // Check iOS
    fs::path plist_path = test_dir_ / "platforms" / "ios" / "MyApp" / "Info.plist";
    std::ifstream in_p(plist_path);
    std::string p_content((std::istreambuf_iterator<char>(in_p)), std::istreambuf_iterator<char>());
    EXPECT_NE(p_content.find("NSCameraUsageDescription"), std::string::npos);
    EXPECT_NE(p_content.find("Scan QR items"), std::string::npos);
    EXPECT_NE(p_content.find("NSLocationWhenInUseUsageDescription"), std::string::npos);
    EXPECT_NE(p_content.find("Store locator"), std::string::npos);
}
