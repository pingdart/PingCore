// include/pingdart/pkg/registry_server.h
// PKG-108: Public Registry API and Developer Portal
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <pingdart/core/result.h>
#include <pingdart/pkg/semver.h>
#include <pingdart/pkg/manifest.h>
#include <pingdart/pkg/registry_client.h>
#include <pingdart/pkg/security_scanner.h>
#include <pingdart/pkg/database.h>
#include <pingdart/pkg/object_storage.h>

namespace pingdart::pkg {

// ============================================================================
// HTTP Primitives for Registry REST API & Web Developer Portal
// ============================================================================

enum class HttpMethod {
    Get,
    Post,
    Put,
    Delete,
    Head,
    Options
};

[[nodiscard]] std::string http_method_to_string(HttpMethod method);
[[nodiscard]] HttpMethod string_to_http_method(std::string_view str);

struct HttpRequest {
    HttpMethod method{HttpMethod::Get};
    std::string path{"/"};
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    std::vector<uint8_t> body;

    [[nodiscard]] std::string get_header(std::string_view name) const;
    [[nodiscard]] std::string get_query(std::string_view key, std::string_view default_val = "") const;
    [[nodiscard]] std::string body_as_string() const;
};

struct HttpResponse {
    int status_code{200};
    std::string status_message{"OK"};
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;

    [[nodiscard]] std::string body_as_string() const;

    static HttpResponse json(int status, std::string_view json_str);
    static HttpResponse html(int status, std::string_view html_str);
    static HttpResponse binary(int status, std::string_view mime_type, std::vector<uint8_t> data);
    static HttpResponse error(int status, std::string_view message);
};

// ============================================================================
// PKG-108: Developer Portal HTML UI Renderer
// ============================================================================

class DeveloperPortalRenderer {
public:
    [[nodiscard]] static std::string render_homepage(
        const std::vector<PackageInfo>& featured_packages,
        size_t total_packages,
        size_t total_publishers,
        size_t total_downloads
    );

    [[nodiscard]] static std::string render_package_page(
        const PackageInfo& pkg,
        const std::vector<std::string>& owners,
        bool is_verified,
        const std::vector<ExtendedSecurityAdvisory>& advisories
    );

    [[nodiscard]] static std::string render_search_results_page(
        std::string_view query,
        const std::vector<PackageInfo>& matches
    );

    [[nodiscard]] static std::string render_publisher_page(
        std::string_view username,
        bool is_verified,
        const std::vector<PackageInfo>& packages
    );
};

// ============================================================================
// PKG-108: Public Registry API Engine & Service Host
// ============================================================================

class RegistryServer final : public IRegistryBackend {
public:
    RegistryServer();

    // REST / HTTP Request Dispatcher
    [[nodiscard]] HttpResponse handle_request(const HttpRequest& request);

    // Direct IRegistryBackend Implementation (for in-process client use)
    core::Result<std::vector<PackageInfo>> search(std::string_view query) override;
    core::Result<PackageInfo> get_package(std::string_view name) override;
    core::Result<PackageVersionInfo> get_version_info(std::string_view name, const SemVersion& version) override;
    core::Result<std::vector<uint8_t>> download_archive(std::string_view name, const SemVersion& version) override;
    core::Status publish_package(const PackageManifest& manifest, const std::vector<uint8_t>& archive_data, std::string_view auth_token) override;
    core::Result<std::vector<SecurityAdvisory>> get_security_advisories() override;

    // Subsystem accessors
    [[nodiscard]] PublisherSecurity& publisher_security() noexcept { return publisher_security_; }
    [[nodiscard]] AdvisoryDatabase& advisory_database() noexcept { return advisory_db_; }
    [[nodiscard]] SecurityScanner& security_scanner() noexcept { return scanner_; }

    // Metrics & Statistics
    [[nodiscard]] size_t total_packages() const noexcept;
    [[nodiscard]] uint64_t total_downloads() const noexcept;

    // PKG-111E: Server-side Private Registry & Scoped Package Access Control
    void set_private_registry(bool is_private) noexcept { is_private_registry_ = is_private; }
    [[nodiscard]] bool is_private_registry() const noexcept { return is_private_registry_; }

    void set_package_visibility(std::string_view pkg_name, bool is_private) {
        package_privacy_[std::string(pkg_name)] = is_private;
    }
    [[nodiscard]] bool is_package_private(std::string_view pkg_name) const {
        if (is_private_registry_) return true;
        if (pkg_name.starts_with("@")) return true;
        auto it = package_privacy_.find(std::string(pkg_name));
        return it != package_privacy_.end() && it->second;
    }

    // PKG-111C/D: Relational Database & Object Storage Integration
    void set_persistent_database(std::shared_ptr<IPersistentPackageDatabase> db) {
        db_ = db;
        publisher_security_.set_database(std::move(db));
    }
    [[nodiscard]] std::shared_ptr<IPersistentPackageDatabase> persistent_database() const noexcept { return db_; }

    void set_object_storage(std::shared_ptr<IObjectStorage> storage) { storage_ = std::move(storage); }
    [[nodiscard]] std::shared_ptr<IObjectStorage> object_storage() const noexcept { return storage_; }

private:
    std::map<std::string, PackageInfo> packages_;
    PublisherSecurity publisher_security_;
    AdvisoryDatabase advisory_db_;
    SecurityScanner scanner_;
    uint64_t total_downloads_{125430};

    bool is_private_registry_{false};
    std::map<std::string, bool> package_privacy_;
    std::shared_ptr<IPersistentPackageDatabase> db_;
    std::shared_ptr<IObjectStorage> storage_;

    void seed_ecosystem();

    [[nodiscard]] bool check_read_authorization(const HttpRequest& req, std::string_view pkg_name) const;

    // Internal Route Handlers
    HttpResponse handle_api_packages(const HttpRequest& req);
    HttpResponse handle_api_package_detail(const HttpRequest& req, std::string_view pkg_name);
    HttpResponse handle_api_version_detail(const HttpRequest& req, std::string_view pkg_name, std::string_view version_str);
    HttpResponse handle_api_download(const HttpRequest& req, std::string_view pkg_name, std::string_view version_str);
    HttpResponse handle_api_publish(const HttpRequest& req);
    HttpResponse handle_api_auth_token(const HttpRequest& req);
    HttpResponse handle_api_advisories();
    HttpResponse handle_api_health();

    // Portal Route Handlers
    HttpResponse handle_portal_home();
    HttpResponse handle_portal_package(std::string_view pkg_name);
    HttpResponse handle_portal_search(const HttpRequest& req);
    HttpResponse handle_portal_publisher(std::string_view username);
};

} // namespace pingdart::pkg
