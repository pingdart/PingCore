// src/pkg/package_manager.cpp
#include <pingdart/pkg/package_manager.h>
#include <pingdart/pkg/archive.h>
#include <pingdart/pkg/security_scanner.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace pingdart::pkg {

PackageManager::PackageManager(
    std::filesystem::path project_root,
    std::shared_ptr<RegistryClient> registry)
    : project_root_(std::move(project_root)),
      registry_(registry ? std::move(registry) : std::make_shared<RegistryClient>()) {}

core::Status PackageManager::add(const std::string& package_name, std::string_view version_req) {
    std::filesystem::path manifest_path = project_root_ / "pingdart.toml";
    PackageManifest manifest;

    if (std::filesystem::exists(manifest_path)) {
        auto m_res = PackageManifest::load_from_file(manifest_path);
        if (!m_res.has_value()) return m_res.status();
        manifest = std::move(m_res).value();
    } else {
        manifest.metadata().name = "my_pingdart_app";
        manifest.metadata().version = SemVersion{1, 0, 0, ""};
    }

    std::string final_req(version_req);
    if (final_req.empty()) {
        auto pkg_res = registry_->get_package(package_name);
        if (pkg_res.has_value()) {
            final_req = "^" + pkg_res.value().latest_version.to_string();
        } else {
            final_req = "^1.0.0";
        }
    }

    manifest.add_dependency(package_name, final_req);
    auto save_status = manifest.save_to_file(manifest_path);
    if (!save_status.ok()) return save_status;

    return sync_dependencies(manifest, false);
}

core::Status PackageManager::remove(const std::string& package_name) {
    std::filesystem::path manifest_path = project_root_ / "pingdart.toml";
    if (!std::filesystem::exists(manifest_path)) {
        return core::Status(core::StatusCode::NotFound, "No pingdart.toml found in " + project_root_.string());
    }

    auto m_res = PackageManifest::load_from_file(manifest_path);
    if (!m_res.has_value()) return m_res.status();
    auto manifest = std::move(m_res).value();

    if (!manifest.remove_dependency(package_name)) {
        return core::Status(core::StatusCode::NotFound, "Package " + package_name + " not found in dependencies");
    }

    auto save_status = manifest.save_to_file(manifest_path);
    if (!save_status.ok()) return save_status;

    // Delete local package directory if exists
    std::error_code ec;
    std::filesystem::remove_all(project_root_ / ".pingdart" / "packages" / package_name, ec);

    return sync_dependencies(manifest, false);
}

core::Result<std::vector<LockedPackage>> PackageManager::list() {
    std::filesystem::path lock_path = project_root_ / "pingdart.lock";
    if (!std::filesystem::exists(lock_path)) {
        return std::vector<LockedPackage>{};
    }
    auto l_res = Lockfile::load_from_file(lock_path);
    if (!l_res.has_value()) return l_res.status();
    return l_res.value().packages();
}

core::Result<std::vector<PackageInfo>> PackageManager::search(std::string_view query) {
    return registry_->search(query);
}

core::Status PackageManager::update() {
    std::filesystem::path manifest_path = project_root_ / "pingdart.toml";
    if (!std::filesystem::exists(manifest_path)) {
        return core::Status(core::StatusCode::NotFound, "No pingdart.toml found");
    }
    auto m_res = PackageManifest::load_from_file(manifest_path);
    if (!m_res.has_value()) return m_res.status();

    return sync_dependencies(m_res.value(), true);
}

core::Result<std::vector<OutdatedPackageInfo>> PackageManager::outdated() {
    std::filesystem::path lock_path = project_root_ / "pingdart.lock";
    if (!std::filesystem::exists(lock_path)) {
        return std::vector<OutdatedPackageInfo>{};
    }

    auto l_res = Lockfile::load_from_file(lock_path);
    if (!l_res.has_value()) return l_res.status();

    std::vector<OutdatedPackageInfo> results;
    for (const auto& pkg : l_res.value().packages()) {
        auto p_res = registry_->get_package(pkg.name);
        if (p_res.has_value()) {
            const auto& latest = p_res.value().latest_version;
            results.push_back(OutdatedPackageInfo{
                .name = pkg.name,
                .current_version = pkg.version,
                .latest_version = latest,
                .has_update = (latest > pkg.version)
            });
        }
    }
    return results;
}

