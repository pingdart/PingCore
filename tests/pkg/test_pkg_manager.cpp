// tests/pkg/test_pkg_manager.cpp
#include <gtest/gtest.h>
#include <pingdart/pkg/semver.h>
#include <pingdart/pkg/manifest.h>
#include <pingdart/pkg/lockfile.h>
#include <pingdart/pkg/archive.h>
#include <pingdart/pkg/resolver.h>
#include <pingdart/pkg/registry_client.h>
#include <pingdart/pkg/package_manager.h>
#include <filesystem>
#include <fstream>

using namespace pingdart::pkg;

// ============================================================================
// PKG-103: SemVer & Version Constraint Tests
// ============================================================================

TEST(PackageManagerGoogleTest, SemVerParsingAndComparisons) {
    auto v1_res = SemVersion::parse("1.2.3");
    ASSERT_TRUE(v1_res.has_value());
    const auto& v1 = v1_res.value();
    EXPECT_EQ(v1.major, 1u);
    EXPECT_EQ(v1.minor, 2u);
    EXPECT_EQ(v1.patch, 3u);
    EXPECT_EQ(v1.to_string(), "1.2.3");

    auto v2_res = SemVersion::parse("v2.0.0-beta.1");
    ASSERT_TRUE(v2_res.has_value());
    const auto& v2 = v2_res.value();
    EXPECT_EQ(v2.major, 2u);
    EXPECT_EQ(v2.prerelease, "beta.1");

    EXPECT_TRUE(v1 < v2);
    EXPECT_TRUE(v2 > v1);
    EXPECT_FALSE(v1 == v2);
}

TEST(PackageManagerGoogleTest, VersionConstraintEvaluation) {
    // Caret constraint: ^1.2.0 matches 1.2.0, 1.3.0, but not 2.0.0 or 1.1.9
    auto c_caret = VersionConstraint::parse("^1.2.0");
    ASSERT_TRUE(c_caret.has_value());
    EXPECT_TRUE(c_caret.value().is_satisfied_by(SemVersion{1, 2, 0, ""}));
    EXPECT_TRUE(c_caret.value().is_satisfied_by(SemVersion{1, 5, 2, ""}));
    EXPECT_FALSE(c_caret.value().is_satisfied_by(SemVersion{1, 1, 9, ""}));
    EXPECT_FALSE(c_caret.value().is_satisfied_by(SemVersion{2, 0, 0, ""}));

    // Tilde constraint: ~1.2.0 matches 1.2.3, but not 1.3.0
    auto c_tilde = VersionConstraint::parse("~1.2.0");
    ASSERT_TRUE(c_tilde.has_value());
    EXPECT_TRUE(c_tilde.value().is_satisfied_by(SemVersion{1, 2, 9, ""}));
    EXPECT_FALSE(c_tilde.value().is_satisfied_by(SemVersion{1, 3, 0, ""}));

    // Range constraint: >=1.0.0
    auto c_ge = VersionConstraint::parse(">=1.0.0");
    ASSERT_TRUE(c_ge.has_value());
    EXPECT_TRUE(c_ge.value().is_satisfied_by(SemVersion{1, 0, 0, ""}));
    EXPECT_TRUE(c_ge.value().is_satisfied_by(SemVersion{3, 0, 0, ""}));
    EXPECT_FALSE(c_ge.value().is_satisfied_by(SemVersion{0, 9, 9, ""}));
}

// ============================================================================
// PKG-101: Manifest Parsing & Project Format Tests
// ============================================================================

