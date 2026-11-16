#include <gtest/gtest.h>
#include <pingdart/cli/toolchain.h>
#include <pingdart/cli/builder.h>
#include <pingdart/cli/font_bundler.h>
#include <pingdart/cli/packager.h>
#include <pingdart/cli/verifier.h>
#include <filesystem>
#include <fstream>

using namespace pingdart;
using namespace pingdart::cli;

TEST(PackagingTest, DoctorIdentifiesCompilerAndDependencies) {
    Doctor doctor;
    DoctorReport report = doctor.run_diagnostics();

    EXPECT_FALSE(report.diagnostics.empty());
    EXPECT_GE(report.diagnostics.size(), 6u);

    bool compiler_found = false;
    bool cmake_found = false;
    for (const auto& d : report.diagnostics) {
        if (d.name == "C++20 Compiler" && d.status == ToolStatus::Available) {
            compiler_found = true;
        }
        if (d.name == "CMake Build System" && d.status == ToolStatus::Available) {
            cmake_found = true;
        }
    }

    EXPECT_TRUE(compiler_found);
    EXPECT_TRUE(cmake_found);

    std::string formatted = doctor.format_report(report);
    EXPECT_FALSE(formatted.empty());
    EXPECT_NE(formatted.find("PingDart Doctor"), std::string::npos);
}

TEST(PackagingTest, BuilderGeneratesValidCMakeArguments) {
    Builder builder;

    BuildOptions opts;
    opts.target = TargetPlatform::MacOSArm64;
    opts.mode = BuildMode::Release;
    opts.source_dir = "/tmp/pingdart_src";
    opts.build_dir = "/tmp/pingdart_build";
    opts.target_app = "desktop_login";

    auto conf_args = builder.generate_cmake_configure_args(opts);
    std::string joined_conf;
    for (const auto& a : conf_args) joined_conf += a + " ";

    EXPECT_NE(joined_conf.find("-S /tmp/pingdart_src"), std::string::npos);
    EXPECT_NE(joined_conf.find("-B /tmp/pingdart_build"), std::string::npos);
    EXPECT_NE(joined_conf.find("-G Ninja"), std::string::npos);
    EXPECT_NE(joined_conf.find("-DCMAKE_BUILD_TYPE=Release"), std::string::npos);
    EXPECT_NE(joined_conf.find("-DCMAKE_OSX_ARCHITECTURES=arm64"), std::string::npos);

    auto build_args = builder.generate_cmake_build_args(opts);
    std::string joined_build;
    for (const auto& a : build_args) joined_build += a + " ";

    EXPECT_NE(joined_build.find("--build /tmp/pingdart_build"), std::string::npos);
    EXPECT_NE(joined_build.find("--target desktop_login"), std::string::npos);
}

