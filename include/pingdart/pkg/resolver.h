// include/pingdart/pkg/resolver.h
#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <pingdart/core/result.h>
#include <pingdart/pkg/manifest.h>
#include <pingdart/pkg/lockfile.h>
#include <pingdart/pkg/registry_client.h>

namespace pingdart::pkg {

struct ResolvedPackage {
    std::string name;
    SemVersion version;
    std::string source{"registry+https://registry.pingdart.com"};
    std::string checksum;
    std::vector<std::string> dependencies;
};

struct ResolutionResult {
    std::vector<ResolvedPackage> packages;
    std::vector<std::string> warnings;
};

class DependencyResolver {
public:
    DependencyResolver() = default;

    [[nodiscard]] core::Result<ResolutionResult> resolve(
        const PackageManifest& root_manifest,
        RegistryClient& registry,
        const Lockfile* existing_lockfile = nullptr,
        bool force_latest = false
    );

private:
    struct ResolutionNode {
        std::string name;
        SemVersion version;
        std::string checksum;
        std::map<std::string, std::string> deps;
    };

    core::Status resolve_recursive(
        const std::string& pkg_name,
        const VersionConstraint& constraint,
        RegistryClient& registry,
        const Lockfile* lockfile,
        bool force_latest,
        std::map<std::string, ResolutionNode>& resolved,
        std::set<std::string>& visit_path
    );
};

} // namespace pingdart::pkg
