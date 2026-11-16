// include/pingdart/pkg/os_sandbox.h
// PKG-111B: OS-Level Isolated Package Build Workers (Windows Job & Linux Namespaces)
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <pingdart/core/result.h>

namespace pingdart::pkg {

enum class OsSandboxEngine {
    Auto,
    WindowsJobAndToken,
    LinuxNamespacesAndSeccomp,
    IsolatedWorkerContainer
};

struct OsSandboxLimits {
    uint32_t max_cpu_time_ms{60000};
    uint64_t max_memory_bytes{1024ULL * 1024ULL * 1024ULL}; // 1GB
    uint32_t max_processes{32};                             // Fork-bomb defense
    bool deny_network{true};                                 // Disallow AF_INET/AF_INET6
    bool read_only_host_fs{true};                            // Host root mounted read-only
    bool strip_host_credentials{true};                       // Drop ambient tokens & keys
};

struct OsSandboxExecutionReport {
    bool success{false};
    int exit_code{0};
    uint64_t elapsed_ms{0};
    bool memory_limit_hit{false};
    bool timeout_hit{false};
    bool network_attempt_blocked{false};
    std::string stdout_log;
    std::string stderr_log;
};

class OsSandboxWorker {
public:
    explicit OsSandboxWorker(OsSandboxLimits limits = OsSandboxLimits{});

    [[nodiscard]] static bool is_os_isolation_available() noexcept;
    [[nodiscard]] static OsSandboxEngine detect_host_engine() noexcept;

    // Executes a build job inside an OS-enforced isolation boundary
    [[nodiscard]] core::Result<OsSandboxExecutionReport> run_isolated_build(
        const std::filesystem::path& workspace_dir,
        std::string_view command_line
    );

    [[nodiscard]] const OsSandboxLimits& limits() const noexcept { return limits_; }

private:
    OsSandboxLimits limits_;

#if defined(_WIN32)
    [[nodiscard]] core::Result<OsSandboxExecutionReport> run_windows_job_worker(
        const std::filesystem::path& workspace_dir,
        std::string_view command_line
    );
#elif defined(__linux__)
    [[nodiscard]] core::Result<OsSandboxExecutionReport> run_linux_namespace_worker(
        const std::filesystem::path& workspace_dir,
        std::string_view command_line
    );
#endif
};

} // namespace pingdart::pkg