TEST(PackageManagerGoogleTest, ManifestParsingAndRoundTrip) {
    std::string toml_sample = R"(
[package]
name = "pingdart-charts"
version = "1.2.0"
description = "Cross-platform chart widgets"
license = "MIT"
edition = "2026"

[dependencies]
pingdart-core = "^1.0.0"

[platforms]
android = true
ios = true
windows = true
macos = true
linux = true
web = true
)";

    auto m_res = PackageManifest::parse(toml_sample);
    ASSERT_TRUE(m_res.has_value());
    const auto& manifest = m_res.value();

    EXPECT_EQ(manifest.metadata().name, "pingdart-charts");
    EXPECT_EQ(manifest.metadata().version.to_string(), "1.2.0");
    EXPECT_EQ(manifest.metadata().license, "MIT");
    EXPECT_EQ(manifest.metadata().edition, "2026");

    EXPECT_TRUE(manifest.platforms().android);
    EXPECT_TRUE(manifest.platforms().ios);
    EXPECT_TRUE(manifest.platforms().macos);

    EXPECT_EQ(manifest.dependencies().size(), 1u);
    ASSERT_TRUE(manifest.dependencies().count("pingdart-core"));
    EXPECT_EQ(manifest.dependencies().at("pingdart-core").raw_requirement, "^1.0.0");

    // Add and remove dependency
    auto modified = manifest;
    modified.add_dependency("pingdart-sqlite", "^1.0.0");
    EXPECT_EQ(modified.dependencies().size(), 2u);
    EXPECT_TRUE(modified.remove_dependency("pingdart-core"));
    EXPECT_EQ(modified.dependencies().size(), 1u);

    std::string serialized = modified.to_toml_string();
    EXPECT_NE(serialized.find("pingdart-sqlite = \"^1.0.0\""), std::string::npos);
}

// ============================================================================
// PKG-103: Lockfile Determinism Tests
// ============================================================================

TEST(PackageManagerGoogleTest, LockfileSerializationAndIntegrity) {
    Lockfile lock;
    LockedPackage p1{
        .name = "pingdart-charts",
        .version = SemVersion{1, 2, 0, ""},
        .source = "registry+https://registry.pingdart.com",
        .checksum = "sha256:7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926",
        .dependencies = {"pingdart-core"}
    };
    lock.add_or_update_package(p1);

    EXPECT_EQ(lock.packages().size(), 1u);
    const auto* found = lock.find_package("pingdart-charts");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->version.to_string(), "1.2.0");

    std::string str = lock.to_string();
    EXPECT_NE(str.find("pingdart-charts"), std::string::npos);
    EXPECT_NE(str.find("sha256:7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926"), std::string::npos);

    auto parsed_res = Lockfile::parse(str);
    ASSERT_TRUE(parsed_res.has_value());
    EXPECT_EQ(parsed_res.value().packages().size(), 1u);
    EXPECT_EQ(parsed_res.value().packages()[0].name, "pingdart-charts");
}

// ============================================================================
// PKG-106: Archive Packing, Unpacking, and Cryptographic SHA-256 Check
// ============================================================================

TEST(PackageManagerGoogleTest, ArchivePackingAndIntegrityVerification) {
    std::filesystem::path test_dir = "build/test_pkg_archive";
    std::error_code ec;
    std::filesystem::remove_all(test_dir, ec);
    std::filesystem::create_directories(test_dir / "include" / "my_lib", ec);
    std::filesystem::create_directories(test_dir / "src", ec);

    // Create test files
    {
        std::ofstream h_out(test_dir / "include" / "my_lib" / "my_lib.h");
        h_out << "#pragma once\nvoid hello();\n";
        std::ofstream cpp_out(test_dir / "src" / "my_lib.cpp");
        cpp_out << "#include <my_lib/my_lib.h>\nvoid hello() {}\n";
    }

    auto pack_res = ArchiveUtil::pack_package_directory(test_dir);
    ASSERT_TRUE(pack_res.has_value());
    const auto& archive_data = pack_res.value();
    EXPECT_FALSE(archive_data.empty());

    // Compute checksum
    std::string hash = ArchiveUtil::compute_sha256(archive_data);
    EXPECT_TRUE(hash.starts_with("sha256:"));

    // Unpack into clean target
    std::filesystem::path dest_dir = "build/test_pkg_unpacked";
    std::filesystem::remove_all(dest_dir, ec);
    auto unpack_res = ArchiveUtil::unpack_package_archive(archive_data, dest_dir);
    EXPECT_TRUE(unpack_res.ok());

    EXPECT_TRUE(std::filesystem::exists(dest_dir / "include" / "my_lib" / "my_lib.h", ec));
    EXPECT_TRUE(std::filesystem::exists(dest_dir / "src" / "my_lib.cpp", ec));
}

