// src/pkg/resolver.cpp
#include <pingdart/pkg/resolver.h>
#include <algorithm>

namespace pingdart::pkg {

core::Result<ResolutionResult> DependencyResolver::resolve(
    const PackageManifest& root_manifest,
    RegistryClient& registry,
    const Lockfile* existing_lockfile,
    bool force_latest)
{
    std::map<std::string, ResolutionNode> resolved;
    std::set<std::string> visit_path;
    ResolutionResult result;

    for (const auto& [dep_name, dep] : root_manifest.dependencies()) {
        auto status = resolve_recursive(dep_name, dep.constraint, registry,
                                        existing_lockfile, force_latest,
                                        resolved, visit_path);
        if (!status.ok()) {
            return status;
        }
    }

    for (const auto& [name, node] : resolved) {
        ResolvedPackage rpkg{
            .name = node.name,
            .version = node.version,
            .source = "registry+https://registry.pingdart.com",
            .checksum = node.checksum,
            .dependencies = {}
        };
        for (const auto& [child_name, child_req] : node.deps) {
            rpkg.dependencies.push_back(child_name);
        }
        std::sort(rpkg.dependencies.begin(), rpkg.dependencies.end());
        result.packages.push_back(std::move(rpkg));
    }

    std::sort(result.packages.begin(), result.packages.end(), [](const ResolvedPackage& a, const ResolvedPackage& b) {
        return a.name < b.name;
    });

    return result;
}

core::Status DependencyResolver::resolve_recursive(
    const std::string& pkg_name,
    const VersionConstraint& constraint,
    RegistryClient& registry,
    const Lockfile* lockfile,
    bool force_latest,
    std::map<std::string, ResolutionNode>& resolved,
    std::set<std::string>& visit_path)
{
    // Ignore pingdart-core as it is built-in framework foundation
    if (pkg_name == "pingdart-core" || pkg_name == "pingdart_core") {
        return core::Status{};
    }

    if (visit_path.count(pkg_name)) {
        return core::Status(core::StatusCode::Internal, "Circular dependency cycle detected at: " + pkg_name);
    }

    // If package already resolved by another branch in graph
    auto it = resolved.find(pkg_name);
    if (it != resolved.end()) {
        if (!constraint.is_satisfied_by(it->second.version)) {
            return core::Status(core::StatusCode::FailedPrecondition,
                                "Dependency conflict on " + pkg_name + ": already resolved to " +
                                it->second.version.to_string() + " which does not satisfy " + constraint.to_string());
        }
        return core::Status{};
    }

    // Try conservative lockfile selection first if allowed
    const LockedPackage* locked = (!force_latest && lockfile) ? lockfile->find_package(pkg_name) : nullptr;
    if (locked && constraint.is_satisfied_by(locked->version)) {
        auto v_info_res = registry.get_version_info(pkg_name, locked->version);
        if (v_info_res.has_value()) {
            ResolutionNode node{
                .name = pkg_name,
                .version = locked->version,
                .checksum = locked->checksum.empty() ? v_info_res.value().checksum : locked->checksum,
                .deps = v_info_res.value().dependencies
            };
            resolved[pkg_name] = node;

            visit_path.insert(pkg_name);
            for (const auto& [c_name, c_req] : node.deps) {
                auto c_constraint_res = VersionConstraint::parse(c_req);
                VersionConstraint c_cons = c_constraint_res.has_value() ? c_constraint_res.value() : VersionConstraint{};
                auto s = resolve_recursive(c_name, c_cons, registry, lockfile, force_latest, resolved, visit_path);
                if (!s.ok()) return s;
            }
            visit_path.erase(pkg_name);
            return core::Status{};
        }
    }

    // Query registry for all published versions of package
    auto pkg_info_res = registry.get_package(pkg_name);
    if (!pkg_info_res.has_value()) {
        return core::Status(core::StatusCode::NotFound, "Package not found in registry: " + pkg_name);
    }

    const auto& pkg_info = pkg_info_res.value();
    std::vector<PackageVersionInfo> matching;
    for (const auto& v : pkg_info.versions) {
        if (constraint.is_satisfied_by(v.version)) {
            matching.push_back(v);
        }
    }

    if (matching.empty()) {
        return core::Status(core::StatusCode::NotFound,
                            "No matching version found for " + pkg_name + " satisfying " + constraint.to_string());
    }

    // Pick highest SemVer
    std::sort(matching.begin(), matching.end(), [](const PackageVersionInfo& a, const PackageVersionInfo& b) {
        return a.version > b.version;
    });

    const auto& selected = matching.front();
    ResolutionNode node{
        .name = pkg_name,
        .version = selected.version,
        .checksum = selected.checksum,
        .deps = selected.dependencies
    };
    resolved[pkg_name] = node;

    visit_path.insert(pkg_name);
    for (const auto& [c_name, c_req] : node.deps) {
        auto c_constraint_res = VersionConstraint::parse(c_req);
        VersionConstraint c_cons = c_constraint_res.has_value() ? c_constraint_res.value() : VersionConstraint{};
        auto s = resolve_recursive(c_name, c_cons, registry, lockfile, force_latest, resolved, visit_path);
        if (!s.ok()) return s;
    }
    visit_path.erase(pkg_name);

    return core::Status{};
}

} // namespace pingdart::pkg
