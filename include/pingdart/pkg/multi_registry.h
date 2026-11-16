// include/pingdart/pkg/multi_registry.h
// PKG-110: Private Registries & Scoped Package Multi-Registry Routing
#pragma once
#include <string>
#include <string_view>
#include <map>
#include <memory>
#include <vector>
#include <filesystem>
#include <pingdart/core/result.h>
#include <pingdart/pkg/semver.h>
#include <pingdart/pkg/manifest.h>
#include <pingdart/pkg/registry_client.h>
#include <pingdart/pkg/scoped_package.h>

namespace pingdart::pkg {

struct RegistryEndpoint {
    std::string alias;       // e.g. "public", "internal", "staging"
    std::string url;         // e.g. "https://registry.pingdart.com", "https://packages.example.com"
    bool is_private{false};  // When true, all operations (read & write) require authentication
    std::string auth_token;
    std::shared_ptr<IRegistryBackend> backend{nullptr};
};

class MultiRegistryConfig {
public:
    MultiRegistryConfig();

    void add_registry(RegistryEndpoint endpoint);
    void map_scope(std::string_view scope, std::string_view registry_alias);
    void set_auth_token(std::string_view registry_alias, std::string_view token);

    [[nodiscard]] const RegistryEndpoint* get_registry(std::string_view alias) const;
    [[nodiscard]] const RegistryEndpoint* resolve_for_package(std::string_view package_name) const;
    [[nodiscard]] std::string resolve_alias_for_package(std::string_view package_name) const;

    [[nodiscard]] static core::Result<MultiRegistryConfig> parse_toml(std::string_view toml_content);
    [[nodiscard]] std::string to_toml_string() const;

    [[nodiscard]] static core::Result<MultiRegistryConfig> load_default_config();
    core::Status save_default_config() const;

    [[nodiscard]] const std::map<std::string, RegistryEndpoint>& registries() const noexcept { return registries_; }
    [[nodiscard]] const std::map<std::string, std::string>& scope_routes() const noexcept { return scope_routes_; }

private:
    std::string default_registry_{"public"};
    std::map<std::string, RegistryEndpoint> registries_;
    std::map<std::string, std::string> scope_routes_; // e.g. "@company" -> "internal"
    void seed_defaults();
};

class MultiRegistryClient {
public:
    explicit MultiRegistryClient(MultiRegistryConfig config = MultiRegistryConfig{});

    [[nodiscard]] core::Result<std::vector<PackageInfo>> search(
        std::string_view query,
        std::string_view registry_alias = ""
    );

    [[nodiscard]] core::Result<PackageInfo> get_package(std::string_view package_name);
    [[nodiscard]] core::Result<PackageVersionInfo> get_version_info(std::string_view package_name, const SemVersion& version);
    [[nodiscard]] core::Result<std::vector<uint8_t>> download_archive(std::string_view package_name, const SemVersion& version);

    [[nodiscard]] core::Status publish_package(
        const PackageManifest& manifest,
        const std::vector<uint8_t>& archive_data,
        std::string_view target_registry_alias = ""
    );

    void set_config(MultiRegistryConfig config) noexcept;
    [[nodiscard]] MultiRegistryConfig& config() noexcept { return config_; }
    [[nodiscard]] const MultiRegistryConfig& config() const noexcept { return config_; }

private:
    MultiRegistryConfig config_;
    [[nodiscard]] core::Result<const RegistryEndpoint*> get_authorized_endpoint(std::string_view package_name) const;
};

} // namespace pingdart::pkg