// ============================================================================
// PKG-102 & PKG-107: End-to-End Package Manager Operations & CMake Generation
// ============================================================================

TEST(PackageManagerGoogleTest, ScaffoldingAndDependencyInstallationFlow) {
    std::filesystem::path app_dir = "build/test_sandbox_app";
    std::error_code ec;
    std::filesystem::remove_all(app_dir, ec);
    std::filesystem::create_directories(app_dir, ec);

    PackageManager pm(app_dir);

    // 1. Search Registry
    auto search_res = pm.search("charts");
    ASSERT_TRUE(search_res.has_value());
    EXPECT_FALSE(search_res.value().empty());
    EXPECT_EQ(search_res.value()[0].name, "pingdart-charts");

    // 2. Add Package
    auto add_res = pm.add("pingdart-charts", "^1.0.0");
    EXPECT_TRUE(add_res.ok());

    // Verify pingdart.toml updated
    EXPECT_TRUE(std::filesystem::exists(app_dir / "pingdart.toml", ec));
    auto manifest_res = PackageManifest::load_from_file(app_dir / "pingdart.toml");
    ASSERT_TRUE(manifest_res.has_value());
    EXPECT_TRUE(manifest_res.value().dependencies().count("pingdart-charts"));

    // Verify pingdart.lock written
    EXPECT_TRUE(std::filesystem::exists(app_dir / "pingdart.lock", ec));
    auto lock_res = Lockfile::load_from_file(app_dir / "pingdart.lock");
    ASSERT_TRUE(lock_res.has_value());
    EXPECT_EQ(lock_res.value().packages().size(), 1u);
    EXPECT_EQ(lock_res.value().packages()[0].name, "pingdart-charts");
    EXPECT_EQ(lock_res.value().packages()[0].version.to_string(), "1.2.0");

    // Verify CMake include generated
    std::filesystem::path cmake_file = app_dir / ".pingdart" / "pingdart_packages.cmake";
    EXPECT_TRUE(std::filesystem::exists(cmake_file, ec));
    std::ifstream cin(cmake_file);
    std::string cmake_content((std::istreambuf_iterator<char>(cin)), std::istreambuf_iterator<char>());
    EXPECT_NE(cmake_content.find("pingdart_pkg_charts"), std::string::npos);
    EXPECT_NE(cmake_content.find("pingdart_packages"), std::string::npos);

    // 3. Outdated Check
    auto outdated_res = pm.outdated();
    ASSERT_TRUE(outdated_res.has_value());
    EXPECT_EQ(outdated_res.value().size(), 1u);
    EXPECT_FALSE(outdated_res.value()[0].has_update); // 1.2.0 is already latest

    // 4. Security Audit
    auto audit_res = pm.audit();
    ASSERT_TRUE(audit_res.has_value());
    EXPECT_EQ(audit_res.value().packages_scanned, 1u);
    EXPECT_TRUE(audit_res.value().detected_vulnerabilities.empty()); // No vulnerability in charts

    // 5. Remove Package
    auto rm_res = pm.remove("pingdart-charts");
    EXPECT_TRUE(rm_res.ok());
    auto lock_after_rm = Lockfile::load_from_file(app_dir / "pingdart.lock");
    ASSERT_TRUE(lock_after_rm.has_value());
    EXPECT_TRUE(lock_after_rm.value().packages().empty());
}

TEST(PackageManagerGoogleTest, CreateReusableLibraryScaffold) {
    std::filesystem::path lib_dir = "build/test_scaffold_charts";
    std::error_code ec;
    std::filesystem::remove_all(lib_dir, ec);

    auto status = PackageManager::create_package_scaffold("pingdart-charts", lib_dir);
    EXPECT_TRUE(status.ok());

    EXPECT_TRUE(std::filesystem::exists(lib_dir / "pingdart.toml", ec));
    EXPECT_TRUE(std::filesystem::exists(lib_dir / "README.md", ec));
    EXPECT_TRUE(std::filesystem::exists(lib_dir / "LICENSE", ec));
    EXPECT_TRUE(std::filesystem::exists(lib_dir / "CHANGELOG.md", ec));
    EXPECT_TRUE(std::filesystem::exists(lib_dir / "include" / "pingdart_charts" / "pingdart_charts.h", ec));
    EXPECT_TRUE(std::filesystem::exists(lib_dir / "src" / "pingdart_charts.cpp", ec));
    EXPECT_TRUE(std::filesystem::exists(lib_dir / "tests" / "pingdart_charts_test.cpp", ec));
}

