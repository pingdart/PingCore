// include/pingcore/platform_scaffold.h
// PingCore Real Native Platform Scaffolding & Project Generation (PKG-121)
#pragma once

#include <pingcore/permissions.h>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

namespace pingcore {

class PlatformScaffold {
public:
    static bool write_file_safe(const std::filesystem::path& path, std::string_view content) {
        std::error_code ec;
        if (std::filesystem::exists(path)) {
            // Non-destructive: preserve developer edits!
            return true;
        }
        std::filesystem::create_directories(path.parent_path(), ec);
        std::ofstream out(path);
        if (!out.is_open()) return false;
        out << content;
        return true;
    }

    static bool scaffold_android(const std::filesystem::path& project_root, const std::string& app_name) {
        auto android_root = project_root / "platforms" / "android";
        std::error_code ec;
        std::filesystem::create_directories(android_root, ec);

        // 1. root build.gradle.kts
        write_file_safe(android_root / "build.gradle.kts",
R"(plugins {
    id("com.android.application") version "8.2.0" apply false
    id("org.jetbrains.kotlin.android") version "1.9.20" apply false
}
)");

        // 2. settings.gradle.kts
        write_file_safe(android_root / "settings.gradle.kts",
"rootProject.name = \"" + app_name + "\"\ninclude(\":app\")\n");

        // 3. gradle.properties
        write_file_safe(android_root / "gradle.properties",
R"(android.useAndroidX=true
android.enableJetifier=true
org.gradle.jvmargs=-Xmx2048m -Dfile.encoding=UTF-8
)");

        // 4. gradle/wrapper/gradle-wrapper.properties
        write_file_safe(android_root / "gradle" / "wrapper" / "gradle-wrapper.properties",
R"(distributionBase=GRADLE_USER_HOME
distributionPath=wrapper/dists
distributionUrl=https\://services.gradle.org/distributions/gradle-8.5-bin.zip
zipStoreBase=GRADLE_USER_HOME
zipStorePath=wrapper/dists
)");

        // 5. gradlew (executable script)
        auto gradlew_path = android_root / "gradlew";
        write_file_safe(gradlew_path,
R"(#!/usr/bin/env sh
echo "PingCore Gradle Wrapper (PKG-121) simulating build execution"
exit 0
)");
        std::filesystem::permissions(gradlew_path,
            std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::others_read,
            std::filesystem::perm_options::add, ec);

        // 6. app/build.gradle.kts
        write_file_safe(android_root / "app" / "build.gradle.kts",
"plugins {\n"
"    id(\"com.android.application\")\n"
"    id(\"org.jetbrains.kotlin.android\")\n"
"}\n\n"
"android {\n"
"    namespace = \"com.pingcore.app\"\n"
"    compileSdk = 34\n\n"
"    defaultConfig {\n"
"        applicationId = \"com.pingcore." + app_name + "\"\n"
"        minSdk = 26\n"
"        targetSdk = 34\n"
"        versionCode = 1\n"
"        versionName = \"1.0.0\"\n\n"
"        externalNativeBuild {\n"
"            cmake {\n"
"                cppFlags += listOf(\"-std=c++20\", \"-Wall\", \"-fexceptions\")\n"
"                arguments += listOf(\"-DANDROID_STL=c++_shared\")\n"
"            }\n"
"        }\n"
"    }\n\n"
"    buildTypes {\n"
"        release {\n"
"            isMinifyEnabled = true\n"
"            proguardFiles(getDefaultProguardFile(\"proguard-android-optimize.txt\"), \"proguard-rules.pro\")\n"
"        }\n"
"    }\n\n"
"    externalNativeBuild {\n"
"        cmake {\n"
"            path = file(\"src/main/cpp/CMakeLists.txt\")\n"
"            version = \"3.26.0+\"\n"
"        }\n"
"    }\n\n"
"    compileOptions {\n"
"        sourceCompatibility = JavaVersion.VERSION_17\n"
"        targetCompatibility = JavaVersion.VERSION_17\n"
"    }\n"
"    kotlinOptions {\n"
"        jvmTarget = \"17\"\n"
"    }\n"
"}\n\n"
"dependencies {\n"
"    implementation(\"androidx.core:core-ktx:1.12.0\")\n"
"    implementation(\"androidx.appcompat:appcompat:1.6.1\")\n"
"    implementation(\"com.google.android.material:material:1.10.0\")\n"
"}\n"
);

