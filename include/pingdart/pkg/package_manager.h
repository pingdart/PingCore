// include/pingdart/pkg/package_manager.h
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <pingdart/core/result.h>
#include <pingdart/pkg/manifest.h>
#include <pingdart/pkg/lockfile.h>
#include <pingdart/pkg/registry_client.h>
#include <pingdart/pkg/resolver.h>

namespace pingdart::pkg {

struct OutdatedPackageInfo {
    std::string name;
    SemVersion current_version;
    SemVersion latest_version;
    bool has_update{false};
};

struct AuditReport {
    std::vector<SecurityAdvisory> detected_vulnerabilities;
    size_t packages_scanned{0};
};

class PackageManager {
public:
    explicit PackageManager(
        std::filesystem::path project_root = ".",
        std::shared_ptr<RegistryClient> registry = nullptr
    );

    // PKG-102: Basic Package Operations
    [[nodiscard]] core::Status add(const std::string& package_name, std::string_view version_req = {});
    [[nodiscard]] core::Status remove(const std::string& package_name);
    [[nodiscard]] core::Result<std::vector<LockedPackage>> list();
    [[nodiscard]] core::Result<std::vector<PackageInfo>> search(std::string_view query);

    // PKG-103/106: Update & Sync
    [[nodiscard]] core::Status update();
    [[nodiscard]] core::Result<std::vector<OutdatedPackageInfo>> outdated();
    [[nodiscard]] core::Result<AuditReport> audit();

    // PKG-105: Authentication & Publishing
    [[nodiscard]] core::Status login(std::string_view token);
    [[nodiscard]] core::Status publish();

    // Scaffolding: Create Reusable Library Package Project
    [[nodiscard]] static core::Status create_package_scaffold(
        const std::string& package_name,
        const std::filesystem::path& destination_dir
    );

    // PKG-107: CMake Build Integration Generator
    [[nodiscard]] core::Status generate_cmake_include(
        const Lockfile& lockfile,
        const std::filesystem::path& out_path
    ) const;

    [[nodiscard]] RegistryClient& registry() noexcept { return *registry_; }

private:
    std::filesystem::path project_root_;
    std::shared_ptr<RegistryClient> registry_;
    DependencyResolver resolver_;

    [[nodiscard]] core::Status sync_dependencies(PackageManifest& manifest, bool force_latest);
};

} // namespace pingdart::pkg
