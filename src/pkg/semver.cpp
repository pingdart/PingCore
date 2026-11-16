// src/pkg/semver.cpp
#include <pingdart/pkg/semver.h>
#include <sstream>
#include <charconv>

namespace pingdart::pkg {

std::string SemVersion::to_string() const {
    std::ostringstream ss;
    ss << major << "." << minor << "." << patch;
    if (!prerelease.empty()) {
        ss << "-" << prerelease;
    }
    return ss.str();
}

core::Result<SemVersion> SemVersion::parse(std::string_view str) {
    if (str.empty()) {
        return core::Status(core::StatusCode::InvalidArgument, "Empty SemVer string");
    }

    // Strip leading 'v' or 'V' if present
    if (str.front() == 'v' || str.front() == 'V') {
        str.remove_prefix(1);
    }

    SemVersion v{};
    std::string_view remaining = str;

    // Prerelease suffix
    size_t dash_pos = remaining.find('-');
    if (dash_pos != std::string_view::npos) {
        v.prerelease = std::string(remaining.substr(dash_pos + 1));
        remaining = remaining.substr(0, dash_pos);
    }

    // Major
    size_t dot1 = remaining.find('.');
    if (dot1 == std::string_view::npos) {
        return core::Status(core::StatusCode::InvalidArgument, "Invalid SemVer: missing minor and patch version");
    }
    auto [p1, ec1] = std::from_chars(remaining.data(), remaining.data() + dot1, v.major);
    if (ec1 != std::errc() || p1 != remaining.data() + dot1) {
        return core::Status(core::StatusCode::InvalidArgument, "Invalid SemVer major component");
    }

    remaining = remaining.substr(dot1 + 1);

    // Minor
    size_t dot2 = remaining.find('.');
    if (dot2 == std::string_view::npos) {
        // Allow major.minor without patch (defaults patch to 0)
        auto [p2, ec2] = std::from_chars(remaining.data(), remaining.data() + remaining.size(), v.minor);
        if (ec2 != std::errc() || p2 != remaining.data() + remaining.size()) {
            return core::Status(core::StatusCode::InvalidArgument, "Invalid SemVer minor component");
        }
        v.patch = 0;
        return v;
    }

    auto [p2, ec2] = std::from_chars(remaining.data(), remaining.data() + dot2, v.minor);
    if (ec2 != std::errc() || p2 != remaining.data() + dot2) {
        return core::Status(core::StatusCode::InvalidArgument, "Invalid SemVer minor component");
    }

    remaining = remaining.substr(dot2 + 1);

    // Patch
    auto [p3, ec3] = std::from_chars(remaining.data(), remaining.data() + remaining.size(), v.patch);
    if (ec3 != std::errc() || p3 != remaining.data() + remaining.size()) {
        return core::Status(core::StatusCode::InvalidArgument, "Invalid SemVer patch component");
    }

    return v;
}

VersionConstraint::VersionConstraint(ConstraintOp op, SemVersion version)
    : op_(op), version_(std::move(version)) {}

core::Result<VersionConstraint> VersionConstraint::parse(std::string_view req_str) {
    // Trim leading whitespace
    while (!req_str.empty() && (req_str.front() == ' ' || req_str.front() == '\t')) {
        req_str.remove_prefix(1);
    }
    // Trim trailing whitespace
    while (!req_str.empty() && (req_str.back() == ' ' || req_str.back() == '\t')) {
        req_str.remove_suffix(1);
    }

    if (req_str.empty() || req_str == "*") {
        return VersionConstraint(ConstraintOp::Any, SemVersion{});
    }

    ConstraintOp op = ConstraintOp::Exact;
    std::string_view ver_part = req_str;

    if (req_str.starts_with(">=")) {
        op = ConstraintOp::GreaterEqual;
        ver_part = req_str.substr(2);
    } else if (req_str.starts_with("<=")) {
        op = ConstraintOp::LessEqual;
        ver_part = req_str.substr(2);
    } else if (req_str.starts_with(">")) {
        op = ConstraintOp::GreaterThan;
        ver_part = req_str.substr(1);
    } else if (req_str.starts_with("<")) {
        op = ConstraintOp::LessThan;
        ver_part = req_str.substr(1);
    } else if (req_str.starts_with("^")) {
        op = ConstraintOp::Caret;
        ver_part = req_str.substr(1);
    } else if (req_str.starts_with("~")) {
        op = ConstraintOp::Tilde;
        ver_part = req_str.substr(1);
    } else if (req_str.starts_with("=")) {
        op = ConstraintOp::Exact;
        ver_part = req_str.substr(1);
    }

    // Trim spaces before version string
    while (!ver_part.empty() && (ver_part.front() == ' ' || ver_part.front() == '\t')) {
        ver_part.remove_prefix(1);
    }

    auto v_res = SemVersion::parse(ver_part);
    if (!v_res.has_value()) {
        return v_res.status();
    }

    return VersionConstraint(op, std::move(v_res).value());
}

bool VersionConstraint::is_satisfied_by(const SemVersion& v) const noexcept {
    switch (op_) {
        case ConstraintOp::Any:
            return true;
        case ConstraintOp::Exact:
            return v.major == version_.major && v.minor == version_.minor && v.patch == version_.patch;
        case ConstraintOp::GreaterThan:
            return v > version_;
        case ConstraintOp::GreaterEqual:
            return v >= version_;
        case ConstraintOp::LessThan:
            return v < version_;
        case ConstraintOp::LessEqual:
            return v <= version_;
        case ConstraintOp::Tilde:
            return (v >= version_) && (v.major == version_.major) && (v.minor == version_.minor);
        case ConstraintOp::Caret:
            if (version_.major > 0) {
                return (v >= version_) && (v.major == version_.major);
            } else if (version_.minor > 0) {
                return (v >= version_) && (v.major == 0) && (v.minor == version_.minor);
            } else {
                return v == version_;
            }
    }
    return false;
}

std::string VersionConstraint::to_string() const {
    switch (op_) {
        case ConstraintOp::Any: return "*";
        case ConstraintOp::Exact: return "=" + version_.to_string();
        case ConstraintOp::Caret: return "^" + version_.to_string();
        case ConstraintOp::Tilde: return "~" + version_.to_string();
        case ConstraintOp::GreaterThan: return ">" + version_.to_string();
        case ConstraintOp::GreaterEqual: return ">=" + version_.to_string();
        case ConstraintOp::LessThan: return "<" + version_.to_string();
        case ConstraintOp::LessEqual: return "<=" + version_.to_string();
    }
    return "*";
}

} // namespace pingdart::pkg
