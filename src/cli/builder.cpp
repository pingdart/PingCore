#include <pingdart/cli/builder.h>
#include <cstdio>
#include <array>
#include <sstream>
#include <filesystem>

namespace pingdart::cli {

std::string Builder::target_platform_to_string(TargetPlatform target) {
    switch (target) {
        case TargetPlatform::Host: return "host";
        case TargetPlatform::MacOSArm64: return "macos-arm64";
        case TargetPlatform::MacOSX86_64: return "macos-x86_64";
        case TargetPlatform::WindowsX64: return "windows-x64";
        case TargetPlatform::LinuxX64: return "linux-x64";
        case TargetPlatform::AndroidArm64: return "android-arm64";
        case TargetPlatform::IOSArm64: return "ios-arm64";
    }
    return "host";
}

core::Result<TargetPlatform> Builder::parse_target_platform(std::string_view str) {
    if (str == "host" || str == "native") return TargetPlatform::Host;
    if (str == "macos-arm64" || str == "darwin-arm64" || str == "macos-aarch64") return TargetPlatform::MacOSArm64;
    if (str == "macos-x86_64" || str == "darwin-x64") return TargetPlatform::MacOSX86_64;
    if (str == "windows-x64" || str == "win-x64" || str == "windows") return TargetPlatform::WindowsX64;
    if (str == "linux-x64" || str == "linux") return TargetPlatform::LinuxX64;
    if (str == "android-arm64" || str == "android" || str == "android-aarch64") return TargetPlatform::AndroidArm64;
    if (str == "ios-arm64" || str == "ios" || str == "iphone") return TargetPlatform::IOSArm64;

    return core::Status(core::StatusCode::InvalidArgument,
                        "Unknown target platform: " + std::string(str) + ". Supported: macos-arm64, macos-x86_64, windows-x64, linux-x64, android-arm64, ios-arm64, host");
}

std::vector<std::string> Builder::generate_cmake_configure_args(const BuildOptions& options) const {
    std::vector<std::string> args;
    args.push_back("cmake");
    args.push_back("-S");
    args.push_back(options.source_dir);
    args.push_back("-B");
    args.push_back(options.build_dir);
    args.push_back("-G");
    args.push_back("Ninja");

    std::string mode_str = (options.mode == BuildMode::Debug) ? "Debug" : "Release";
    args.push_back("-DCMAKE_BUILD_TYPE=" + mode_str);

    if (options.target == TargetPlatform::MacOSArm64) {
        args.push_back("-DCMAKE_OSX_ARCHITECTURES=arm64");
    } else if (options.target == TargetPlatform::MacOSX86_64) {
        args.push_back("-DCMAKE_OSX_ARCHITECTURES=x86_64");
    } else if (options.target == TargetPlatform::AndroidArm64) {
        args.push_back("-DCMAKE_SYSTEM_NAME=Android");
        args.push_back("-DCMAKE_ANDROID_ARCH_ABI=arm64-v8a");
        args.push_back("-DCMAKE_SYSTEM_VERSION=26");
    } else if (options.target == TargetPlatform::IOSArm64) {
        args.push_back("-DCMAKE_SYSTEM_NAME=iOS");
        args.push_back("-DCMAKE_OSX_ARCHITECTURES=arm64");
    }

    for (const auto& extra : options.extra_cmake_args) {
        args.push_back(extra);
    }

    return args;
}

std::vector<std::string> Builder::generate_cmake_build_args(const BuildOptions& options) const {
    std::vector<std::string> args;
    args.push_back("cmake");
    args.push_back("--build");
    args.push_back(options.build_dir);
    if (!options.target_app.empty()) {
        args.push_back("--target");
        args.push_back(options.target_app);
    }
    return args;
}

core::Result<BuildResult> Builder::build(const BuildOptions& options) {
    BuildResult result;
    result.target_name = options.target_app;

    // 1. Configure step
    auto conf_args = generate_cmake_configure_args(options);
    std::string conf_cmd;
    for (size_t i = 0; i < conf_args.size(); ++i) {
        if (i > 0) conf_cmd += " ";
        conf_cmd += conf_args[i];
    }
    conf_cmd += " 2>&1";

    FILE* p1 = popen(conf_cmd.c_str(), "r");
    if (!p1) {
        return core::Status(core::StatusCode::Internal, "Failed to launch CMake configuration process");
    }
    std::array<char, 256> buf{};
    while (fgets(buf.data(), static_cast<int>(buf.size()), p1) != nullptr) {
        result.log_output += buf.data();
    }
    int code1 = pclose(p1);
    if (code1 != 0) {
        result.success = false;
        result.error_count++;
        return core::Status(core::StatusCode::Internal, "CMake configure failed with exit code: " + std::to_string(code1));
    }

    // 2. Build step
    auto build_args = generate_cmake_build_args(options);
    std::string build_cmd;
    for (size_t i = 0; i < build_args.size(); ++i) {
        if (i > 0) build_cmd += " ";
        build_cmd += build_args[i];
    }
    build_cmd += " 2>&1";

    FILE* p2 = popen(build_cmd.c_str(), "r");
    if (!p2) {
        return core::Status(core::StatusCode::Internal, "Failed to launch CMake build process");
    }
    while (fgets(buf.data(), static_cast<int>(buf.size()), p2) != nullptr) {
        result.log_output += buf.data();
    }
    int code2 = pclose(p2);
    if (code2 != 0) {
        result.success = false;
        result.error_count++;
        return core::Status(core::StatusCode::Internal, "CMake build failed with exit code: " + std::to_string(code2));
    }

    // Resolve binary path
    std::string candidate1 = options.build_dir + "/examples/" + options.target_app + "/" + options.target_app;
    std::string candidate2 = options.build_dir + "/" + options.target_app;
    std::string candidate3 = options.build_dir + "/bin/" + options.target_app;

    std::error_code ec;
    if (std::filesystem::exists(candidate1, ec)) {
        result.executable_path = candidate1;
    } else if (std::filesystem::exists(candidate2, ec)) {
        result.executable_path = candidate2;
    } else if (std::filesystem::exists(candidate3, ec)) {
        result.executable_path = candidate3;
    } else {
        result.executable_path = candidate1; // Default
    }

    result.success = true;
    return result;
}

} // namespace pingdart::cli
