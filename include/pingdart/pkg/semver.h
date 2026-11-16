// include/pingdart/pkg/semver.h
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <pingdart/core/result.h>

namespace pingdart::pkg {

struct SemVersion {
    uint32_t major{0};
    uint32_t minor{0};
    uint32_t patch{0};
    std::string prerelease;

    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] static core::Result<SemVersion> parse(std::string_view str);

    auto operator<=>(const SemVersion& other) const = default;
};

enum class ConstraintOp {
    Exact,       // =1.2.3 or 1.2.3
    Caret,       // ^1.2.3 (compatible updates)
    Tilde,       // ~1.2.3 (patch updates)
    GreaterThan, // >1.2.3
    GreaterEqual,// >=1.2.3
    LessThan,    // <1.2.3
    LessEqual,   // <=1.2.3
    Any          // *
};

class VersionConstraint {
public:
    VersionConstraint() = default;
    VersionConstraint(ConstraintOp op, SemVersion version);

    [[nodiscard]] static core::Result<VersionConstraint> parse(std::string_view req_str);
    [[nodiscard]] bool is_satisfied_by(const SemVersion& version) const noexcept;
    [[nodiscard]] std::string to_string() const;

private:
    ConstraintOp op_{ConstraintOp::Any};
    SemVersion version_{};
};

} // namespace pingdart::pkg
