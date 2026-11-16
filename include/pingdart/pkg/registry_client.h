// include/pingdart/pkg/registry_client.h
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <memory>
#include <pingdart/core/result.h>
#include <pingdart/pkg/semver.h>
#include <pingdart/pkg/manifest.h>

namespace pingdart::pkg {

struct PackageVersionInfo {
    SemVersion version;
    std::string description;
    std::string checksum; // e.g. sha256:...
    std::map<std::string, std::string> dependencies;
    PlatformMatrix platforms;
    std::vector<uint8_t> archive_payload; // Source archive data
};

struct PackageInfo {
    std::string name;
    SemVersion latest_version;
    std::string description;
    std::string license;
    std::vector<PackageVersionInfo> versions;
};

struct SecurityAdvisory {
    std::string advisory_id;
    std::string package_name;
    VersionConstraint affected_constraint;
    std::string patched_version;
    std::string severity; // Low, Medium, High, Critical
    std::string description;
};

class IRegistryBackend {
public:
    virtual ~IRegistryBackend() = default;

    [[nodiscard]] virtual core::Result<std::vector<PackageInfo>> search(std::string_view query) = 0;
    [[nodiscard]] virtual core::Result<PackageInfo> get_package(std::string_view name) = 0;
    [[nodiscard]] virtual core::Result<PackageVersionInfo> get_version_info(std::string_view name, const SemVersion& version) = 0;
    [[nodiscard]] virtual core::Result<std::vector<uint8_t>> download_archive(std::string_view name, const SemVersion& version) = 0;
    [[nodiscard]] virtual core::Status publish_package(const PackageManifest& manifest, const std::vector<uint8_t>& archive_data, std::string_view auth_token) = 0;
    [[nodiscard]] virtual core::Result<std::vector<SecurityAdvisory>> get_security_advisories() = 0;
};

class RegistryClient {
public:
    explicit RegistryClient(std::shared_ptr<IRegistryBackend> backend = nullptr);

    [[nodiscard]] core::Result<std::vector<PackageInfo>> search(std::string_view query);
    [[nodiscard]] core::Result<PackageInfo> get_package(std::string_view name);
    [[nodiscard]] core::Result<PackageVersionInfo> get_version_info(std::string_view name, const SemVersion& version);
    [[nodiscard]] core::Result<std::vector<uint8_t>> download_archive(std::string_view name, const SemVersion& version);
    [[nodiscard]] core::Status publish_package(const PackageManifest& manifest, const std::vector<uint8_t>& archive_data, std::string_view auth_token);
    [[nodiscard]] core::Result<std::vector<SecurityAdvisory>> check_advisories();

    void set_backend(std::shared_ptr<IRegistryBackend> backend) noexcept;
    [[nodiscard]] IRegistryBackend* backend() const noexcept;

    // Local Credentials Management (~/.pingdart/credentials.toml)
    static core::Status save_auth_token(std::string_view token, const std::string& registry_url = "https://registry.pingdart.com");
    [[nodiscard]] static core::Result<std::string> load_auth_token(const std::string& registry_url = "https://registry.pingdart.com");

private:
    std::shared_ptr<IRegistryBackend> backend_;
};

// Built-in Default In-Memory Registry Backend with Standard Ecosystem Packages
class DefaultRegistryBackend final : public IRegistryBackend {
public:
    DefaultRegistryBackend();

    core::Result<std::vector<PackageInfo>> search(std::string_view query) override;
    core::Result<PackageInfo> get_package(std::string_view name) override;
    core::Result<PackageVersionInfo> get_version_info(std::string_view name, const SemVersion& version) override;
    core::Result<std::vector<uint8_t>> download_archive(std::string_view name, const SemVersion& version) override;
    core::Status publish_package(const PackageManifest& manifest, const std::vector<uint8_t>& archive_data, std::string_view auth_token) override;
    core::Result<std::vector<SecurityAdvisory>> get_security_advisories() override;

private:
    std::map<std::string, PackageInfo> registry_;
    std::vector<SecurityAdvisory> advisories_;
    void seed_default_packages();
};

} // namespace pingdart::pkg
