// tests/mobile/test_mobile_packaging.cpp
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "../../examples/shared_login/login_app.h"

using namespace pingdart::examples;

namespace {

std::filesystem::path resolve_platform_path(const std::string& rel) {
    const std::vector<std::string> prefixes = {"", "../", "../../", "../../../", "/Volumes/Development/PingDart/PingDartCore/"};
    for (const auto& p : prefixes) {
        if (std::filesystem::exists(p + rel)) return p + rel;
    }
    return rel;
}

} // namespace

// ============================================================================
// Android Packaging Manifest & Configuration Tests
// ============================================================================

TEST(MobilePackagingGoogleTest, AndroidManifestRequirementsVerification) {
    std::filesystem::path manifest_path = resolve_platform_path("platforms/android/app/src/main/AndroidManifest.xml");
    ASSERT_TRUE(std::filesystem::exists(manifest_path)) << "AndroidManifest.xml must exist in platforms/android";

    std::ifstream f(manifest_path);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    // 1. Vulkan hardware feature requirement
    EXPECT_NE(content.find("android.hardware.vulkan.version"), std::string::npos);

    // 2. Multitouch touchscreen capability
    EXPECT_NE(content.find("android.hardware.touchscreen.multitouch"), std::string::npos);

    // 3. NativeActivity or GameActivity entry point configuration
    EXPECT_NE(content.find("android.app.NativeActivity"), std::string::npos);
    EXPECT_NE(content.find("android.app.lib_name"), std::string::npos);
    EXPECT_NE(content.find("android.intent.action.MAIN"), std::string::npos);
    EXPECT_NE(content.find("android.intent.category.LAUNCHER"), std::string::npos);
}

TEST(MobilePackagingGoogleTest, AndroidBuildGradleConfigurationVerification) {
    std::filesystem::path gradle_path = resolve_platform_path("platforms/android/app/build.gradle.kts");
    ASSERT_TRUE(std::filesystem::exists(gradle_path)) << "build.gradle.kts must exist in platforms/android/app";

    std::ifstream f(gradle_path);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    // Minimum SDK 26 for modern Vulkan support, C++20 compiler flags, NDK arm64
    EXPECT_NE(content.find("minSdk = 26"), std::string::npos);
    EXPECT_NE(content.find("arm64-v8a"), std::string::npos);
    EXPECT_NE(content.find("-std=c++20"), std::string::npos);
    EXPECT_NE(content.find("c++_shared"), std::string::npos);
}

// ============================================================================
// iOS Application Bundle & Info.plist Tests
// ============================================================================

TEST(MobilePackagingGoogleTest, IOSInfoPlistRequirementsVerification) {
    std::filesystem::path plist_path = resolve_platform_path("platforms/ios/Info.plist");
    ASSERT_TRUE(std::filesystem::exists(plist_path)) << "Info.plist must exist in platforms/ios";

    std::ifstream f(plist_path);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    // 1. Metal capability requirement
    EXPECT_NE(content.find("<string>metal</string>"), std::string::npos);

    // 2. 64-bit ARM architecture
    EXPECT_NE(content.find("<string>arm64</string>"), std::string::npos);

    // 3. Bundle identifier and executable
    EXPECT_NE(content.find("dev.pingdart.sharedlogin"), std::string::npos);
    EXPECT_NE(content.find("<key>LSRequiresIPhoneOS</key>"), std::string::npos);

    // 4. Orientation declarations
    EXPECT_NE(content.find("UIInterfaceOrientationPortrait"), std::string::npos);
    EXPECT_NE(content.find("UIInterfaceOrientationLandscapeLeft"), std::string::npos);
}

// ============================================================================
// Mobile Viewport & Safe Area Responsiveness Tests
// ============================================================================

TEST(MobilePackagingGoogleTest, ViewportAndSafeAreaResponsiveness) {
    SharedLoginApp app;

    // 1. Standard iPhone Portrait (393 x 852) with Dynamic Island (59pt top, 34pt bottom)
    app.set_screen_geometry(393.0f, 852.0f, {59.0f, 34.0f, 0.0f, 0.0f});
    EXPECT_FLOAT_EQ(app.width(), 393.0f);
    EXPECT_FLOAT_EQ(app.height(), 852.0f);
    EXPECT_FLOAT_EQ(app.safe_area_insets().top, 59.0f);
    EXPECT_FLOAT_EQ(app.safe_area_insets().bottom, 34.0f);

    // 2. iPhone Landscape (852 x 393) with Notch at Left (59pt left, 21pt bottom)
    app.set_screen_geometry(852.0f, 393.0f, {0.0f, 21.0f, 59.0f, 0.0f});
    EXPECT_FLOAT_EQ(app.width(), 852.0f);
    EXPECT_FLOAT_EQ(app.height(), 393.0f);
    EXPECT_FLOAT_EQ(app.safe_area_insets().left, 59.0f);
    EXPECT_FLOAT_EQ(app.safe_area_insets().bottom, 21.0f);

    // 3. High-Density Android Phone (1080 x 2400) with status bar and nav bar
    app.set_screen_geometry(1080.0f, 2400.0f, {96.0f, 48.0f, 0.0f, 0.0f});
    EXPECT_FLOAT_EQ(app.width(), 1080.0f);
    EXPECT_FLOAT_EQ(app.height(), 2400.0f);
    EXPECT_FLOAT_EQ(app.safe_area_insets().top, 96.0f);
    EXPECT_FLOAT_EQ(app.safe_area_insets().bottom, 48.0f);

    // 4. Tablet Layout (1024 x 1366)
    app.set_screen_geometry(1024.0f, 1366.0f, {24.0f, 20.0f, 0.0f, 0.0f});
    EXPECT_FLOAT_EQ(app.width(), 1024.0f);
    EXPECT_FLOAT_EQ(app.height(), 1366.0f);
}
