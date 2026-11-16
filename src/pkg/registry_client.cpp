// src/pkg/registry_client.cpp
#include <pingdart/pkg/registry_client.h>
#include <pingdart/pkg/archive.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace pingdart::pkg {

RegistryClient::RegistryClient(std::shared_ptr<IRegistryBackend> backend)
    : backend_(backend ? std::move(backend) : std::make_shared<DefaultRegistryBackend>()) {}

core::Result<std::vector<PackageInfo>> RegistryClient::search(std::string_view query) {
    return backend_->search(query);
}

core::Result<PackageInfo> RegistryClient::get_package(std::string_view name) {
    return backend_->get_package(name);
}

core::Result<PackageVersionInfo> RegistryClient::get_version_info(std::string_view name, const SemVersion& version) {
    return backend_->get_version_info(name, version);
}

core::Result<std::vector<uint8_t>> RegistryClient::download_archive(std::string_view name, const SemVersion& version) {
    return backend_->download_archive(name, version);
}

core::Status RegistryClient::publish_package(
    const PackageManifest& manifest,
    const std::vector<uint8_t>& archive_data,
    std::string_view auth_token)
{
    return backend_->publish_package(manifest, archive_data, auth_token);
}

core::Result<std::vector<SecurityAdvisory>> RegistryClient::check_advisories() {
    return backend_->get_security_advisories();
}

void RegistryClient::set_backend(std::shared_ptr<IRegistryBackend> backend) noexcept {
    backend_ = std::move(backend);
}

IRegistryBackend* RegistryClient::backend() const noexcept {
    return backend_.get();
}

core::Status RegistryClient::save_auth_token(std::string_view token, const std::string& registry_url) {
    const char* home = std::getenv("HOME");
    if (!home) return core::Status(core::StatusCode::Internal, "HOME directory not found");

    std::filesystem::path config_dir = std::filesystem::path(home) / ".pingdart";
    std::error_code ec;
    std::filesystem::create_directories(config_dir, ec);

    std::filesystem::path cred_file = config_dir / "credentials.toml";
    std::ofstream out(cred_file);
    if (!out.is_open()) {
        return core::Status(core::StatusCode::Internal, "Failed to open credentials file for writing");
    }

    out << "[registry]\n";
    out << "url = \"" << registry_url << "\"\n";
    out << "token = \"" << token << "\"\n";
    return core::Status{};
}

core::Result<std::string> RegistryClient::load_auth_token(const std::string& /*registry_url*/) {
    const char* home = std::getenv("HOME");
    if (!home) return core::Status(core::StatusCode::NotFound, "HOME directory not found");

    std::filesystem::path cred_file = std::filesystem::path(home) / ".pingdart" / "credentials.toml";
    std::ifstream in(cred_file);
    if (!in.is_open()) {
        return core::Status(core::StatusCode::NotFound, "No credentials file found. Please run 'pd login'");
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.starts_with("token =")) {
            size_t q1 = line.find('"');
            size_t q2 = line.rfind('"');
            if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1) {
                return line.substr(q1 + 1, q2 - q1 - 1);
            }
        }
    }
    return core::Status(core::StatusCode::NotFound, "Token not found in credentials.toml");
}

// --- DefaultRegistryBackend Implementation ---

DefaultRegistryBackend::DefaultRegistryBackend() {
    seed_default_packages();
}

void DefaultRegistryBackend::seed_default_packages() {
    // 1. pingdart-charts
    {
        PackageInfo charts_pkg{
            .name = "pingdart-charts",
            .latest_version = SemVersion{1, 2, 0, ""},
            .description = "Cross-platform charting and visualization widgets",
            .license = "MIT",
            .versions = {}
        };

        PackageVersionInfo v120{
            .version = SemVersion{1, 2, 0, ""},
            .description = "Cross-platform chart widgets with animations",
            .checksum = "sha256:7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926",
            .dependencies = {{"pingdart-core", "^1.0.0"}},
            .platforms = PlatformMatrix{},
            .archive_payload = {}
        };

        PackageVersionInfo v110{
            .version = SemVersion{1, 1, 0, ""},
            .description = "Line and Bar charts",
            .checksum = "sha256:109501089b3b44bde47537651a5118c7c25c6dbd9f36f87ea9372957973d8b9e",
            .dependencies = {{"pingdart-core", "^1.0.0"}},
            .platforms = PlatformMatrix{},
            .archive_payload = {}
        };

        charts_pkg.versions.push_back(v110);
        charts_pkg.versions.push_back(v120);
        registry_["pingdart-charts"] = charts_pkg;
    }

    // 2. pingdart-sqlite
    {
        PackageInfo sqlite_pkg{
            .name = "pingdart-sqlite",
            .latest_version = SemVersion{1, 0, 0, ""},
            .description = "High-performance embedded SQLite database client",
            .license = "Apache-2.0",
            .versions = {}
        };

        PackageVersionInfo v100{
            .version = SemVersion{1, 0, 0, ""},
            .description = "Stable SQLite ORM & Query Builder",
            .checksum = "sha256:9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08",
            .dependencies = {},
            .platforms = PlatformMatrix{},
            .archive_payload = {}
        };

        PackageVersionInfo v090{
            .version = SemVersion{0, 9, 0, ""},
            .description = "Beta SQLite driver",
            .checksum = "sha256:5cb75efbb9ad2d8dfd50a26e85cbccba1c9d9d28dbd60cf4e0a779dd5843a0e1",
            .dependencies = {},
            .platforms = PlatformMatrix{},
            .archive_payload = {}
        };

        sqlite_pkg.versions.push_back(v090);
        sqlite_pkg.versions.push_back(v100);
        registry_["pingdart-sqlite"] = sqlite_pkg;
    }

    // 3. pingdart-camera
    {
        PackageInfo cam_pkg{
            .name = "pingdart-camera",
            .latest_version = SemVersion{1, 0, 0, ""},
            .description = "Cross-platform native camera capture interface",
            .license = "MIT",
            .versions = {}
        };

        PackageVersionInfo v100{
            .version = SemVersion{1, 0, 0, ""},
            .description = "Hardware-accelerated camera preview & photo capture",
            .checksum = "sha256:3a7bd3e2360a3d29eea436fcfb7e44c735d117c42d1c1835420b6b9942dd4f1b",
            .dependencies = {},
            .platforms = PlatformMatrix{},
            .archive_payload = {}
        };

        cam_pkg.versions.push_back(v100);
        registry_["pingdart-camera"] = cam_pkg;
    }

    // 4. Sample Security Advisory for pd audit
    {
        SecurityAdvisory adv{
            .advisory_id = "PINGDART-2026-0001",
            .package_name = "pingdart-sqlite",
            .affected_constraint = VersionConstraint(ConstraintOp::LessThan, SemVersion{1, 0, 0, ""}),
            .patched_version = "1.0.0",
            .severity = "High",
            .description = "SQL injection vulnerability in legacy parameter binding in sqlite versions < 1.0.0"
        };
        advisories_.push_back(adv);
    }
}