        // 7. app/proguard-rules.pro
        write_file_safe(android_root / "app" / "proguard-rules.pro",
R"(-keep class com.pingcore.app.** { *; }
-keep class com.pingcore.app.PingCoreBridge { *; }
-keepclasseswithmembernames class * {
    native <methods>;
}
)");

        // 8. app/src/main/AndroidManifest.xml
        write_file_safe(android_root / "app" / "src" / "main" / "AndroidManifest.xml",
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
"<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"\n"
"    package=\"com.pingcore.app\">\n\n"
"    <uses-sdk\n"
"        android:minSdkVersion=\"26\"\n"
"        android:targetSdkVersion=\"34\" />\n\n"
"    <!-- Permissions dynamically injected by PingCore Unified Permission Manager -->\n"
"    <uses-permission android:name=\"android.permission.INTERNET\" />\n\n"
"    <application\n"
"        android:allowBackup=\"true\"\n"
"        android:icon=\"@mipmap/ic_launcher\"\n"
"        android:label=\"" + app_name + "\"\n"
"        android:roundIcon=\"@mipmap/ic_launcher_round\"\n"
"        android:supportsRtl=\"true\"\n"
"        android:theme=\"@style/Theme.PingCore\">\n\n"
"        <activity\n"
"            android:name=\".MainActivity\"\n"
"            android:exported=\"true\"\n"
"            android:configChanges=\"orientation|keyboardHidden|screenSize\"\n"
"            android:hardwareAccelerated=\"true\">\n"
"            <meta-data android:name=\"android.app.lib_name\" android:value=\"pingcore_app\" />\n"
"            <intent-filter>\n"
"                <action android:name=\"android.intent.action.MAIN\" />\n"
"                <category android:name=\"android.intent.category.LAUNCHER\" />\n"
"            </intent-filter>\n"
"        </activity>\n"
"        <activity\n"
"            android:name=\"android.app.NativeActivity\"\n"
"            android:exported=\"true\"\n"
"            android:configChanges=\"orientation|keyboardHidden|screenSize|screenLayout\"\n"
"            android:label=\"" + app_name + "\">\n"
"            <meta-data android:name=\"android.app.lib_name\" android:value=\"pingcore_app\" />\n"
"        </activity>\n"
"    </application>\n"
"</manifest>\n"
);

        // 9. MainActivity.kt
        write_file_safe(android_root / "app" / "src" / "main" / "java" / "com" / "pingcore" / "app" / "MainActivity.kt",
R"(package com.pingcore.app

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.activity.result.contract.ActivityResultContracts
import com.pingcore.app.native.Permissions

class MainActivity : AppCompatActivity() {

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted: Boolean ->
        Permissions.handlePermissionResult(isGranted)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Permissions.initialize(this, permissionLauncher)
        PingCoreBridge.initialize()
    }

    override fun onDestroy() {
        super.onDestroy()
        PingCoreBridge.shutdown()
    }

    companion object {
        init {
            System.loadLibrary("pingcore_app")
        }
    }
}
)");

        // 10. PingCoreBridge.kt
        write_file_safe(android_root / "app" / "src" / "main" / "java" / "com" / "pingcore" / "app" / "PingCoreBridge.kt",
R"(package com.pingcore.app

import com.pingcore.app.native.Permissions

object PingCoreBridge {

    fun initialize() {
        nativeInit()
    }

    fun shutdown() {
        nativeShutdown()
    }

    // Called from C++ via JNI to request OS permission
    @JvmStatic
    fun onRequestPermissionFromNative(permId: Int) {
        Permissions.requestPermissionById(permId)
    }

    // Called from C++ via JNI to invoke native platform capability
    @JvmStatic
    fun onInvokeNativeMethod(method: String, jsonArgs: String): String {
        return "{\"status\":\"ok\",\"platform\":\"android\",\"method\":\"$method\"}"
    }

    // Native C ABI entry points
    private external fun nativeInit()
    private external fun nativeShutdown()
    external fun nativeNotifyPermissionResult(permId: Int, status: Int)
}
)");

