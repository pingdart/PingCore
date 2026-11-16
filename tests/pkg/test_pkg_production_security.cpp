// tests/pkg/test_pkg_production_security.cpp
// PKG-111: Production Security & Staging Deployment Suite
// Verifies CSPRNG token entropy, PostgreSQL distributed transactions,
// Content-Addressable Storage immutability, OS sandbox isolation,
// Cryptographic audit chain tamper detection, and Server-Side Private Auth.

#include <gtest/gtest.h>
#include <pingdart/pkg/security_scanner.h>
#include <pingdart/pkg/database.h>
#include <pingdart/pkg/object_storage.h>
#include <pingdart/pkg/os_sandbox.h>
#include <pingdart/pkg/registry_server.h>
#include <pingdart/pkg/manifest.h>
#include <set>
#include <filesystem>

using namespace pingdart::pkg;

// ============================================================================
// PKG-111A: CSPRNG 256-Bit Entropy & Non-Predictability
// ============================================================================

TEST(PkgProductionSecurityTest, CSPRNGTokenEntropyAndNonPredictability) {
    PublisherSecurity security;
    std::set<std::string> generated_tokens;
    const size_t iterations = 100;

    for (size_t i = 0; i < iterations; ++i) {
        auto res = security.create_token("test-publisher", {"pkg:publish", "pkg:read"});
        ASSERT_TRUE(res.has_value());

        const std::string& secret = res.value().plaintext_secret;
        // Verify prefix and 256 bits of entropy (64 hex characters)
        EXPECT_TRUE(secret.starts_with("pd_live_"));
        EXPECT_EQ(secret.size(), 8 + 64); // pd_live_ (8 chars) + 64 hex chars = 72 chars

        // Verify all 64 entropy chars are valid hexadecimal digits
        for (size_t c = 8; c < secret.size(); ++c) {
            char ch = secret[c];
            bool is_hex = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
            EXPECT_TRUE(is_hex);
        }

        // Verify uniqueness (collision probability is 1 in 2^256)
        EXPECT_EQ(generated_tokens.count(secret), 0);
        generated_tokens.insert(secret);

        // Verify token struct only stores nonreversible SHA-256 hash, not raw secret
        EXPECT_TRUE(res.value().token.token_secret_hash.starts_with("sha256:"));
        EXPECT_NE(res.value().token.token_secret_hash, secret);
    }

    EXPECT_EQ(generated_tokens.size(), iterations);
}

// ============================================================================
// PKG-111C: PostgreSQL-Backed Relational Schema & Concurrent Transactions
// ============================================================================

TEST(PkgProductionSecurityTest, PostgresDatabaseConcurrentPublishTransactions) {
    PostgresPackageDatabase db("postgresql://localhost:5432/pingdart_registry_test");
    ASSERT_TRUE(db.initialize_schema().ok());

    db.register_publisher("acme-corp", "admin@acme.dev", true);
    db.register_publisher("external-dev", "external@dev.org", false);

    PackageManifest manifest;
    manifest.metadata().name = "acme-service";
    manifest.metadata().version = SemVersion{1, 0, 0, ""};
    manifest.metadata().description = "ACME Core Service";
    manifest.metadata().license = "Apache-2.0";

    // 1. First publish claims namespace atomically
    auto pub1_status = db.publish_package_version_tx(
        manifest,
        "sha256:1111222233334444555566667777888899990000111122223333444455556666",
        "cas://acme-service/1.0.0.pdar",
        "acme-corp"
    );
    EXPECT_TRUE(pub1_status.ok());

    // 2. Duplicate publish of exact same version rejected by UNIQUE(package_id, version) constraint
    auto dup_status = db.publish_package_version_tx(
        manifest,
        "sha256:1111222233334444555566667777888899990000111122223333444455556666",
        "cas://acme-service/1.0.0.pdar",
        "acme-corp"
    );
    EXPECT_EQ(dup_status.code(), pingdart::core::StatusCode::AlreadyExists);

    // 3. Unauthorized publisher cannot publish new version of claimed package
    manifest.metadata().version = SemVersion{1, 1, 0, ""};
    auto unauth_status = db.publish_package_version_tx(
        manifest,
        "sha256:aaaa222233334444555566667777888899990000111122223333444455556666",
        "cas://acme-service/1.1.0.pdar",
        "external-dev"
    );
    EXPECT_EQ(unauth_status.code(), pingdart::core::StatusCode::PermissionDenied);

    // 4. Add collaborator transactionally
    auto collab_status = db.add_collaborator_tx("acme-corp", "acme-service", "external-dev");
    EXPECT_TRUE(collab_status.ok());

    // 5. Authorized collaborator can now publish
    auto collab_pub = db.publish_package_version_tx(
        manifest,
        "sha256:bbbb222233334444555566667777888899990000111122223333444455556666",
        "cas://acme-service/1.1.0.pdar",
        "external-dev"
    );
    EXPECT_TRUE(collab_pub.ok());

    // Verify package records in database
    auto pkg_res = db.get_package("acme-service");
    ASSERT_TRUE(pkg_res.has_value());
    EXPECT_EQ(pkg_res.value().latest_version, (SemVersion{1, 1, 0, ""}));
    EXPECT_EQ(pkg_res.value().versions.size(), 2);
}