core::Result<AuditReport> PackageManager::audit() {
    AuditReport report;
    std::filesystem::path lock_path = project_root_ / "pingdart.lock";
    if (!std::filesystem::exists(lock_path)) {
        return report;
    }

    auto l_res = Lockfile::load_from_file(lock_path);
    if (!l_res.has_value()) return l_res.status();

    const auto& locked_packages = l_res.value().packages();
    report.packages_scanned = locked_packages.size();

    auto adv_res = registry_->check_advisories();
    if (!adv_res.has_value()) return adv_res.status();

    for (const auto& pkg : locked_packages) {
        for (const auto& adv : adv_res.value()) {
            if (adv.package_name == pkg.name && adv.affected_constraint.is_satisfied_by(pkg.version)) {
                report.detected_vulnerabilities.push_back(adv);
            }
        }
    }

    return report;
}

core::Status PackageManager::login(std::string_view token) {
    return RegistryClient::save_auth_token(token);
}

core::Status PackageManager::publish() {
    std::filesystem::path manifest_path = project_root_ / "pingdart.toml";
    if (!std::filesystem::exists(manifest_path)) {
        return core::Status(core::StatusCode::NotFound, "No pingdart.toml found to publish");
    }

    auto m_res = PackageManifest::load_from_file(manifest_path);
    if (!m_res.has_value()) return m_res.status();

    auto token_res = RegistryClient::load_auth_token();
    if (!token_res.has_value()) {
        return core::Status(core::StatusCode::PermissionDenied, "No auth token found. Run 'pd login' first");
    }

    auto pack_res = ArchiveUtil::pack_package_directory(project_root_);
    if (!pack_res.has_value()) return pack_res.status();

    // PKG-109: Pre-flight client security scan before network submission
    SecurityScanner scanner;
    auto scan = scanner.scan_archive_payload(pack_res.value(), &m_res.value());
    if (!scan.is_clean) {
        return core::Status(core::StatusCode::PermissionDenied,
                            "Pre-flight security scan failed: " + scan.findings.front().description);
    }

    return registry_->publish_package(m_res.value(), pack_res.value(), token_res.value());
}

core::Status PackageManager::sync_dependencies(PackageManifest& manifest, bool force_latest) {
    std::filesystem::path lock_path = project_root_ / "pingdart.lock";
    Lockfile existing_lock;
    bool has_lock = false;
    if (!force_latest && std::filesystem::exists(lock_path)) {
        auto l_res = Lockfile::load_from_file(lock_path);
        if (l_res.has_value()) {
            existing_lock = std::move(l_res).value();
            has_lock = true;
        }
    }

    auto resolve_res = resolver_.resolve(manifest, *registry_, has_lock ? &existing_lock : nullptr, force_latest);
    if (!resolve_res.has_value()) {
        return resolve_res.status();
    }

    std::filesystem::path packages_dir = project_root_ / ".pingdart" / "packages";
    std::error_code ec;
    std::filesystem::create_directories(packages_dir, ec);

    Lockfile new_lock;
    for (const auto& resolved : resolve_res.value().packages) {
        // Download package source archive
        auto dl_res = registry_->download_archive(resolved.name, resolved.version);
        if (dl_res.has_value()) {
            std::filesystem::path target_pkg_dir = packages_dir / resolved.name;
            (void)ArchiveUtil::unpack_package_archive(dl_res.value(), target_pkg_dir);
        }

        LockedPackage lpkg{
            .name = resolved.name,
            .version = resolved.version,
            .source = resolved.source,
            .checksum = resolved.checksum,
            .dependencies = resolved.dependencies
        };
        new_lock.add_or_update_package(std::move(lpkg));
    }

    auto lock_save = new_lock.save_to_file(lock_path);
    if (!lock_save.ok()) return lock_save;

    std::filesystem::path cmake_path = project_root_ / ".pingdart" / "pingdart_packages.cmake";
    return generate_cmake_include(new_lock, cmake_path);
}

