#pragma once

#include "builder.h"
#include "font_bundler.h"
#include <string>
#include <vector>
#include <pingdart/core/result.h>
#include <pingdart/core/status.h>

namespace pingdart::cli {

enum class PackageFormat {
    MacOSAppBundle,
    WindowsExecutablePackage,
    LinuxAppDir,
    AndroidProject,
    IOSXcodeBundle
};

struct PackageDescriptor {
    std::string app_name{"PingDartApp"};
    std::string bundle_id{"dev.pingdart.app"};
    std::string version{"1.0.0"};
    std::string short_version{"1.0.0"};
    std::string executable_path;
    std::string output_directory{"dist"};
    TargetPlatform target{TargetPlatform::Host};
    std::string assets_directory;
    std::vector<std::string> font_paths;
    std::vector<std::string> shared_libraries;
    bool high_dpi_capable{true};
    std::string category{"Utility"};
    std::string signing_identity{"-"}; // "-" denotes ad-hoc signing for local verification
    bool codesign_bundle{true};
    bool create_appimage{false};
};

struct PackageResult {
    bool success{false};
    PackageFormat format{PackageFormat::MacOSAppBundle};
    std::string package_root_path;
    std::string executable_bundled_path;
    std::vector<std::string> bundled_files;
    uint64_t total_size_bytes{0};
    std::string manifest_or_plist_path;
};

class Packager {
public:
    Packager() = default;

    [[nodiscard]] core::Result<PackageResult> create_package(const PackageDescriptor& descriptor);

    // Platform-specific bundle generators
    [[nodiscard]] core::Result<PackageResult> package_macos(const PackageDescriptor& descriptor);
    [[nodiscard]] core::Result<PackageResult> package_windows(const PackageDescriptor& descriptor);
    [[nodiscard]] core::Result<PackageResult> package_linux(const PackageDescriptor& descriptor);
    [[nodiscard]] core::Result<PackageResult> package_android(const PackageDescriptor& descriptor);
    [[nodiscard]] core::Result<PackageResult> package_ios(const PackageDescriptor& descriptor);

    // Manifest and metadata string generators
    [[nodiscard]] static std::string generate_macos_info_plist(const PackageDescriptor& descriptor);
    [[nodiscard]] static std::string generate_windows_manifest(const PackageDescriptor& descriptor);
    [[nodiscard]] static std::string generate_linux_apprun(const PackageDescriptor& descriptor);
    [[nodiscard]] static std::string generate_linux_desktop_entry(const PackageDescriptor& descriptor);
    [[nodiscard]] static std::string generate_android_manifest(const PackageDescriptor& descriptor);
    [[nodiscard]] static std::string generate_android_build_gradle(const PackageDescriptor& descriptor);
    [[nodiscard]] static std::string generate_android_native_main(const PackageDescriptor& descriptor);
    [[nodiscard]] static std::string generate_ios_info_plist(const PackageDescriptor& descriptor);
    [[nodiscard]] static std::string generate_ios_view_controller_mm(const PackageDescriptor& descriptor);
};

} // namespace pingdart::cli