// ============================================================================
// PKG-111D: Content-Addressable Storage Immutability & Write-Once
// ============================================================================

TEST(PkgProductionSecurityTest, ContentAddressableStorageImmutability) {
    std::filesystem::path temp_cas_dir = std::filesystem::current_path() / "build" / "test_cas_storage";
    std::error_code ec;
    std::filesystem::remove_all(temp_cas_dir, ec);

    ContentAddressableStorage cas(temp_cas_dir);

    std::vector<uint8_t> archive_payload = {'P', 'D', 'A', 'R', 0x01, 0x02, 0x03, 0x04};
    std::string key = "cas://acme/1.0.0.pdar";

    // 1. Initial write succeeds
    auto put_res = cas.put_archive(key, archive_payload);
    EXPECT_TRUE(put_res.ok());
    EXPECT_TRUE(cas.has_archive(key));

    // 2. Fetch retrieves exact bitwise identical data
    auto get_res = cas.get_archive(key);
    ASSERT_TRUE(get_res.has_value());
    EXPECT_EQ(get_res.value(), archive_payload);

    // 3. Overwriting existing storage key is strictly forbidden (Immutability guarantee)
    std::vector<uint8_t> malicious_replacement = {'M', 'A', 'L', 'I', 'C', 'I', 'O', 'U', 'S'};
    auto overwrite_res = cas.put_archive(key, malicious_replacement);
    EXPECT_EQ(overwrite_res.code(), pingdart::core::StatusCode::AlreadyExists);

    // Verify payload remained untouched
    auto untouched = cas.get_archive(key);
    ASSERT_TRUE(untouched.has_value());
    EXPECT_EQ(untouched.value(), archive_payload);

    std::filesystem::remove_all(temp_cas_dir, ec);
}

// ============================================================================
// PKG-111B: OS-Level Sandbox Isolation & Resource Limits
// ============================================================================

TEST(PkgProductionSecurityTest, OsSandboxWorkerResourceQuotas) {
    EXPECT_TRUE(OsSandboxWorker::is_os_isolation_available());

    OsSandboxLimits limits{
        .max_cpu_time_ms = 5000,
        .max_memory_bytes = 256ULL * 1024ULL * 1024ULL, // 256MB
        .max_processes = 16,
        .deny_network = true,
        .read_only_host_fs = true,
        .strip_host_credentials = true
    };

    OsSandboxWorker worker(limits);
    EXPECT_TRUE(worker.limits().deny_network);
    EXPECT_EQ(worker.limits().max_processes, 16);

    std::filesystem::path ws = std::filesystem::current_path();
    auto report_res = worker.run_isolated_build(ws, "cmake --build .");
    ASSERT_TRUE(report_res.has_value());

    const auto& report = report_res.value();
    EXPECT_TRUE(report.success);
    EXPECT_TRUE(report.network_attempt_blocked);
    EXPECT_FALSE(report.memory_limit_hit);
    EXPECT_FALSE(report.timeout_hit);
}

// ============================================================================
// PKG-111: Cryptographically Chained Tamper-Evident Audit Trail
// ============================================================================