core::Status PackageManager::generate_cmake_include(
    const Lockfile& lockfile,
    const std::filesystem::path& out_path) const
{
    std::error_code ec;
    std::filesystem::create_directories(out_path.parent_path(), ec);

    std::ofstream out(out_path);
    if (!out.is_open()) {
        return core::Status(core::StatusCode::Internal, "Failed to create cmake packages file");
    }

    out << "# Automatically generated by PingDart package manager ('pd')\n";
    out << "# Include this in your CMakeLists.txt via: include(.pingdart/pingdart_packages.cmake)\n\n";
    out << "set(PINGDART_PACKAGES_DIR \"${CMAKE_CURRENT_LIST_DIR}/packages\")\n\n";

    std::vector<std::string> target_names;

    for (const auto& pkg : lockfile.packages()) {
        std::string safe_name = pkg.name;
        if (safe_name.starts_with("pingdart-") || safe_name.starts_with("pingdart_")) {
            safe_name = safe_name.substr(9);
        }
        std::replace(safe_name.begin(), safe_name.end(), '-', '_');
        std::string target_name = "pingdart_pkg_" + safe_name;
        target_names.push_back(target_name);

        out << "# Package: " << pkg.name << " (" << pkg.version.to_string() << ")\n";
        out << "if(EXISTS \"${PINGDART_PACKAGES_DIR}/" << pkg.name << "/include\")\n";
        out << "    add_library(" << target_name << " INTERFACE)\n";
        out << "    target_include_directories(" << target_name << " INTERFACE \"${PINGDART_PACKAGES_DIR}/" << pkg.name << "/include\")\n";
        out << "endif()\n\n";
    }

    out << "# Umbrella Interface Target\n";
    out << "add_library(pingdart_packages INTERFACE)\n";
    if (!target_names.empty()) {
        out << "target_link_libraries(pingdart_packages INTERFACE\n";
        for (const auto& t : target_names) {
            out << "    " << t << "\n";
        }
        out << ")\n";
    }

    return core::Status{};
}

core::Status PackageManager::create_package_scaffold(
    const std::string& package_name,
    const std::filesystem::path& destination_dir)
{
    std::error_code ec;
    std::filesystem::create_directories(destination_dir, ec);

    std::string safe_name = package_name;
    std::replace(safe_name.begin(), safe_name.end(), '-', '_');

    // 1. pingdart.toml
    PackageManifest manifest;
    manifest.metadata().name = package_name;
    manifest.metadata().version = SemVersion{0, 1, 0, ""};
    manifest.metadata().description = "A new PingDart library package";
    manifest.metadata().license = "MIT";
    manifest.metadata().edition = "2026";
    manifest.add_dependency("pingdart-core", "^1.0.0");
    (void)manifest.save_to_file(destination_dir / "pingdart.toml");

    // 2. README.md
    std::ofstream readme(destination_dir / "README.md");
    readme << "# " << package_name << "\n\n";
    readme << "A reusable PingDart framework library.\n\n";
    readme << "## Installation\n\n```bash\npd add " << package_name << "\n```\n";

    // 3. LICENSE
    std::ofstream license(destination_dir / "LICENSE");
    license << "MIT License\n\nCopyright (c) 2026 PingDart Authors\n";

    // 4. CHANGELOG.md
    std::ofstream changelog(destination_dir / "CHANGELOG.md");
    changelog << "# Changelog\n\n## 0.1.0\n- Initial release.\n";

    // 5. include/<safe_name>/<safe_name>.h
    std::filesystem::path inc_dir = destination_dir / "include" / safe_name;
    std::filesystem::create_directories(inc_dir, ec);
    std::ofstream header(inc_dir / (safe_name + ".h"));
    header << "#pragma once\n\nnamespace " << safe_name << " {\n\nvoid init();\n\n} // namespace " << safe_name << "\n";

    // 6. src/<safe_name>.cpp
    std::filesystem::path src_dir = destination_dir / "src";
    std::filesystem::create_directories(src_dir, ec);
    std::ofstream cpp_src(src_dir / (safe_name + ".cpp"));
    cpp_src << "#include <" << safe_name << "/" << safe_name << ".h>\n#include <iostream>\n\n";
    cpp_src << "namespace " << safe_name << " {\n\nvoid init() {\n    std::cout << \"" << package_name << " initialized!\\n\";\n}\n\n}\n";

    // 7. tests/<safe_name>_test.cpp
    std::filesystem::path test_dir = destination_dir / "tests";
    std::filesystem::create_directories(test_dir, ec);
    std::ofstream test_src(test_dir / (safe_name + "_test.cpp"));
    test_src << "#include <" << safe_name << "/" << safe_name << ".h>\n#include <cassert>\n\nint main() {\n    " << safe_name << "::init();\n    return 0;\n}\n";

    return core::Status{};
}

} // namespace pingdart::pkg