core::Result<std::vector<PackageInfo>> DefaultRegistryBackend::search(std::string_view query) {
    std::vector<PackageInfo> results;
    std::string q(query);
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    for (const auto& [name, info] : registry_) {
        std::string n = name;
        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        std::string d = info.description;
        std::transform(d.begin(), d.end(), d.begin(), ::tolower);

        if (q.empty() || n.find(q) != std::string::npos || d.find(q) != std::string::npos) {
            results.push_back(info);
        }
    }
    return results;
}

core::Result<PackageInfo> DefaultRegistryBackend::get_package(std::string_view name) {
    auto it = registry_.find(std::string(name));
    if (it == registry_.end()) {
        return core::Status(core::StatusCode::NotFound, "Package not found in registry: " + std::string(name));
    }
    return it->second;
}

core::Result<PackageVersionInfo> DefaultRegistryBackend::get_version_info(
    std::string_view name,
    const SemVersion& version)
{
    auto pkg_res = get_package(name);
    if (!pkg_res.has_value()) return pkg_res.status();

    for (const auto& v_info : pkg_res.value().versions) {
        if (v_info.version == version) {
            return v_info;
        }
    }
    return core::Status(core::StatusCode::NotFound,
                        "Version " + version.to_string() + " not found for package " + std::string(name));
}

core::Result<std::vector<uint8_t>> DefaultRegistryBackend::download_archive(
    std::string_view name,
    const SemVersion& version)
{
    auto v_res = get_version_info(name, version);
    if (!v_res.has_value()) return v_res.status();

    // If payload is populated, return it; otherwise generate a minimal synthetic source package
    if (!v_res.value().archive_payload.empty()) {
        return v_res.value().archive_payload;
    }

    std::string dummy_src = "// " + std::string(name) + " v" + version.to_string() + "\n";
    return std::vector<uint8_t>(dummy_src.begin(), dummy_src.end());
}

core::Status DefaultRegistryBackend::publish_package(
    const PackageManifest& manifest,
    const std::vector<uint8_t>& archive_data,
    std::string_view auth_token)
{
    if (auth_token.empty()) {
        return core::Status(core::StatusCode::PermissionDenied, "Authentication required. Please run 'pd login'");
    }

    const auto& meta = manifest.metadata();
    PackageInfo& info = registry_[meta.name];
    info.name = meta.name;
    info.description = meta.description;
    info.license = meta.license;

    if (info.latest_version < meta.version) {
        info.latest_version = meta.version;
    }

    for (const auto& existing : info.versions) {
        if (existing.version == meta.version) {
            return core::Status(core::StatusCode::AlreadyExists,
                                "Version " + meta.version.to_string() + " already exists. Package versions are immutable.");
        }
    }

    std::string hash = ArchiveUtil::compute_sha256(archive_data);
    PackageVersionInfo v_info{
        .version = meta.version,
        .description = meta.description,
        .checksum = hash,
        .dependencies = {},
        .platforms = manifest.platforms(),
        .archive_payload = archive_data
    };

    for (const auto& [dep_name, dep] : manifest.dependencies()) {
        v_info.dependencies[dep_name] = dep.raw_requirement;
    }

    info.versions.push_back(v_info);
    return core::Status{};
}

core::Result<std::vector<SecurityAdvisory>> DefaultRegistryBackend::get_security_advisories() {
    return advisories_;
}

} // namespace pingdart::pkg