TEST(PackagingTest, MacOSAppBundleGenerationAndPlistValidation) {
    std::filesystem::path test_out = "build/test_dist";
    std::error_code ec;
    std::filesystem::create_directories(test_out, ec);

    // Create a mock executable for test packaging
    std::filesystem::path dummy_bin = test_out / "dummy_app_binary";
    {
        std::ofstream out(dummy_bin, std::ios::binary);
        out << "#!/bin/sh\nexit 0\n";
    }

    PackageDescriptor desc;
    desc.app_name = "PingDartTestApp";
    desc.bundle_id = "dev.pingdart.testapp";
    desc.version = "1.2.3";
    desc.short_version = "1.2.3";
    desc.executable_path = dummy_bin.string();
    desc.output_directory = test_out.string();
    desc.target = TargetPlatform::MacOSArm64;
    desc.high_dpi_capable = true;

    Packager packager;
    auto res = packager.package_macos(desc);
    ASSERT_TRUE(res.has_value());

    const auto& pkg = res.value();
    EXPECT_EQ(pkg.format, PackageFormat::MacOSAppBundle);
    EXPECT_TRUE(std::filesystem::exists(pkg.package_root_path, ec));
    EXPECT_TRUE(std::filesystem::exists(pkg.executable_bundled_path, ec));
    EXPECT_TRUE(std::filesystem::exists(pkg.manifest_or_plist_path, ec));

    // Verify Info.plist contents
    std::ifstream plist_in(pkg.manifest_or_plist_path);
    std::string plist_content((std::istreambuf_iterator<char>(plist_in)), std::istreambuf_iterator<char>());

    EXPECT_NE(plist_content.find("<key>CFBundlePackageType</key>\n    <string>APPL</string>"), std::string::npos);
    EXPECT_NE(plist_content.find("<key>CFBundleExecutable</key>\n    <string>PingDartTestApp</string>"), std::string::npos);
    EXPECT_NE(plist_content.find("<key>CFBundleIdentifier</key>\n    <string>dev.pingdart.testapp</string>"), std::string::npos);
    EXPECT_NE(plist_content.find("<key>CFBundleVersion</key>\n    <string>1.2.3</string>"), std::string::npos);
    EXPECT_NE(plist_content.find("<key>NSHighResolutionCapable</key>\n    <true/>"), std::string::npos);
    EXPECT_NE(plist_content.find("<key>NSSupportsAutomaticGraphicsSwitching</key>\n    <true/>"), std::string::npos);

    // Verify PkgInfo
    std::filesystem::path pkginfo_path = std::filesystem::path(pkg.package_root_path) / "Contents" / "PkgInfo";
    EXPECT_TRUE(std::filesystem::exists(pkginfo_path, ec));
}

TEST(PackagingTest, WindowsManifestAndPackageLayout) {
    std::filesystem::path test_out = "build/test_dist_win";
    std::error_code ec;
    std::filesystem::create_directories(test_out, ec);

    std::filesystem::path dummy_bin = test_out / "dummy_win.exe";
    {
        std::ofstream out(dummy_bin, std::ios::binary);
        out << "MZ\0\0";
    }

    PackageDescriptor desc;
    desc.app_name = "WinPingDart";
    desc.bundle_id = "dev.pingdart.win";
    desc.version = "2.0.0";
    desc.executable_path = dummy_bin.string();
    desc.output_directory = test_out.string();
    desc.target = TargetPlatform::WindowsX64;

    Packager packager;
    auto res = packager.package_windows(desc);
    ASSERT_TRUE(res.has_value());

    const auto& pkg = res.value();
    EXPECT_EQ(pkg.format, PackageFormat::WindowsExecutablePackage);
    EXPECT_TRUE(std::filesystem::exists(pkg.manifest_or_plist_path, ec));

    std::ifstream mf_in(pkg.manifest_or_plist_path);
    std::string mf_content((std::istreambuf_iterator<char>(mf_in)), std::istreambuf_iterator<char>());

    EXPECT_NE(mf_content.find("<dpiAwareness xmlns=\"http://schemas.microsoft.com/SMI/2016/WindowsSettings\">PerMonitorV2, PerMonitor</dpiAwareness>"), std::string::npos);
    EXPECT_NE(mf_content.find("<supportedOS Id=\"{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}\"/>"), std::string::npos);
}

TEST(PackagingTest, LinuxAppDirStructureAndAppRun) {
    std::filesystem::path test_out = "build/test_dist_linux";
    std::error_code ec;
    std::filesystem::create_directories(test_out, ec);

    std::filesystem::path dummy_bin = test_out / "dummy_linux";
    {
        std::ofstream out(dummy_bin, std::ios::binary);
        out << "\x7F" "ELF";
    }

    PackageDescriptor desc;
    desc.app_name = "LinuxPingDart";
    desc.bundle_id = "dev.pingdart.linux";
    desc.executable_path = dummy_bin.string();
    desc.output_directory = test_out.string();
    desc.target = TargetPlatform::LinuxX64;

    Packager packager;
    auto res = packager.package_linux(desc);
    ASSERT_TRUE(res.has_value());

    const auto& pkg = res.value();
    EXPECT_EQ(pkg.format, PackageFormat::LinuxAppDir);

    std::filesystem::path apprun = std::filesystem::path(pkg.package_root_path) / "AppRun";
    EXPECT_TRUE(std::filesystem::exists(apprun, ec));

    std::ifstream ar_in(apprun);
    std::string ar_content((std::istreambuf_iterator<char>(ar_in)), std::istreambuf_iterator<char>());
    EXPECT_NE(ar_content.find("export LD_LIBRARY_PATH=\"${HERE}/usr/lib:${LD_LIBRARY_PATH}\""), std::string::npos);
    EXPECT_NE(ar_content.find("exec \"${HERE}/usr/bin/LinuxPingDart\" \"$@\""), std::string::npos);

    std::filesystem::path desktop_file = std::filesystem::path(pkg.package_root_path) / "LinuxPingDart.desktop";
    EXPECT_TRUE(std::filesystem::exists(desktop_file, ec));
}

