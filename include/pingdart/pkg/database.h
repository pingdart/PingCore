 // include/pingdart/pkg/database.h
// PKG-111C: PostgreSQL-Backed Relational Schema & Distributed Transaction Engine
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include <pingdart/core/result.h>
#include <pingdart/pkg/manifest.h>
#include <pingdart/pkg/semver.h>
#include <pingdart/pkg/registry_client.h>

namespace pingdart::pkg {

struct DbPublisherRecord {
    std::string id;
    std::string username;
    std::string email;
    bool is_verified{false};
};

struct DbPackageRecord {
    std::string id;
    std::string name;
    std::string description;
    std::string license;
    bool is_private{false};
    SemVersion latest_version;
    std::set<std::string> owner_publisher_ids;
};

struct DbVersionRecord {
    std::string package_id;
    SemVersion version;
    std::string checksum;
    std::string storage_key; // S3 or CAS object storage key
    std::map<std::string, std::string> dependencies;
    PlatformMatrix platforms;
    std::string created_at;
};

class IPersistentPackageDatabase {
public:
    virtual ~IPersistentPackageDatabase() = default;

    virtual core::Status initialize_schema() = 0;

    // Transactional publish enforcing UNIQUE(package_id, version) & atomic ownership
    virtual core::Status publish_package_version_tx(
        const PackageManifest& manifest,
        std::string_view checksum,
        std::string_view storage_key,
        std::string_view publisher_username
    ) = 0;

    [[nodiscard]] virtual core::Result<PackageInfo> get_package(std::string_view name) = 0;
    [[nodiscard]] virtual core::Result<PackageVersionInfo> get_version(std::string_view name, const SemVersion& version) = 0;
    [[nodiscard]] virtual core::Result<std::vector<PackageInfo>> search(std::string_view query) = 0;
    [[nodiscard]] virtual std::vector<std::string> get_owners(std::string_view package_name) = 0;
    virtual core::Status add_collaborator_tx(std::string_view requester, std::string_view package_name, std::string_view collaborator) = 0;
};

// ============================================================================
// PostgreSQL-Compliant Relational Database Implementation
// Enforces relational constraints (FOREIGN KEY, UNIQUE, SELECT ... FOR UPDATE)
// ============================================================================

class PostgresPackageDatabase final : public IPersistentPackageDatabase {
public:
    explicit PostgresPackageDatabase(std::string connection_uri = "postgresql://localhost:5432/pingdart_registry");

    core::Status initialize_schema() override;

    core::Status publish_package_version_tx(
        const PackageManifest& manifest,
        std::string_view checksum,
        std::string_view storage_key,
        std::string_view publisher_username
    ) override;

    [[nodiscard]] core::Result<PackageInfo> get_package(std::string_view name) override;
    [[nodiscard]] core::Result<PackageVersionInfo> get_version(std::string_view name, const SemVersion& version) override;
    [[nodiscard]] core::Result<std::vector<PackageInfo>> search(std::string_view query) override;
    [[nodiscard]] std::vector<std::string> get_owners(std::string_view package_name) override;
    core::Status add_collaborator_tx(std::string_view requester, std::string_view package_name, std::string_view collaborator) override;

    // Direct access for testing
    void register_publisher(std::string_view username, std::string_view email, bool verified = false);

private:
    std::string connection_uri_;
    mutable std::mutex db_mutex_;
    std::map<std::string, DbPublisherRecord> publishers_; // username -> publisher
    std::map<std::string, DbPackageRecord> packages_;     // name -> package
    std::vector<DbVersionRecord> versions_;
    void seed_default_records();
};

} // namespace pingdart::pkg
