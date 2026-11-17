// tests/pkg/test_pkg_scoped_registries.cpp
// PKG-110: Private Registries, Scoped Packages & Isolated Sandbox Tests
#include <gtest/gtest.h>
#include <pingdart/pkg/scoped_package.h>
#include <pingdart/pkg/multi_registry.h>
#include <pingdart/pkg/build_sandbox.h>
#include <fstream>

using namespace pingdart::pkg;
namespace core = pingdart::core;

// ============================================================================
// PKG-110: Scoped Package Identification & Validation Tests
// ============================================================================

TEST(ScopedRegistriesGoogleTest, ScopedPackageNameParsingAndNormalization) {
    // 1. Scoped company package
    auto s1_res = ScopedPackageName::parse("@company/internal-ui");
    ASSERT_TRUE(s1_res.has_value());
    const auto& s1 = s1_res.value();
    EXPECT_TRUE(s1.is_scoped());
    EXPECT_EQ(s1.scope(), "@company");
    EXPECT_EQ(s1.name(), "internal-ui");
    EXPECT_EQ(s1.full_name(), "@company/internal-ui");
    EXPECT_EQ(s1.to_cmake_target_name(), "company_internal_ui");

    // 2. Scoped pingdart package
    auto s2_res = ScopedPackageName::parse("@pingdart/database");
    ASSERT_TRUE(s2_res.has_value());
    EXPECT_EQ(s2_res.value().to_cmake_target_name(), "pingdart_database");

    // 3. Unscoped standard package
    auto s3_res = ScopedPackageName::parse("pingdart-charts");
    ASSERT_TRUE(s3_res.has_value());
    EXPECT_FALSE(s3_res.value().is_scoped());
    EXPECT_EQ(s3_res.value().to_cmake_target_name(), "charts");

    // 4. Invalid names rejected
    EXPECT_FALSE(ScopedPackageName::parse("").has_value());
    EXPECT_FALSE(ScopedPackageName::parse("@/invalid").has_value());
    EXPECT_FALSE(ScopedPackageName::parse("@scope/").has_value());
    EXPECT_FALSE(ScopedPackageName::parse("@inv@lid/name").has_value());
}

// ============================================================================
// PKG-110: Multi-Registry Configuration & Scope Routing Tests
// ============================================================================

TEST(ScopedRegistriesGoogleTest, MultiRegistryTomlParsingAndRouting) {
    std::string toml_sample = R"(
[registries.public]
url = "https://registry.pingdart.com"

[registries.internal]
url = "https://packages.example.com"
private = true
token = "sec_internal_token_12345"

[scopes]
"@company" = "internal"
"@pingdart" = "public"
)";

    auto cfg_res = MultiRegistryConfig::parse_toml(toml_sample);
    ASSERT_TRUE(cfg_res.has_value());
    const auto& cfg = cfg_res.value();

    // Verify registry endpoints
    const auto* pub = cfg.get_registry("public");
    ASSERT_NE(pub, nullptr);
    EXPECT_EQ(pub->url, "https://registry.pingdart.com");
    EXPECT_FALSE(pub->is_private);

    const auto* internal = cfg.get_registry("internal");
    ASSERT_NE(internal, nullptr);
    EXPECT_EQ(internal->url, "https://packages.example.com");
    EXPECT_TRUE(internal->is_private);
    EXPECT_EQ(internal->auth_token, "sec_internal_token_12345");

    // Verify scope resolution
    EXPECT_EQ(cfg.resolve_alias_for_package("@company/internal-ui"), "internal");
    EXPECT_EQ(cfg.resolve_alias_for_package("@company/auth-sdk"), "internal");
    EXPECT_EQ(cfg.resolve_alias_for_package("@pingdart/charts"), "public");
    EXPECT_EQ(cfg.resolve_alias_for_package("pingdart-sqlite"), "public");
}

// ============================================================================
// PKG-110: Private Registry Authentication Enforcement
// ============================================================================

TEST(ScopedRegistriesGoogleTest, PrivateRegistryAuthorizationEnforcement) {
    MultiRegistryConfig cfg;

    // Configure unauthenticated private registry
    RegistryEndpoint private_ep{
        .alias = "corp",
        .url = "https://corp.registry.internal",
        .is_private = true,
        .auth_token = "", // No token initially
        .backend = std::make_shared<DefaultRegistryBackend>()
    };
    cfg.add_registry(private_ep);
    cfg.map_scope("@corp", "corp");

    MultiRegistryClient client(cfg);

    // 1. Unauthenticated read/search on private package fails
    auto search_res = client.search("internal", "corp");
    EXPECT_FALSE(search_res.has_value());
    EXPECT_EQ(search_res.status().code(), core::StatusCode::Unauthenticated);

    auto pkg_res = client.get_package("@corp/internal-ui");
    EXPECT_FALSE(pkg_res.has_value());
    EXPECT_EQ(pkg_res.status().code(), core::StatusCode::Unauthenticated);

    auto dl_res = client.download_archive("@corp/internal-ui", SemVersion{1, 0, 0, ""});
    EXPECT_FALSE(dl_res.has_value());
    EXPECT_EQ(dl_res.status().code(), core::StatusCode::Unauthenticated);

    // 2. Authenticated access succeeds
    cfg.set_auth_token("corp", "pd_corp_bearer_token_xyz");
    client.set_config(cfg);

    auto auth_search = client.search("", "corp");
    EXPECT_TRUE(auth_search.has_value());
}

// ============================================================================
// Finding B / PKG-109: Isolated Sandbox Build Execution
// ============================================================================

TEST(ScopedRegistriesGoogleTest, IsolatedSandboxBuildAndSecretSanitization) {
    // 1. Environment sanitization
    std::map<std::string, std::string> dirty_env = {
        {"PATH", "C:\\bin"},
        {"TEMP", "C:\\temp"},
        {"GITHUB_TOKEN", "ghp_supersecret12345"},
        {"AWS_SECRET_ACCESS_KEY", "aws_secret_key"},
        {"REGISTRY_AUTH_TOKEN", "pd_tok_live_secret"},
        {"DATABASE_PASSWORD", "dbpass"}
    };

    auto clean_env = IsolatedPackageBuilder::sanitize_environment(dirty_env, {"PATH", "TEMP"});
    EXPECT_EQ(clean_env.size(), 2u);
    EXPECT_TRUE(clean_env.count("PATH"));
    EXPECT_TRUE(clean_env.count("TEMP"));
    EXPECT_FALSE(clean_env.count("GITHUB_TOKEN"));
    EXPECT_FALSE(clean_env.count("REGISTRY_AUTH_TOKEN"));

    // 2. Sandbox directory isolation
    std::filesystem::path test_pkg_dir = "build/test_untrusted_pkg";
    std::error_code ec;
    std::filesystem::remove_all(test_pkg_dir, ec);
    std::filesystem::create_directories(test_pkg_dir / "include", ec);
    {
        std::ofstream cm(test_pkg_dir / "CMakeLists.txt");
        cm << "cmake_minimum_required(VERSION 3.20)\nproject(Untrusted)\n";
        std::ofstream toml(test_pkg_dir / "pingdart.toml");
        toml << "[package]\nname = \"untrusted-pkg\"\nversion = \"0.1.0\"\n";
    }

    IsolatedPackageBuilder builder;
    auto res = builder.test_build_in_sandbox(test_pkg_dir);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res.value().success);
    EXPECT_EQ(res.value().exit_code, 0);
    EXPECT_FALSE(res.value().quota_exceeded);
}