TEST(PackagingTest, FontBundlerStrictlyRejectsSystemProprietaryFonts) {
    FontBundler bundler;

    // 1. Prohibit Apple system proprietary fonts
    EXPECT_TRUE(FontBundler::is_system_proprietary("/System/Library/Fonts/KohinoorTelugu.ttc"));
    EXPECT_TRUE(FontBundler::is_system_proprietary("/System/Library/Fonts/Helvetica.ttc"));
    EXPECT_TRUE(FontBundler::is_system_proprietary("/Library/Fonts/Arial.ttf"));

    // Attempting to evaluate system proprietary font returns PermissionDenied
    auto eval1 = bundler.evaluate_font_for_bundle("/System/Library/Fonts/KohinoorTelugu.ttc");
    EXPECT_FALSE(eval1.has_value());
    EXPECT_EQ(eval1.status().code(), core::StatusCode::PermissionDenied);
    EXPECT_NE(eval1.status().message().find("Refusing to bundle proprietary system font"), std::string::npos);

    // 2. Permitted project font
    std::filesystem::path dummy_font = "build/test_dist/OpenSans-Regular.ttf";
    {
        std::ofstream out(dummy_font, std::ios::binary);
        out << "ttf font data";
    }

    auto eval2 = bundler.evaluate_font_for_bundle(dummy_font.string(), "SIL Open Font License 1.1");
    ASSERT_TRUE(eval2.has_value());
    EXPECT_TRUE(eval2.value().is_redistributable);
    EXPECT_EQ(eval2.value().license_type, "SIL Open Font License 1.1");
}

TEST(PackagingTest, PackageVerifierAuditsMacOSBundle) {
    std::filesystem::path test_out = "build/test_dist_verify";
    std::error_code ec;
    std::filesystem::create_directories(test_out, ec);

    // Use actual compiled binary if present, or dummy
    std::string live_bin = "build/examples/desktop_login/desktop_login";
    if (!std::filesystem::exists(live_bin, ec)) {
        live_bin = (test_out / "live_bin").string();
        std::ofstream out(live_bin, std::ios::binary);
        out << "bin";
    }

    PackageDescriptor desc;
    desc.app_name = "AuditedPingDart";
    desc.bundle_id = "dev.pingdart.audit";
    desc.executable_path = live_bin;
    desc.output_directory = test_out.string();
    desc.target = TargetPlatform::MacOSArm64;

    Packager packager;
    auto pkg_res = packager.package_macos(desc);
    ASSERT_TRUE(pkg_res.has_value());

    Verifier verifier;
    auto verify_res = verifier.verify(pkg_res.value().package_root_path, false);
    ASSERT_TRUE(verify_res.has_value());

    const auto& report = verify_res.value();
    EXPECT_TRUE(report.valid);
    EXPECT_EQ(report.detected_format, PackageFormat::MacOSAppBundle);
    EXPECT_FALSE(report.checks.empty());

    std::string formatted = verifier.format_report(report);
    EXPECT_NE(formatted.find("Package Verification PASSED"), std::string::npos);
}