// ============================================================================
// PKG-103: SemVer 2.0.0 Pre-release Precedence & Constraints
// ============================================================================

TEST(PackageManagerGoogleTest, SemVerPreReleasePrecedenceAndConstraintFiltering) {
    auto v_alpha = SemVersion::parse("1.0.0-alpha").value();
    auto v_alpha1 = SemVersion::parse("1.0.0-alpha.1").value();
    auto v_beta = SemVersion::parse("1.0.0-beta").value();
    auto v_rc1 = SemVersion::parse("1.0.0-rc.1").value();
    auto v_norm = SemVersion::parse("1.0.0").value();

    EXPECT_TRUE(v_alpha < v_alpha1);
    EXPECT_TRUE(v_alpha1 < v_beta);
    EXPECT_TRUE(v_beta < v_rc1);
    EXPECT_TRUE(v_rc1 < v_norm);
    EXPECT_TRUE(v_norm > v_rc1);

    // Standard constraints must NOT match pre-releases without explicit pre-release tag
    auto c_caret = VersionConstraint::parse("^1.0.0").value();
    EXPECT_FALSE(c_caret.is_satisfied_by(v_rc1));
    EXPECT_TRUE(c_caret.is_satisfied_by(v_norm));

    auto c_any = VersionConstraint::parse("*").value();
    EXPECT_FALSE(c_any.is_satisfied_by(v_rc1));
    EXPECT_TRUE(c_any.is_satisfied_by(v_norm));
}

// ============================================================================
// PKG-103: Transitive Dependencies, Cycle & Conflict Detection
// ============================================================================

class MockGraphRegistryBackend : public IRegistryBackend {
public:
    core::Result<std::vector<PackageInfo>> search(std::string_view) override { return std::vector<PackageInfo>{}; }
    core::Result<PackageInfo> get_package(std::string_view name) override {
        auto it = packages_.find(std::string(name));
        if (it == packages_.end()) return core::Status(core::StatusCode::NotFound, "Not found");
        return it->second;
    }
    core::Result<PackageVersionInfo> get_version_info(std::string_view name, const SemVersion& v) override {
        auto pkg = get_package(name);
        if (!pkg.has_value()) return pkg.status();
        for (const auto& ver : pkg.value().versions) {
            if (ver.version == v) return ver;
        }
        return core::Status(core::StatusCode::NotFound, "Version not found");
    }
    core::Result<std::vector<uint8_t>> download_archive(std::string_view, const SemVersion&) override {
        return std::vector<uint8_t>{};
    }
    core::Status publish_package(const PackageManifest&, const std::vector<uint8_t>&, std::string_view) override {
        return core::Status{};
    }
    core::Result<std::vector<SecurityAdvisory>> get_security_advisories() override {
        return std::vector<SecurityAdvisory>{};
    }

    void add_pkg(const std::string& name, const SemVersion& version, const std::map<std::string, std::string>& deps) {
        PackageVersionInfo v_info{
            .version = version,
            .dependencies = deps
        };
        packages_[name].name = name;
        packages_[name].latest_version = version;
        packages_[name].versions.push_back(v_info);
    }

private:
    std::map<std::string, PackageInfo> packages_;
};

