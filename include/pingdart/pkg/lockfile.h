// include/pingdart/pkg/lockfile.h
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <filesystem>
#include <pingdart/core/result.h>
#include <pingdart/pkg/semver.h>

namespace pingdart::pkg {

struct LockedPackage {
    std::string name;
    SemVersion version{1, 0, 0, ""};
    std::string source{"registry+https://registry.pingdart.com"};
    std::string checksum;
    std::vector<std::string> dependencies;
};

class Lockfile {
public:
    Lockfile() = default;

    [[nodiscard]] static core::Result<Lockfile> parse(std::string_view content);
    [[nodiscard]] static core::Result<Lockfile> load_from_file(const std::filesystem::path& file_path);

    [[nodiscard]] std::string to_string() const;
    core::Status save_to_file(const std::filesystem::path& file_path) const;

    [[nodiscard]] const LockedPackage* find_package(std::string_view name) const noexcept;
    void add_or_update_package(LockedPackage pkg);
    bool remove_package(std::string_view name);

    [[nodiscard]] uint32_t schema_version() const noexcept { return schema_version_; }
    [[nodiscard]] const std::vector<LockedPackage>& packages() const noexcept { return packages_; }

private:
    uint32_t schema_version_{1};
    std::vector<LockedPackage> packages_{};
};

} // namespace pingdart::pkg
