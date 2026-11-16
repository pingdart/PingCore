// include/pingdart/pkg/manifest.h
#pragma once
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <filesystem>
#include <pingdart/core/result.h>
#include <pingdart/pkg/semver.h>

namespace pingdart::pkg {

struct PackageMetadata {
    std::string name;
    SemVersion version{1, 0, 0, ""};
    std::string description;
    std::string license{"MIT"};
    std::string edition{"2026"};
    std::vector<std::string> authors;
    std::string repository;
};

struct PlatformMatrix {
    bool android{true};
    bool ios{true};
    bool windows{true};
    bool macos{true};
    bool linux{true};
    bool web{true};
};

struct Dependency {
    std::string name;
    VersionConstraint constraint;
    std::string raw_requirement{"*"};
    std::string local_path;
};

class PackageManifest {
public:
    PackageManifest() = default;

    [[nodiscard]] static core::Result<PackageManifest> parse(std::string_view toml_content);
    [[nodiscard]] static core::Result<PackageManifest> load_from_file(const std::filesystem::path& file_path);

    [[nodiscard]] std::string to_toml_string() const;
    core::Status save_to_file(const std::filesystem::path& file_path) const;

    void add_dependency(const std::string& name, const std::string& req_str);
    bool remove_dependency(const std::string& name);

    [[nodiscard]] const PackageMetadata& metadata() const noexcept { return metadata_; }
    [[nodiscard]] PackageMetadata& metadata() noexcept { return metadata_; }

    [[nodiscard]] const PlatformMatrix& platforms() const noexcept { return platforms_; }
    [[nodiscard]] PlatformMatrix& platforms() noexcept { return platforms_; }

    [[nodiscard]] const std::map<std::string, Dependency>& dependencies() const noexcept { return dependencies_; }
    [[nodiscard]] const std::map<std::string, Dependency>& dev_dependencies() const noexcept { return dev_dependencies_; }

private:
    PackageMetadata metadata_{};
    PlatformMatrix platforms_{};
    std::map<std::string, Dependency> dependencies_{};
    std::map<std::string, Dependency> dev_dependencies_{};
};

} // namespace pingdart::pkg