TEST(PackagingTest, SmokeLaunchExecutionOnNativeBinary) {
    std::string live_bin = "build/examples/desktop_login/desktop_login";
    std::error_code ec;
    if (std::filesystem::exists(live_bin, ec)) {
        auto res = Verifier::smoke_launch_binary(live_bin, 3000);
        EXPECT_TRUE(res.has_value());
        EXPECT_TRUE(res.value());
    }
}

TEST(PackagingTest, AndroidProjectScaffoldingGeneration) {
    std::filesystem::path test_out = "build/test_dist_android";
    std::error_code ec;
    std::filesystem::create_directories(test_out, ec);

    PackageDescriptor desc;
    desc.app_name = "MobileLogin";
    desc.bundle_id = "dev.pingdart.mobilelogin";
    desc.version = "1.2.0";
    desc.output_directory = test_out.string();
    desc.target = TargetPlatform::AndroidArm64;

    Packager packager;
    auto res = packager.create_package(desc);
    ASSERT_TRUE(res.has_value());

    const auto& pkg = res.value();
    EXPECT_TRUE(pkg.success);
    EXPECT_EQ(pkg.format, PackageFormat::AndroidProject);

    std::filesystem::path root(pkg.package_root_path);
    std::filesystem::path manifest = root / "app" / "src" / "main" / "AndroidManifest.xml";
    std::filesystem::path gradle = root / "app" / "build.gradle.kts";
    std::filesystem::path native_main = root / "app" / "src" / "main" / "cpp" / "native_activity_main.cpp";

    EXPECT_TRUE(std::filesystem::exists(manifest, ec));
    EXPECT_TRUE(std::filesystem::exists(gradle, ec));
    EXPECT_TRUE(std::filesystem::exists(native_main, ec));

    auto read_file_text = [](const std::filesystem::path& p) -> std::string {
        std::ifstream in(p);
        return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    };

    std::string m_content = read_file_text(manifest);
    EXPECT_NE(m_content.find("android.app.NativeActivity"), std::string::npos);
    EXPECT_NE(m_content.find("android.hardware.vulkan.version"), std::string::npos);

    std::string g_content = read_file_text(gradle);
    EXPECT_NE(g_content.find("dev.pingdart.mobilelogin"), std::string::npos);
    EXPECT_NE(g_content.find("externalNativeBuild"), std::string::npos);
}

TEST(PackagingTest, IOSProjectScaffoldingGeneration) {
    std::filesystem::path test_out = "build/test_dist_ios";
    std::error_code ec;
    std::filesystem::create_directories(test_out, ec);

    PackageDescriptor desc;
    desc.app_name = "IOSLogin";
    desc.bundle_id = "dev.pingdart.ioslogin";
    desc.version = "1.0.0";
    desc.output_directory = test_out.string();
    desc.target = TargetPlatform::IOSArm64;

    Packager packager;
    auto res = packager.create_package(desc);
    ASSERT_TRUE(res.has_value());

    const auto& pkg = res.value();
    EXPECT_TRUE(pkg.success);
    EXPECT_EQ(pkg.format, PackageFormat::IOSXcodeBundle);

    std::filesystem::path root(pkg.package_root_path);
    std::filesystem::path plist = root / "IOSLogin" / "Info.plist";
    std::filesystem::path vc = root / "IOSLogin" / "PingDartViewController.mm";

    EXPECT_TRUE(std::filesystem::exists(plist, ec));
    EXPECT_TRUE(std::filesystem::exists(vc, ec));

    auto read_file_text = [](const std::filesystem::path& p) -> std::string {
        std::ifstream in(p);
        return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    };

    std::string p_content = read_file_text(plist);
    EXPECT_NE(p_content.find("<string>metal</string>"), std::string::npos);
    EXPECT_NE(p_content.find("<string>arm64</string>"), std::string::npos);

    std::string vc_content = read_file_text(vc);
    EXPECT_NE(vc_content.find("CAMetalLayer"), std::string::npos);
    EXPECT_NE(vc_content.find("viewSafeAreaInsetsDidChange"), std::string::npos);
}
