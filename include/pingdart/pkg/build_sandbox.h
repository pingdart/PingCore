// include/pingdart/pkg/build_sandbox.h
// PKG-109 / PKG-110: Isolated Build Execution Sandbox for Untrusted Packages
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <filesystem>
#include <cstdint>
#include <pingdart/core/result.h>

namespace pingdart::pkg {

struct SandboxExecutionResult {
    bool success{false};
    int exit_code{0};
    uint64_t elapsed_ms{0};
    std::string stdout_log;
    std::string stderr_log;
    bool quota_exceeded{false};
};

struct SandboxPolicy {
    bool offline_network{true};
    bool strip_environment_secrets{true};
    uint32_t timeout_seconds{60};
    uint64_t max_output_bytes{10 * 1024 * 1024}; // 10MB
    std::vector<std::string> allowed_env_vars = {
        "PATH", "TEMP", "TMP", "SYSTEMROOT", "COMSPEC", "CXX", "CC"
    };
};

class IsolatedPackageBuilder {
public:
    explicit IsolatedPackageBuilder(SandboxPolicy policy = SandboxPolicy{});

    // Validates that an untrusted package builds safely inside an isolated directory
    [[nodiscard]] core::Result<SandboxExecutionResult> test_build_in_sandbox(
        const std::filesystem::path& package_source_dir
    );

    // Sanitizes environment map by removing credentials, tokens, and home secrets
    [[nodiscard]] static std::map<std::string, std::string> sanitize_environment(
        const std::map<std::string, std::string>& raw_env,
        const std::vector<std::string>& whitelist
    );

    [[nodiscard]] const SandboxPolicy& policy() const noexcept { return policy_; }

private:
    SandboxPolicy policy_;
};

} // namespace pingdart::pkg
