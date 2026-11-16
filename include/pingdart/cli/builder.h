#pragma once

#include <string>
#include <vector>
#include <pingdart/core/result.h>
#include <pingdart/core/status.h>

namespace pingdart::cli {

enum class TargetPlatform {
    Host,
    MacOSArm64,
    MacOSX86_64,
    WindowsX64,
    LinuxX64,
    AndroidArm64,
    IOSArm64
};

enum class BuildMode {
    Debug,
    Release
};

struct BuildOptions {
    TargetPlatform target{TargetPlatform::Host};
    BuildMode mode{BuildMode::Release};
    std::string source_dir{"."};
    std::string build_dir{"build"};
    std::string target_app{"desktop_login"};
    std::vector<std::string> extra_cmake_args;
    bool verbose{false};
};

struct BuildResult {
    bool success{false};
    std::string target_name;
    std::string executable_path;
    std::string log_output;
    uint32_t warning_count{0};
    uint32_t error_count{0};
};

class Builder {
public:
    Builder() = default;

    [[nodiscard]] static std::string target_platform_to_string(TargetPlatform target);
    [[nodiscard]] static core::Result<TargetPlatform> parse_target_platform(std::string_view str);

    [[nodiscard]] core::Result<BuildResult> build(const BuildOptions& options);
    [[nodiscard]] std::vector<std::string> generate_cmake_configure_args(const BuildOptions& options) const;
    [[nodiscard]] std::vector<std::string> generate_cmake_build_args(const BuildOptions& options) const;
};

} // namespace pingdart::cli