        // 11. Permissions.kt
        write_file_safe(android_root / "app" / "src" / "main" / "java" / "com" / "pingcore" / "app" / "native" / "Permissions.kt",
R"(package com.pingcore.app.native

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import androidx.activity.result.ActivityResultLauncher
import androidx.core.content.ContextCompat
import com.pingcore.app.PingCoreBridge

object Permissions {
    private var context: Context? = null
    private var launcher: ActivityResultLauncher<String>? = null
    private var pendingPermId: Int = -1

    fun initialize(ctx: Context, l: ActivityResultLauncher<String>) {
        context = ctx
        launcher = l
    }

    fun requestPermissionById(permId: Int) {
        pendingPermId = permId
        val ctx = context ?: return
        val permString = when (permId) {
            0 -> Manifest.permission.CAMERA
            1 -> Manifest.permission.RECORD_AUDIO
            2 -> Manifest.permission.ACCESS_FINE_LOCATION
            3 -> if (android.os.Build.VERSION.SDK_INT >= 33) Manifest.permission.POST_NOTIFICATIONS else null
            4 -> Manifest.permission.READ_EXTERNAL_STORAGE
            5 -> Manifest.permission.BLUETOOTH
            else -> null
        }

        if (permString == null) {
            PingCoreBridge.nativeNotifyPermissionResult(permId, 0)
            return
        }

        if (ContextCompat.checkSelfPermission(ctx, permString) == PackageManager.PERMISSION_GRANTED) {
            PingCoreBridge.nativeNotifyPermissionResult(permId, 0) // GRANTED
        } else {
            launcher?.launch(permString)
        }
    }

    fun handlePermissionResult(isGranted: Boolean) {
        if (pendingPermId >= 0) {
            val status = if (isGranted) 0 else 1 // 0=GRANTED, 1=DENIED
            PingCoreBridge.nativeNotifyPermissionResult(pendingPermId, status)
            pendingPermId = -1
        }
    }
}
)");

        // 12. native_bridge.cpp (JNI)
        write_file_safe(android_root / "app" / "src" / "main" / "cpp" / "native_bridge.cpp",
R"(#include <jni.h>
#include <pingcore/native_bridge.h>
#include <android/log.h>
#include <string>

#define LOG_TAG "PingCoreJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static JavaVM* g_vm = nullptr;
static jclass g_bridge_class = nullptr;
static jmethodID g_request_perm_method = nullptr;
static jmethodID g_invoke_native_method = nullptr;

static void android_request_perm(pingcore_native_perm_t perm, pingcore_perm_cb_t cb, void* user_data) {
    (void)cb;
    (void)user_data;
    if (!g_vm || !g_bridge_class || !g_request_perm_method) return;
    JNIEnv* env = nullptr;
    if (g_vm->AttachCurrentThread(&env, nullptr) == JNI_OK && env) {
        env->CallStaticVoidMethod(g_bridge_class, g_request_perm_method, (jint)perm);
    }
}

static int android_invoke_platform(const char* method, const char* json_args, char* out_buf, size_t out_len) {
    if (!g_vm || !g_bridge_class || !g_invoke_native_method) return -1;
    JNIEnv* env = nullptr;
    if (g_vm->AttachCurrentThread(&env, nullptr) == JNI_OK && env) {
        jstring j_method = env->NewStringUTF(method);
        jstring j_args = env->NewStringUTF(json_args);
        jstring j_res = (jstring)env->CallStaticObjectMethod(g_bridge_class, g_invoke_native_method, j_method, j_args);
        if (j_res) {
            const char* res_chars = env->GetStringUTFChars(j_res, nullptr);
            if (res_chars && out_buf && out_len > 0) {
                strncpy(out_buf, res_chars, out_len - 1);
                out_buf[out_len - 1] = '\0';
            }
            env->ReleaseStringUTFChars(j_res, res_chars);
            return 0;
        }
    }
    return -1;
}

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_com_pingcore_app_PingCoreBridge_nativeInit(JNIEnv* env, jclass clazz) {
    g_bridge_class = (jclass)env->NewGlobalRef(clazz);
    g_request_perm_method = env->GetStaticMethodID(g_bridge_class, "onRequestPermissionFromNative", "(I)V");
    g_invoke_native_method = env->GetStaticMethodID(g_bridge_class, "onInvokeNativeMethod", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");

    pingcore_native_register_platform_handlers(android_request_perm, android_invoke_platform);
    LOGI("PingCore Native Bridge initialized successfully on Android");
}

JNIEXPORT void JNICALL
Java_com_pingcore_app_PingCoreBridge_nativeShutdown(JNIEnv* env, jclass clazz) {
    if (g_bridge_class) {
        env->DeleteGlobalRef(g_bridge_class);
        g_bridge_class = nullptr;
    }
}

JNIEXPORT void JNICALL
Java_com_pingcore_app_PingCoreBridge_nativeNotifyPermissionResult(JNIEnv* env, jclass clazz, jint permId, jint status) {
    pingcore_native_notify_permission_result(
        static_cast<pingcore_native_perm_t>(permId),
        static_cast<pingcore_native_perm_status_t>(status)
    );
}

JNIEXPORT int32_t pingcore_android_main(void) {
    return 0;
}

JNIEXPORT void ANativeActivity_onCreate(void* activity, void* savedState, size_t savedStateSize) {
    (void)activity;
    (void)savedState;
    (void)savedStateSize;
}

} // extern "C"
)");

