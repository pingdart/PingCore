// src/pkg/scoped_package.cpp
// PKG-110: Scoped Package Identification & Validation Implementation
#include <pingdart/pkg/scoped_package.h>
#include <algorithm>

namespace pingdart::pkg {

namespace {

bool is_valid_name_part(std::string_view str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_' && c != '.') {
            return false;
        }
    }
    return true;
}

} // namespace

ScopedPackageName::ScopedPackageName(std::string scope, std::string name)
    : scope_(std::move(scope)), name_(std::move(name)) {}

core::Result<ScopedPackageName> ScopedPackageName::parse(std::string_view raw_name) {
    // Trim quotes if wrapped
    if (raw_name.size() >= 2 && raw_name.front() == '"' && raw_name.back() == '"') {
        raw_name.remove_prefix(1);
        raw_name.remove_suffix(1);
    }

    if (raw_name.empty()) {
        return core::Status(core::StatusCode::InvalidArgument, "Package name cannot be empty");
    }

    if (raw_name.front() == '@') {
        size_t slash = raw_name.find('/');
        if (slash == std::string_view::npos || slash == 1 || slash == raw_name.size() - 1) {
            return core::Status(core::StatusCode::InvalidArgument,
                                "Invalid scoped package name format (expected @scope/package): " + std::string(raw_name));
        }

        std::string_view scope_part = raw_name.substr(0, slash);
        std::string_view name_part = raw_name.substr(slash + 1);

        // Validate scope
        if (!is_valid_name_part(scope_part.substr(1))) {
            return core::Status(core::StatusCode::InvalidArgument,
                                "Invalid scope identifier characters: " + std::string(scope_part));
        }

        // Validate package name
        if (!is_valid_name_part(name_part)) {
            return core::Status(core::StatusCode::InvalidArgument,
                                "Invalid package name characters: " + std::string(name_part));
        }

        return ScopedPackageName(std::string(scope_part), std::string(name_part));
    }

    if (!is_valid_name_part(raw_name)) {
        return core::Status(core::StatusCode::InvalidArgument,
                            "Invalid package name characters: " + std::string(raw_name));
    }

    return ScopedPackageName("", std::string(raw_name));
}

std::string ScopedPackageName::full_name() const {
    if (is_scoped()) {
        return scope_ + "/" + name_;
    }
    return name_;
}

std::string ScopedPackageName::to_cmake_target_name() const {
    std::string safe;
    if (is_scoped()) {
        std::string s = scope_.starts_with('@') ? scope_.substr(1) : scope_;
        safe = s + "_" + name_;
    } else {
        safe = name_;
        if (safe.starts_with("pingdart-") || safe.starts_with("pingdart_")) {
            safe = safe.substr(9);
        }
    }

    std::replace(safe.begin(), safe.end(), '-', '_');
    std::replace(safe.begin(), safe.end(), '/', '_');
    std::replace(safe.begin(), safe.end(), '.', '_');
    return safe;
}

} // namespace pingdart::pkg
