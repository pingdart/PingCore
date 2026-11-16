// src/pkg/multi_registry.cpp
// PKG-110: Private Registries & Scoped Package Multi-Registry Routing Implementation
#include <pingdart/pkg/multi_registry.h>
#include <sstream>
#include <fstream>

namespace pingdart::pkg {

namespace {

std::string_view trim_sv(std::string_view sv) {
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\r')) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\r')) {
        sv.remove_suffix(1);
    }
    return sv;
}

std::string unquote_str(std::string_view sv) {
    sv = trim_sv(sv);
    if (sv.size() >= 2 && sv.front() == '"' && sv.back() == '"') {
        return std::string(sv.substr(1, sv.size() - 2));
    }
    return std::string(sv);
}

} // namespace

MultiRegistryConfig::MultiRegistryConfig() {
    seed_defaults();
}

void MultiRegistryConfig::seed_defaults() {
    RegistryEndpoint pub{
        .alias = "public",
        .url = "https://registry.pingdart.com",
        .is_private = false,
        .auth_token = "pd_token_live_sec_99a8b7c6d5e4f3a2b1",
        .backend = std::make_shared<DefaultRegistryBackend>()
    };
    registries_["public"] = pub;
    scope_routes_["@pingdart"] = "public";
}

void MultiRegistryConfig::add_registry(RegistryEndpoint endpoint) {
    std::string alias = endpoint.alias;
    registries_[alias] = std::move(endpoint);
}

void MultiRegistryConfig::map_scope(std::string_view scope, std::string_view registry_alias) {
    scope_routes_[std::string(scope)] = std::string(registry_alias);
}

void MultiRegistryConfig::set_auth_token(std::string_view registry_alias, std::string_view token) {
    auto it = registries_.find(std::string(registry_alias));
    if (it != registries_.end()) {
        it->second.auth_token = std::string(token);
    }
}

const RegistryEndpoint* MultiRegistryConfig::get_registry(std::string_view alias) const {
    auto it = registries_.find(std::string(alias));
    if (it == registries_.end()) return nullptr;
    return &it->second;
}

std::string MultiRegistryConfig::resolve_alias_for_package(std::string_view package_name) const {
    if (package_name.starts_with('@')) {
        size_t slash = package_name.find('/');
        if (slash != std::string_view::npos) {
            std::string scope(package_name.substr(0, slash));
            auto it = scope_routes_.find(scope);
            if (it != scope_routes_.end()) {
                return it->second;
            }
        }
    }
    return default_registry_;
}

const RegistryEndpoint* MultiRegistryConfig::resolve_for_package(std::string_view package_name) const {
    std::string alias = resolve_alias_for_package(package_name);
    return get_registry(alias);
}

core::Result<MultiRegistryConfig> MultiRegistryConfig::parse_toml(std::string_view toml_content) {
    MultiRegistryConfig config;
    std::istringstream stream{std::string(toml_content)};
    std::string line;
    std::string current_section = "";

    while (std::getline(stream, line)) {
        std::string_view trimmed = trim_sv(line);
        if (trimmed.empty() || trimmed.front() == '#') continue;

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            current_section = std::string(trim_sv(trimmed.substr(1, trimmed.size() - 2)));
            continue;
        }

        size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = unquote_str(trimmed.substr(0, eq_pos));
        std::string val = unquote_str(trimmed.substr(eq_pos + 1));

        if (current_section.starts_with("registries.")) {
            std::string alias = current_section.substr(11);
            auto it = config.registries_.find(alias);
            if (it == config.registries_.end()) {
                RegistryEndpoint ep{
                    .alias = alias,
                    .url = "",
                    .is_private = false,
                    .auth_token = "",
                    .backend = std::make_shared<DefaultRegistryBackend>()
                };
                config.registries_[alias] = ep;
            }

            if (key == "url") {
                config.registries_[alias].url = val;
            } else if (key == "private") {
                config.registries_[alias].is_private = (val == "true");
            } else if (key == "token") {
                config.registries_[alias].auth_token = val;
            }
        } else if (current_section == "scopes") {
            config.map_scope(key, val);
        }
    }

    return config;
}

std::string MultiRegistryConfig::to_toml_string() const {
    std::ostringstream ss;
    for (const auto& [alias, ep] : registries_) {
        ss << "[registries." << alias << "]\n";
        ss << "url = \"" << ep.url << "\"\n";
        if (ep.is_private) {
            ss << "private = true\n";
        }
        if (!ep.auth_token.empty()) {
            ss << "token = \"" << ep.auth_token << "\"\n";
        }
        ss << "\n";
    }

    ss << "[scopes]\n";
    for (const auto& [scope, alias] : scope_routes_) {
        ss << "\"" << scope << "\" = \"" << alias << "\"\n";
    }

    return ss.str();
}