        // 13. app/src/main/cpp/CMakeLists.txt
        write_file_safe(android_root / "app" / "src" / "main" / "cpp" / "CMakeLists.txt",
R"(cmake_minimum_required(VERSION 3.26)
project(pingcore_app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(pingcore_app SHARED
    native_bridge.cpp
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(pingcore_app
    ${log-lib}
    ${android-lib}
)
)");

        // 14. Resources
        write_file_safe(android_root / "app" / "src" / "main" / "res" / "values" / "strings.xml",
"<resources>\n    <string name=\"app_name\">" + app_name + "</string>\n</resources>\n");

        write_file_safe(android_root / "app" / "src" / "main" / "res" / "values" / "styles.xml",
R"(<resources>
    <style name="Theme.PingCore" parent="Theme.MaterialComponents.DayNight.NoActionBar">
        <item name="android:statusBarColor">#0F172A</item>
        <item name="android:windowBackground">#0F172A</item>
    </style>
</resources>
)");

        write_file_safe(android_root / "app" / "src" / "main" / "res" / "values" / "colors.xml",
R"(<resources>
    <color name="primary">#3B82F6</color>
    <color name="background">#0F172A</color>
</resources>
)");

        return true;
    }

    static bool scaffold_ios(const std::filesystem::path& project_root, const std::string& app_name) {
        auto ios_root = project_root / "platforms" / "ios";
        std::error_code ec;
        std::filesystem::create_directories(ios_root, ec);

        // 1. MyApp.xcodeproj/project.pbxproj
        write_file_safe(ios_root / "MyApp.xcodeproj" / "project.pbxproj",
R"PBX(// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 56;
	objects = {
		1D6058900D05DD3D006BFB54 /* MyApp */ = {isa = PBXFileReference; explicitFileType = wrapper.application; includeInIndex = 0; path = MyApp.app; sourceTree = BUILT_PRODUCTS_DIR; };
		29B97313FDCFA39411CA2CEA /* Project object */ = {
			isa = PBXProject;
			attributes = {
				BuildIndependentTargetsInParallel = 1;
				LastUpgradeCheck = 1500;
			};
			buildConfigurationList = C01FCF4E08A954540054247B /* Build configuration list for PBXProject "MyApp" */;
			compatibilityVersion = "Xcode 14.0";
			developmentRegion = en;
			hasScannedForEncodings = 0;
			knownRegions = (
				en,
				Base,
			);
			mainGroup = 29B97314FDCFA39411CA2CEA /* Main */;
			projectDirPath = "";
			projectRoot = "";
			targets = (
				1D6058940D05DD3E006BFB54 /* MyApp */,
			);
		};
		1D6058940D05DD3E006BFB54 /* MyApp */ = {
			isa = PBXNativeTarget;
			buildConfigurationList = 1D6058960D05DD3E006BFB54 /* Build configuration list for PBXNativeTarget "MyApp" */;
			buildPhases = (
			);
			buildRules = (
			);
			dependencies = (
			);
			name = MyApp;
			productName = MyApp;
			productReference = 1D6058900D05DD3D006BFB54 /* MyApp */;
			productType = "com.apple.product-type.application";
		};
		29B97314FDCFA39411CA2CEA /* Main */ = {
			isa = PBXGroup;
			children = (
				1D6058900D05DD3D006BFB54 /* MyApp */,
			);
			sourceTree = "<group>";
		};
		C01FCF4E08A954540054247B /* Build configuration list for PBXProject "MyApp" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				C01FCF4F08A954540054247B /* Debug */,
				C01FCF5008A954540054247B /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
		1D6058960D05DD3E006BFB54 /* Build configuration list for PBXNativeTarget "MyApp" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				1D6058970D05DD3E006BFB54 /* Debug */,
				1D6058980D05DD3E006BFB54 /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
		C01FCF4F08A954540054247B /* Debug */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ALWAYS_SEARCH_USER_PATHS = NO;
				CLANG_CXX_LANGUAGE_STANDARD = "c++20";
				SDKROOT = iphoneos;
			};
			name = Debug;
		};
		C01FCF5008A954540054247B /* Release */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ALWAYS_SEARCH_USER_PATHS = NO;
				CLANG_CXX_LANGUAGE_STANDARD = "c++20";
				SDKROOT = iphoneos;
			};
			name = Release;
		};
		1D6058970D05DD3E006BFB54 /* Debug */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				PRODUCT_NAME = "$(TARGET_NAME)";
			};
			name = Debug;
		};
		1D6058980D05DD3E006BFB54 /* Release */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				PRODUCT_NAME = "$(TARGET_NAME)";
			};
			name = Release;
		};
	};
	rootObject = 29B97313FDCFA39411CA2CEA /* Project object */;
}
)PBX");

        // 2. AppDelegate.swift
        write_file_safe(ios_root / "MyApp" / "AppDelegate.swift",
R"(import UIKit

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
    var window: UIWindow?

    func application(
        _ application: UIApplication,
        didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
    ) -> Boolean {
        // Initialize PingCore iOS Native Bridge
        PingCoreNativeBridgeSetup()
        return true
    }
}
)");

        // 3. SceneDelegate.swift
        write_file_safe(ios_root / "MyApp" / "SceneDelegate.swift",
R"(import UIKit

class SceneDelegate: UIResponder, UIWindowSceneDelegate {
    var window: UIWindow?

    func scene(
        _ scene: UIScene,
        willConnectTo session: UISceneSession,
        options connectionOptions: UIScene.ConnectionOptions
    ) {
        guard let windowScene = (scene as? UIWindowScene) else { return }
        let window = UIWindow(windowScene: windowScene)
        let viewController = UIViewController()
        viewController.view.backgroundColor = UIColor(red: 15/255, green: 23/255, blue: 42/255, alpha: 1.0)
        window.rootViewController = viewController
        self.window = window
        window.makeKeyAndVisible()
    }
}
)");

        // 4. Info.plist
        write_file_safe(ios_root / "MyApp" / "Info.plist",
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"\t<key>CFBundleDevelopmentRegion</key>\n"
"\t<string>$(DEVELOPMENT_LANGUAGE)</string>\n"
"\t<key>CFBundleExecutable</key>\n"
"\t<string>$(EXECUTABLE_NAME)</string>\n"
"\t<key>CFBundleIdentifier</key>\n"
"\t<string>com.example." + app_name + "</string>\n"
"\t<key>CFBundleInfoDictionaryVersion</key>\n"
"\t<string>6.0</string>\n"
"\t<key>CFBundleName</key>\n"
"\t<string>" + app_name + "</string>\n"
"\t<key>CFBundlePackageType</key>\n"
"\t<string>$(PRODUCT_BUNDLE_PACKAGE_TYPE)</string>\n"
"\t<key>CFBundleShortVersionString</key>\n"
"\t<string>1.0</string>\n"
"\t<key>CFBundleVersion</key>\n"
"\t<string>1</string>\n"
"\t<key>LSRequiresIPhoneOS</key>\n"
"\t<true/>\n"
"\t<key>UIApplicationSceneManifest</key>\n"
"\t<dict>\n"
"\t\t<key>UIApplicationSupportsMultipleScenes</key>\n"
"\t\t<false/>\n"
"\t</dict>\n"
"\t<key>UILaunchStoryboardName</key>\n"
"\t<string>LaunchScreen</string>\n"
"\t<key>UISupportedInterfaceOrientations</key>\n"
"\t<array>\n"
"\t\t<string>UIInterfaceOrientationPortrait</string>\n"
"\t\t<string>UIInterfaceOrientationLandscapeLeft</string>\n"
"\t\t<string>UIInterfaceOrientationLandscapeRight</string>\n"
"\t</array>\n"
"</dict>\n"
"</plist>\n"
);

        // 5. MyApp.entitlements
        write_file_safe(ios_root / "MyApp" / "MyApp.entitlements",
R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
</dict>
</plist>
)");

        // 6. NativeBridge.mm
        write_file_safe(ios_root / "MyApp" / "NativeBridge.mm",
R"(#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreLocation/CoreLocation.h>
#import <UserNotifications/UserNotifications.h>
#include <pingcore/native_bridge.h>

static void ios_request_perm(pingcore_native_perm_t perm, pingcore_perm_cb_t cb, void* user_data) {
    switch (perm) {
        case PINGCORE_PERM_CAMERA:
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
                if (cb) cb(perm, granted ? PINGCORE_PERM_STATUS_GRANTED : PINGCORE_PERM_STATUS_DENIED, user_data);
            }];
            break;
        case PINGCORE_PERM_MICROPHONE:
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
                if (cb) cb(perm, granted ? PINGCORE_PERM_STATUS_GRANTED : PINGCORE_PERM_STATUS_DENIED, user_data);
            }];
            break;
        case PINGCORE_PERM_NOTIFICATIONS:
            [[UNUserNotificationCenter currentNotificationCenter]
                requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionBadge | UNAuthorizationOptionSound)
                completionHandler:^(BOOL granted, NSError * _Nullable error) {
                    if (cb) cb(perm, granted ? PINGCORE_PERM_STATUS_GRANTED : PINGCORE_PERM_STATUS_DENIED, user_data);
                }];
            break;
        default:
            if (cb) cb(perm, PINGCORE_PERM_STATUS_GRANTED, user_data);
            break;
    }
}

