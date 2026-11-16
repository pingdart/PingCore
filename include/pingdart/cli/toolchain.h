#pragma once

#include <string>
#include <vector>
#include <pingdart/core/result.h>
#include <pingdart/core/status.h>

namespace pingdart::cli {

enum class ToolStatus {
    Available,
    Missing,
    Warning
};

struct ToolDiagnostic {
    std::string name;
    std::string version;
    std::string path;
    ToolStatus status{ToolStatus::Missing};
    std::string details;
    bool required{true};
};

struct DoctorReport {
    std::vector<ToolDiagnostic> diagnostics;
    bool toolchain_ready{false};
    std::string summary;
};

class Doctor {
public:
    Doctor() = default;

    [[nodiscard]] DoctorReport run_diagnostics();
    [[nodiscard]] std::string format_report(const DoctorReport& report) const;

private:
    [[nodiscard]] ToolDiagnostic check_compiler() const;
    [[nodiscard]] ToolDiagnostic check_cmake() const;
    [[nodiscard]] ToolDiagnostic check_ninja() const;
    [[nodiscard]] ToolDiagnostic check_rust() const;
    [[nodiscard]] ToolDiagnostic check_freetype() const;
    [[nodiscard]] ToolDiagnostic check_harfbuzz() const;
    [[nodiscard]] ToolDiagnostic check_vulkan() const;
    [[nodiscard]] ToolDiagnostic check_platform_sdk() const;
};

} // namespace pingdart::cli
