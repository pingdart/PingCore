#pragma once

#include "packager.h"
#include <string>
#include <vector>
#include <pingdart/core/result.h>
#include <pingdart/core/status.h>

namespace pingdart::cli {

struct VerificationCheck {
    std::string check_name;
    bool passed{false};
    std::string details;
    bool critical{true};
};

struct PackageVerificationReport {
    bool valid{false};
    PackageFormat detected_format{PackageFormat::MacOSAppBundle};
    std::string package_path;
    std::string detected_architecture;
    std::vector<VerificationCheck> checks;
    bool smoke_launch_passed{false};
    std::string summary;
};

class Verifier {
public:
    Verifier() = default;

    [[nodiscard]] core::Result<PackageVerificationReport> verify(
        const std::string& package_path,
        bool run_smoke_launch = false
    );

    [[nodiscard]] std::string format_report(const PackageVerificationReport& report) const;

    [[nodiscard]] static std::string inspect_binary_architecture(const std::string& binary_path);
    [[nodiscard]] static core::Result<bool> smoke_launch_binary(const std::string& binary_path, uint32_t timeout_ms = 3000);

private:
    [[nodiscard]] core::Result<PackageVerificationReport> verify_macos_bundle(const std::string& bundle_path, bool smoke_test);
    [[nodiscard]] core::Result<PackageVerificationReport> verify_windows_package(const std::string& package_path, bool smoke_test);
    [[nodiscard]] core::Result<PackageVerificationReport> verify_linux_appdir(const std::string& appdir_path, bool smoke_test);
    [[nodiscard]] core::Result<PackageVerificationReport> verify_android_package(const std::string& android_path, bool smoke_test);
    [[nodiscard]] core::Result<PackageVerificationReport> verify_ios_package(const std::string& ios_path, bool smoke_test);
};

} // namespace pingdart::cli