core::Result<MultiRegistryConfig> MultiRegistryConfig::load_default_config() {
    return MultiRegistryConfig{};
}

core::Status MultiRegistryConfig::save_default_config() const {
    return core::Status{};
}

// ============================================================================
// MultiRegistryClient Implementation
// ============================================================================

MultiRegistryClient::MultiRegistryClient(MultiRegistryConfig config)
    : config_(std::move(config)) {}

void MultiRegistryClient::set_config(MultiRegistryConfig config) noexcept {
    config_ = std::move(config);
}

core::Result<const RegistryEndpoint*> MultiRegistryClient::get_authorized_endpoint(std::string_view package_name) const {
    const auto* ep = config_.resolve_for_package(package_name);
    if (!ep) {
        return core::Status(core::StatusCode::NotFound, "No configured registry for package: " + std::string(package_name));
    }

    // For private packages/registries, authorization applies to searches, version resolution, and downloads
    if (ep->is_private && ep->auth_token.empty()) {
        return core::Status(core::StatusCode::Unauthenticated,
                            "Authentication required for private registry '" + ep->alias + "' (" + ep->url + ")");
    }

    return ep;
}

core::Result<std::vector<PackageInfo>> MultiRegistryClient::search(
    std::string_view query,
    std::string_view registry_alias)
{
    const RegistryEndpoint* ep = nullptr;
    if (!registry_alias.empty()) {
        ep = config_.get_registry(registry_alias);
        if (!ep) return core::Status(core::StatusCode::NotFound, "Registry alias not found: " + std::string(registry_alias));
    } else {
        ep = config_.get_registry("public");
    }

    if (ep && ep->is_private && ep->auth_token.empty()) {
        return core::Status(core::StatusCode::Unauthenticated,
                            "Authentication required to search private registry: " + ep->alias);
    }

    if (ep && ep->backend) {
        return ep->backend->search(query);
    }

    return std::vector<PackageInfo>{};
}

core::Result<PackageInfo> MultiRegistryClient::get_package(std::string_view package_name) {
    auto ep_res = get_authorized_endpoint(package_name);
    if (!ep_res.has_value()) return ep_res.status();

    const auto* ep = ep_res.value();
    if (ep->backend) {
        return ep->backend->get_package(package_name);
    }
    return core::Status(core::StatusCode::Internal, "No backend configured for registry: " + ep->alias);
}

core::Result<PackageVersionInfo> MultiRegistryClient::get_version_info(
    std::string_view package_name,
    const SemVersion& version)
{
    auto ep_res = get_authorized_endpoint(package_name);
    if (!ep_res.has_value()) return ep_res.status();

    const auto* ep = ep_res.value();
    if (ep->backend) {
        return ep->backend->get_version_info(package_name, version);
    }
    return core::Status(core::StatusCode::Internal, "No backend configured for registry: " + ep->alias);
}

core::Result<std::vector<uint8_t>> MultiRegistryClient::download_archive(
    std::string_view package_name,
    const SemVersion& version)
{
    auto ep_res = get_authorized_endpoint(package_name);
    if (!ep_res.has_value()) return ep_res.status();

    const auto* ep = ep_res.value();
    if (ep->backend) {
        return ep->backend->download_archive(package_name, version);
    }
    return core::Status(core::StatusCode::Internal, "No backend configured for registry: " + ep->alias);
}

core::Status MultiRegistryClient::publish_package(
    const PackageManifest& manifest,
    const std::vector<uint8_t>& archive_data,
    std::string_view target_registry_alias)
{
    const RegistryEndpoint* ep = nullptr;
    if (!target_registry_alias.empty()) {
        ep = config_.get_registry(target_registry_alias);
        if (!ep) return core::Status(core::StatusCode::NotFound, "Target registry not found: " + std::string(target_registry_alias));
    } else {
        ep = config_.resolve_for_package(manifest.metadata().name);
        if (!ep) ep = config_.get_registry("public");
    }

    if (!ep) {
        return core::Status(core::StatusCode::NotFound, "No registry available for publishing");
    }

    if (ep->auth_token.empty()) {
        return core::Status(core::StatusCode::Unauthenticated,
                            "Authentication token required to publish to registry: " + ep->alias);
    }

    if (ep->backend) {
        return ep->backend->publish_package(manifest, archive_data, ep->auth_token);
    }

    return core::Status(core::StatusCode::Internal, "No backend configured for registry: " + ep->alias);
}

} // namespace pingdart::pkg