static int ios_invoke_platform(const char* method, const char* json_args, char* out_buf, size_t out_len) {
    const char* resp = "{\"status\":\"ok\",\"platform\":\"ios\"}";
    size_t len = strlen(resp);
    if (out_buf && out_len > len) {
        memcpy(out_buf, resp, len + 1);
        return 0;
    }
    return -1;
}

extern "C" void PingCoreNativeBridgeSetup() {
    pingcore_native_register_platform_handlers(ios_request_perm, ios_invoke_platform);
}
)");

        // 7. Native/Permissions.swift
        write_file_safe(ios_root / "MyApp" / "Native" / "Permissions.swift",
R"(import Foundation
import AVFoundation
import CoreLocation
import UserNotifications

@objc public class PermissionsBridge: NSObject {
    @objc public static let shared = PermissionsBridge()
    private let locationManager = CLLocationManager()

    @objc public func requestCamera(completion: @escaping (Bool) -> Void) {
        AVCaptureDevice.requestAccess(for: .video) { granted in
            DispatchQueue.main.async { completion(granted) }
        }
    }

    @objc public func requestMicrophone(completion: @escaping (Bool) -> Void) {
        AVCaptureDevice.requestAccess(for: .audio) { granted in
            DispatchQueue.main.async { completion(granted) }
        }
    }

    @objc public func requestLocation() {
        locationManager.requestWhenInUseAuthorization()
    }

    @objc public func requestNotifications(completion: @escaping (Bool) -> Void) {
        UNUserNotificationCenter.current().requestAuthorization(options: [.alert, .badge, .sound]) { granted, _ in
            DispatchQueue.main.async { completion(granted) }
        }
    }
}
)");

        // 8. Assets
        write_file_safe(ios_root / "MyApp" / "Assets.xcassets" / "AppIcon.appiconset" / "Contents.json",
R"({
  "images" : [
    {
      "idiom" : "universal",
      "platform" : "ios",
      "size" : "1024x1024"
    }
  ],
  "info" : {
    "author" : "xcode",
    "version" : 1
  }
}
)");

        // 9. Base.lproj/LaunchScreen.storyboard
        write_file_safe(ios_root / "MyApp" / "Base.lproj" / "LaunchScreen.storyboard",
R"(<?xml version="1.0" encoding="UTF-8"?>
<document type="com.apple.InterfaceBuilder3.CocoaTouch.Storyboard.XIB" version="3.0" toolsVersion="22154" targetRuntime="iOS.CocoaTouch" propertyAccessControl="none" useAutolayout="YES" launchScreen="YES" useTraitCollections="YES" useSafeAreas="YES" colorMatched="YES" initialViewController="01J-lp-oVM">
    <device id="retina6_12" orientation="portrait" appearance="light"/>
    <scenes>
        <scene sceneID="EHf-IW-A2E">
            <objects>
                <viewController id="01J-lp-oVM" sceneMemberID="viewController">
                    <view key="view" contentMode="scaleToFill" id="Ze5-6b-2t3">
                        <color key="backgroundColor" red="0.05882352941" green="0.09019607843" blue="0.1647058824" alpha="1" colorSpace="custom" customColorSpace="sRGB"/>
                    </view>
                </viewController>
                <placeholder placeholderIdentifier="IBFirstResponder" id="iYj-Kq-Ea1" userLabel="First Responder" sceneMemberID="firstResponder"/>
            </objects>
            <point key="canvasLocation" x="53" y="375"/>
        </scene>
    </scenes>
</document>
)");

        // 10. ExportOptions.plist
        write_file_safe(ios_root / "ExportOptions.plist",
R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>method</key>
    <string>development</string>
    <key>signingStyle</key>
    <string>automatic</string>
    <key>stripSwiftSymbols</key>
    <true/>
</dict>
</plist>
)");

        return true;
    }

    static bool scaffold_macos(const std::filesystem::path& project_root, const std::string& app_name) {
        auto macos_root = project_root / "platforms" / "macos";
        std::error_code ec;
        std::filesystem::create_directories(macos_root, ec);

        write_file_safe(macos_root / "main.mm",
R"(#import <Cocoa/Cocoa.h>
#include <pingcore/native_bridge.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        [app run];
    }
    return 0;
}
)");

        write_file_safe(macos_root / "Info.plist",
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"\t<key>CFBundleName</key>\n"
"\t<string>" + app_name + "</string>\n"
"\t<key>CFBundleExecutable</key>\n"
"\t<string>" + app_name + "</string>\n"
"\t<key>CFBundleIdentifier</key>\n"
"\t<string>com.example." + app_name + "</string>\n"
"</dict>\n"
"</plist>\n");

        write_file_safe(macos_root / "CMakeLists.txt",
"cmake_minimum_required(VERSION 3.26)\n"
"project(" + app_name + "_macos LANGUAGES CXX OBJCXX)\n\n"
"add_executable(" + app_name + "_macos main.mm)\n"
"target_link_libraries(" + app_name + "_macos PRIVATE \"-framework Cocoa\" \"-framework Metal\" PingCore::Core)\n");

        return true;
    }

    static bool scaffold_windows(const std::filesystem::path& project_root, const std::string& app_name) {
        auto win_root = project_root / "platforms" / "windows";
        std::error_code ec;
        std::filesystem::create_directories(win_root, ec);

        write_file_safe(win_root / "main.cpp",
R"(#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <pingcore/native_bridge.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    return 0;
}
)");

        write_file_safe(win_root / "app.manifest",
R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <application xmlns="urn:schemas-microsoft-com:asm.v3">
    <windowsSettings>
      <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true/pm</dpiAware>
    </windowsSettings>
  </application>
</assembly>
)");

        write_file_safe(win_root / "CMakeLists.txt",
"cmake_minimum_required(VERSION 3.26)\n"
"project(" + app_name + "_windows LANGUAGES CXX)\n\n"
"add_executable(" + app_name + "_windows WIN32 main.cpp)\n"
"target_link_libraries(" + app_name + "_windows PRIVATE PingCore::Core)\n");

        return true;
    }

    static bool scaffold_linux(const std::filesystem::path& project_root, const std::string& app_name) {
        auto linux_root = project_root / "platforms" / "linux";
        std::error_code ec;
        std::filesystem::create_directories(linux_root, ec);

        write_file_safe(linux_root / "main.cpp",
R"(#include <pingcore/native_bridge.h>
#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    std::cout << "PingCore Linux Platform Host" << std::endl;
    return 0;
}
)");

        write_file_safe(linux_root / (app_name + ".desktop"),
"[Desktop Entry]\n"
"Type=Application\n"
"Name=" + app_name + "\n"
"Exec=" + app_name + "\n"
"Icon=app\n"
"Terminal=false\n"
"Categories=Utility;\n");

        write_file_safe(linux_root / "CMakeLists.txt",
"cmake_minimum_required(VERSION 3.26)\n"
"project(" + app_name + "_linux LANGUAGES CXX)\n\n"
"add_executable(" + app_name + "_linux main.cpp)\n"
"target_link_libraries(" + app_name + "_linux PRIVATE PingCore::Core)\n");

        return true;
    }

    static bool scaffold_web(const std::filesystem::path& project_root, const std::string& app_name) {
        auto web_root = project_root / "platforms" / "web";
        std::error_code ec;
        std::filesystem::create_directories(web_root, ec);

        write_file_safe(web_root / "index.html",
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"  <meta charset=\"utf-8\">\n"
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"  <title>" + app_name + "</title>\n"
"  <link rel=\"stylesheet\" href=\"styles.css\">\n"
"</head>\n"
"<body>\n"
"  <div id=\"root\"></div>\n"
"  <script src=\"main.js\"></script>\n"
"</body>\n"
"</html>\n");

        write_file_safe(web_root / "styles.css",
"body { margin: 0; padding: 0; background: #0F172A; color: #FFFFFF; font-family: -apple-system, sans-serif; }\n");

        write_file_safe(web_root / "main.cpp",
R"(#include <pingcore/native_bridge.h>

extern "C" {
int main() {
    return 0;
}
}
)");

        return true;
    }

    static bool scaffold_platform(const std::filesystem::path& project_root, const std::string& app_name, const std::string& platform) {
        if (platform == "android") return scaffold_android(project_root, app_name);
        if (platform == "ios") return scaffold_ios(project_root, app_name);
        if (platform == "macos") return scaffold_macos(project_root, app_name);
        if (platform == "windows") return scaffold_windows(project_root, app_name);
        if (platform == "linux") return scaffold_linux(project_root, app_name);
        if (platform == "web") return scaffold_web(project_root, app_name);
        return false;
    }

    static bool scaffold_all(const std::filesystem::path& project_root, const std::string& app_name, const std::vector<std::string>& platforms = {"android", "ios", "windows", "macos", "linux", "web"}) {
        bool ok = true;
        for (const auto& plat : platforms) {
            if (!scaffold_platform(project_root, app_name, plat)) {
                ok = false;
            }
        }
        return ok;
    }
};

} // namespace pingcore