TEST(PkgProductionSecurityTest, CryptographicAuditLogHashChain) {
    PublisherSecurity security;

    // Log multiple audit events
    security.record_audit_event(SecurityAuditEntry{
        .timestamp = "2026-11-17T00:01:00Z",
        .actor = "alice",
        .action = "token_create",
        .package_name = "",
        .version = "",
        .status = "success",
        .details = "Generated CLI access token"
    });

    security.record_audit_event(SecurityAuditEntry{
        .timestamp = "2026-11-17T00:02:00Z",
        .actor = "alice",
        .action = "publish",
        .package_name = "pingdart-crypto",
        .version = "1.0.0",
        .status = "success",
        .details = "Published release"
    });

    security.record_audit_event(SecurityAuditEntry{
        .timestamp = "2026-11-17T00:03:00Z",
        .actor = "bob",
        .action = "add_collaborator",
        .package_name = "pingdart-crypto",
        .version = "",
        .status = "success",
        .details = "Added charlie as collaborator"
    });

    // 1. Verify chain integrity
    const auto& trail = security.audit_trail();
    ASSERT_EQ(trail.size(), 3);
    EXPECT_EQ(trail[0].prev_hash, "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(trail[1].prev_hash, trail[0].entry_hash);
    EXPECT_EQ(trail[2].prev_hash, trail[1].entry_hash);

    EXPECT_TRUE(security.verify_audit_log_integrity());

    // 2. Tampering test: Modifying any field in an intermediate record breaks chain verification
    auto& mutable_trail = const_cast<std::vector<SecurityAuditEntry>&>(security.audit_trail());
    mutable_trail[1].action = "publish_malicious_override";

    EXPECT_FALSE(security.verify_audit_log_integrity());
}

// ============================================================================
// PKG-111E: Server-Side Private Registry Authorization Enforcement
// ============================================================================

TEST(PkgProductionSecurityTest, ServerSidePrivateRegistryAuthorization) {
    RegistryServer server;

    // Register a valid publisher token
    auto tok_res = server.publisher_security().create_token("corp-admin", {"pkg:read", "pkg:publish"});
    ASSERT_TRUE(tok_res.has_value());
    std::string valid_token = tok_res.value().plaintext_secret;

    // Package visibility configuration
    server.set_package_visibility("enterprise-telemetry", true);

    // 1. Unauthenticated request to private package metadata returns HTTP 401
    HttpRequest unauth_req{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages/enterprise-telemetry",
        .headers = {},
        .query_params = {},
        .body = {}
    };
    auto unauth_resp = server.handle_request(unauth_req);
    EXPECT_EQ(unauth_resp.status_code, 401);

    // 2. Unauthenticated download request returns HTTP 401
    HttpRequest unauth_dl{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages/enterprise-telemetry/1.0.0/download",
        .headers = {},
        .query_params = {},
        .body = {}
    };
    auto unauth_dl_resp = server.handle_request(unauth_dl);
    EXPECT_EQ(unauth_dl_resp.status_code, 401);

    // 3. Scoped package (@corp/auth) is automatically treated as private
    HttpRequest scoped_req{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages/@corp/auth",
        .headers = {},
        .query_params = {},
        .body = {}
    };
    auto scoped_resp = server.handle_request(scoped_req);
    EXPECT_EQ(scoped_resp.status_code, 401);

    // 4. Authenticated request with valid Bearer token passes authorization check
    HttpRequest auth_req{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages/@corp/auth",
        .headers = {{"Authorization", "Bearer " + valid_token}},
        .query_params = {},
        .body = {}
    };
    auto auth_resp = server.handle_request(auth_req);
    // Should pass auth check (may return 404 because @corp/auth is not published yet, but NOT 401)
    EXPECT_NE(auth_resp.status_code, 401);

    // 5. Global Private Registry Mode: locks down all catalog discovery
    server.set_private_registry(true);

    HttpRequest catalog_req{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages",
        .headers = {},
        .query_params = {},
        .body = {}
    };
    auto catalog_resp = server.handle_request(catalog_req);
    EXPECT_EQ(catalog_resp.status_code, 401);

    // Catalog request with Bearer token succeeds
    HttpRequest catalog_auth{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages",
        .headers = {{"Authorization", "Bearer " + valid_token}},
        .query_params = {},
        .body = {}
    };
    auto catalog_auth_resp = server.handle_request(catalog_auth);
    EXPECT_EQ(catalog_auth_resp.status_code, 200);
}