TEST(PackageManagerGoogleTest, TransitiveDependencyResolution) {
    auto backend = std::make_shared<MockGraphRegistryBackend>();
    // A -> B (^1.0.0) -> C (^2.0.0)
    backend->add_pkg("lib-c", SemVersion{2, 1, 0, ""}, {});
    backend->add_pkg("lib-b", SemVersion{1, 2, 0, ""}, {{"lib-c", "^2.0.0"}});
    backend->add_pkg("lib-a", SemVersion{1, 0, 0, ""}, {{"lib-b", "^1.0.0"}});

    RegistryClient client(backend);
    PackageManifest root;
    root.add_dependency("lib-a", "^1.0.0");

    DependencyResolver resolver;
    auto res = resolver.resolve(root, client);
    ASSERT_TRUE(res.has_value());

    const auto& pkgs = res.value().packages;
    EXPECT_EQ(pkgs.size(), 3u); // lib-a, lib-b, and transitive lib-c
    EXPECT_EQ(pkgs[0].name, "lib-a");
    EXPECT_EQ(pkgs[1].name, "lib-b");
    EXPECT_EQ(pkgs[2].name, "lib-c");
}

TEST(PackageManagerGoogleTest, DependencyCycleDetection) {
    auto backend = std::make_shared<MockGraphRegistryBackend>();
    // Cycle: X -> Y -> X
    backend->add_pkg("pkg-x", SemVersion{1, 0, 0, ""}, {{"pkg-y", "^1.0.0"}});
    backend->add_pkg("pkg-y", SemVersion{1, 0, 0, ""}, {{"pkg-x", "^1.0.0"}});

    RegistryClient client(backend);
    PackageManifest root;
    root.add_dependency("pkg-x", "^1.0.0");

    DependencyResolver resolver;
    auto res = resolver.resolve(root, client);
    EXPECT_FALSE(res.has_value());
    EXPECT_NE(res.status().message().find("Circular dependency"), std::string::npos);
}

TEST(PackageManagerGoogleTest, DependencyVersionConflictDetection) {
    auto backend = std::make_shared<MockGraphRegistryBackend>();
    backend->add_pkg("common-lib", SemVersion{1, 5, 0, ""}, {});
    backend->add_pkg("common-lib", SemVersion{2, 0, 0, ""}, {});
    backend->add_pkg("pkg-1", SemVersion{1, 0, 0, ""}, {{"common-lib", "^1.0.0"}});
    backend->add_pkg("pkg-2", SemVersion{1, 0, 0, ""}, {{"common-lib", "^2.0.0"}});

    RegistryClient client(backend);
    PackageManifest root;
    root.add_dependency("pkg-1", "^1.0.0");
    root.add_dependency("pkg-2", "^1.0.0");

    DependencyResolver resolver;
    auto res = resolver.resolve(root, client);
    EXPECT_FALSE(res.has_value());
    EXPECT_NE(res.status().message().find("Dependency conflict"), std::string::npos);
}

// ============================================================================
// PKG-106 / Audit Finding C: Path Traversal Zip-Slip Defense
// ============================================================================

TEST(PackageManagerGoogleTest, ArchiveZipSlipPathTraversalRejection) {
    std::vector<uint8_t> malicious_archive;
    uint32_t magic = 0x52414450; // PDAR
    const uint8_t* m_ptr = reinterpret_cast<const uint8_t*>(&magic);
    malicious_archive.insert(malicious_archive.end(), m_ptr, m_ptr + 4);

    // Entry with path traversal: "../../system_leak.txt"
    std::string bad_path = "../../system_leak.txt";
    uint32_t path_len = static_cast<uint32_t>(bad_path.size());
    const uint8_t* pl_ptr = reinterpret_cast<const uint8_t*>(&path_len);
    malicious_archive.insert(malicious_archive.end(), pl_ptr, pl_ptr + 4);
    malicious_archive.insert(malicious_archive.end(), bad_path.begin(), bad_path.end());

    std::string payload = "evil payload";
    uint64_t file_len = static_cast<uint64_t>(payload.size());
    const uint8_t* fl_ptr = reinterpret_cast<const uint8_t*>(&file_len);
    malicious_archive.insert(malicious_archive.end(), fl_ptr, fl_ptr + 8);
    malicious_archive.insert(malicious_archive.end(), payload.begin(), payload.end());

    std::filesystem::path dest = "build/test_unzip_safe";
    auto status = ArchiveUtil::unpack_package_archive(malicious_archive, dest);
    EXPECT_FALSE(status.ok());
    EXPECT_NE(status.message().find("Path traversal"), std::string::npos);
}
