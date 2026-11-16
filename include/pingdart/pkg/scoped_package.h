// include/pingdart/pkg/scoped_package.h
// PKG-110: Scoped Package Identification & Validation
#pragma once
#include <string>
#include <string_view>
#include <pingdart/core/result.h>

namespace pingdart::pkg {

class ScopedPackageName {
public:
    ScopedPackageName() = default;
    ScopedPackageName(std::string scope, std::string name);

    [[nodiscard]] static core::Result<ScopedPackageName> parse(std::string_view raw_name);

    [[nodiscard]] bool is_scoped() const noexcept { return !scope_.empty(); }
    [[nodiscard]] const std::string& scope() const noexcept { return scope_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] std::string full_name() const;

    // Generates a valid, sanitized CMake target identifier
    // e.g. "@company/internal-ui" -> "company_internal_ui"
    // e.g. "pingdart-charts" -> "charts"
    [[nodiscard]] std::string to_cmake_target_name() const;

    bool operator==(const ScopedPackageName& other) const = default;

private:
    std::string scope_; // e.g. "@company" or "@pingdart"
    std::string name_;  // e.g. "internal-ui" or "charts"
};

} // namespace pingdart::pkg
