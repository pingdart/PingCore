// tests/pkg/test_pkg_production_security.cpp
// PKG-112: Production Security Hardening, Relational ACLs & Staging Delivery Suite
// Verifies OS CSPRNG entropy, Windows AppContainer/Restricted Token isolation,
// Digest-addressed CAS storage with retrieval reverification,
// Organization-level private package ACLs, Externally anchored audit checkpoints,
// and PostgreSQL transaction rollback & crash recovery.

#include <gtest/gtest.h>
#include <pingdart/pkg/security_scanner.h>
#include <pingdart/pkg/database.h>
#include <pingdart/pkg/object_storage.h>
#include <pingdart/pkg/os_sandbox.h>
#include <pingdart/pkg/registry_server.h>
#include <pingdart/pkg/manifest.h>
#include <pingdart/pkg/archive.h>
#include <set>
#include <fstream>
#include <filesystem>

using namespace pingdart::pkg;

// ============================================================================
// PKG-112A: CSPRNG 256-Bit OS Entropy & Non-Predictability
// ============================================================================

TEST(PkgProductionSecurityTest, CSPRNGTokenEntropyAndNonPredictability) {
    PublisherSecurity security;
    security.register_publisher("test-publisher", "test@pingdart.dev", true);
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

        // Verify uniqueness across CSPRNG generation
        EXPECT_EQ(generated_tokens.count(secret), 0);
        generated_tokens.insert(secret);

        // Verify token struct only stores nonreversible SHA-256 hash, not raw secret
        EXPECT_TRUE(res.value().token.token_hash.starts_with("sha256:"));
        EXPECT_NE(res.value().token.token_hash, secret);
    }

    EXPECT_EQ(generated_tokens.size(), iterations);
}

// ============================================================================
// PKG-112B: Digest-Addressed Storage & Download Reverification
// ============================================================================

TEST(PkgProductionSecurityTest, ContentAddressableStorageDigestAndReverification) {
    std::filesystem::path temp_cas_dir = std::filesystem::current_path() / "build" / "test_cas_digest_storage";
    std::error_code ec;
    std::filesystem::remove_all(temp_cas_dir, ec);

    ContentAddressableStorage cas(temp_cas_dir);

    std::vector<uint8_t> archive_payload = {'P', 'D', 'A', 'R', 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    std::string expected_sha256 = ArchiveUtil::compute_sha256(archive_payload);

    // 1. Put archive deriving true SHA-256 digest key
    auto put_res = cas.put_digest_archive(archive_payload);
    ASSERT_TRUE(put_res.has_value());
    std::string storage_key = put_res.value();
    EXPECT_EQ(storage_key, "sha256:" + expected_sha256);
    EXPECT_TRUE(cas.has_archive(storage_key));

    // 2. Fetch retrieves exact bitwise identical data and verifies digest
    auto get_res = cas.get_archive(storage_key);
    ASSERT_TRUE(get_res.has_value());
    EXPECT_EQ(get_res.value(), archive_payload);

    // 3. Immutability violation test: Overwriting with different payload is strictly rejected
    std::vector<uint8_t> corrupted_payload = {'C', 'O', 'R', 'R', 'U', 'P', 'T'};
    auto dup_res = cas.put_archive(storage_key, corrupted_payload);
    EXPECT_EQ(dup_res.code(), pingdart::core::StatusCode::AlreadyExists);

    // 4. Download reverification: If file on disk is tampered with, get_archive returns DataLoss
    std::string prefix = expected_sha256.substr(0, 2);
    std::filesystem::path disk_file = temp_cas_dir / "sha256" / prefix / (expected_sha256 + ".pdar");
    ASSERT_TRUE(std::filesystem::exists(disk_file, ec));

    // Tamper with disk contents directly (bypass filesystem read-only lock for test)
    std::filesystem::permissions(disk_file, std::filesystem::perms::owner_write, std::filesystem::perm_options::add, ec);
    {
        std::ofstream corrupt_out(disk_file, std::ios::binary | std::ios::trunc);
        corrupt_out.write("TAMPERED_CONTENT", 16);
    }

    auto tampered_res = cas.get_archive(storage_key);
    EXPECT_FALSE(tampered_res.has_value());
    EXPECT_EQ(tampered_res.status().code(), pingdart::core::StatusCode::DataLoss);

    std::filesystem::remove_all(temp_cas_dir, ec);
}

// ============================================================================
// PKG-112A: OS-Level Sandbox Isolation & AppContainer / Restricted Token
// ============================================================================

TEST(PkgProductionSecurityTest, OsSandboxWorkerAppContainerAndLimits) {
    EXPECT_TRUE(OsSandboxWorker::is_os_isolation_available());

    OsSandboxLimits limits{
        .max_cpu_time_ms = 5000,
        .max_memory_bytes = 256ULL * 1024ULL * 1024ULL, // 256MB
        .max_processes = 16,
        .deny_network = true,
        .read_only_host_fs = true,
        .strip_host_credentials = true,
        .enable_app_container = true
    };

    OsSandboxWorker worker(limits);
    EXPECT_TRUE(worker.limits().deny_network);
    EXPECT_TRUE(worker.limits().enable_app_container);

    std::filesystem::path ws = std::filesystem::current_path();
    auto report_res = worker.run_isolated_build(ws, "cmake --build .");
    ASSERT_TRUE(report_res.has_value());

    const auto& report = report_res.value();
    EXPECT_TRUE(report.success);
    EXPECT_TRUE(report.network_attempt_blocked);
    EXPECT_TRUE(report.app_container_applied);
    EXPECT_TRUE(report.restricted_token_applied);
    EXPECT_TRUE(report.filesystem_access_restricted);
    EXPECT_TRUE(report.host_credentials_stripped);
}

// ============================================================================
// PKG-112D: Externally Anchored Audit Checkpoints & Tamper Detection
// ============================================================================

TEST(PkgProductionSecurityTest, ExternallyAnchoredAuditCheckpoints) {
    PublisherSecurity security;

    // Record audit events
    security.record_audit_event(SecurityAuditEntry{
        .timestamp = "2026-11-17T01:00:00Z",
        .actor = "admin@pingdart.dev",
        .action = "token_create",
        .package_name = "",
        .version = "",
        .status = "success",
        .details = "Issued deployment token"
    });

    security.record_audit_event(SecurityAuditEntry{
        .timestamp = "2026-11-17T01:05:00Z",
        .actor = "admin@pingdart.dev",
        .action = "publish",
        .package_name = "pingdart-charts",
        .version = "1.3.0",
        .status = "success",
        .details = "Published immutable release"
    });

    // 1. Create external signed checkpoint
    auto anchor = security.create_external_anchor("https://transparency.pingdart.dev/api/v1/anchor");
    EXPECT_EQ(anchor.checkpoint_id, 1);
    EXPECT_EQ(anchor.entry_count, 2);
    EXPECT_TRUE(anchor.anchor_signature.starts_with("sig_ed25519_"));

    // 2. Cross-verify audit trail against anchor
    std::vector<AuditCheckpoint> external_trust_anchors = {anchor};
    EXPECT_TRUE(security.verify_audit_log_against_anchors(external_trust_anchors));

    // 3. Attack scenario: Attacker rewrites audit trail with valid internal hashes
    // but cannot forge the external trust anchor signature
    auto& mutable_trail = const_cast<std::vector<SecurityAuditEntry>&>(security.audit_trail());
    mutable_trail[1].actor = "malicious_actor";
    mutable_trail[1].entry_hash = mutable_trail[1].compute_hash(); // recompute internal hash

    // Internal verification alone might pass if chain is recomputed,
    // but verification AGAINST EXTERNAL ANCHOR MUST FAIL:
    EXPECT_FALSE(security.verify_audit_log_against_anchors(external_trust_anchors));
}

// ============================================================================
// PKG-112C: Organization-Level Private Package ACLs & Cross-Org Isolation
// ============================================================================

TEST(PkgProductionSecurityTest, OrganizationLevelPrivatePackageAcls) {
    PostgresPackageDatabase db("postgresql://localhost:5432/pingdart_registry_test");
    ASSERT_TRUE(db.initialize_schema().ok());

    // Register publishers and organizations
    db.register_publisher("alice", "alice@alpha-corp.com", true);
    db.register_publisher("bob", "bob@beta-corp.com", true);

    ASSERT_TRUE(db.register_organization("alpha-corp", "alice").ok());
    ASSERT_TRUE(db.register_organization("beta-corp", "bob").ok());

    PackageManifest alpha_manifest;
    alpha_manifest.metadata().name = "@alpha-corp/telemetry";
    alpha_manifest.metadata().version = SemVersion{1, 0, 0, ""};
    alpha_manifest.metadata().description = "Alpha Corp Internal Telemetry";
    alpha_manifest.metadata().license = "Proprietary";

    // 1. Alice publishes private package under @alpha-corp
    auto pub_status = db.publish_package_version_tx(
        alpha_manifest,
        "sha256:1111222233334444555566667777888899990000111122223333444455556666",
        "sha256:1111222233334444555566667777888899990000111122223333444455556666",
        "alice"
    );
    EXPECT_TRUE(pub_status.ok());

    // 2. Cross-Org Unauthorized Publish: Bob (beta-corp) CANNOT publish to @alpha-corp
    PackageManifest hijack_manifest;
    hijack_manifest.metadata().name = "@alpha-corp/malicious-sdk";
    hijack_manifest.metadata().version = SemVersion{1, 0, 0, ""};

    auto hijack_status = db.publish_package_version_tx(
        hijack_manifest,
        "sha256:3333222233334444555566667777888899990000111122223333444455556666",
        "sha256:3333222233334444555566667777888899990000111122223333444455556666",
        "bob"
    );
    EXPECT_EQ(hijack_status.code(), pingdart::core::StatusCode::PermissionDenied);

    // 3. Search ACL Isolation: Alice can discover @alpha-corp/telemetry; Bob CANNOT
    auto alice_search = db.search_for_user("alice", "telemetry");
    ASSERT_TRUE(alice_search.has_value());
    EXPECT_EQ(alice_search.value().size(), 1);
    EXPECT_EQ(alice_search.value()[0].name, "@alpha-corp/telemetry");

    auto bob_search = db.search_for_user("bob", "telemetry");
    ASSERT_TRUE(bob_search.has_value());
    EXPECT_EQ(bob_search.value().size(), 0); // Omitted to avoid private package name leakage

    // 4. Anonymous Search: private scoped package is completely hidden
    auto anon_search = db.search_for_user("", "telemetry");
    ASSERT_TRUE(anon_search.has_value());
    EXPECT_EQ(anon_search.value().size(), 0);

    // 5. Package Access Check
    EXPECT_TRUE(db.can_access_package("alice", "@alpha-corp/telemetry", "read"));
    EXPECT_FALSE(db.can_access_package("bob", "@alpha-corp/telemetry", "read"));
}

// ============================================================================
// PKG-112E: PostgreSQL Concurrency, Rollback & Crash Recovery
// ============================================================================

TEST(PkgProductionSecurityTest, PostgresTransactionRollbackAndCrashRecovery) {
    PostgresPackageDatabase db("postgresql://localhost:5432/pingdart_registry_test");
    ASSERT_TRUE(db.initialize_schema().ok());

    db.register_publisher("staging-admin", "admin@pingdart.dev", true);

    // 1. Transaction BEGIN
    EXPECT_TRUE(db.begin_transaction().ok());

    // Double begin fails
    EXPECT_EQ(db.begin_transaction().code(), pingdart::core::StatusCode::FailedPrecondition);

    // 2. Transaction ROLLBACK
    EXPECT_TRUE(db.rollback_transaction().ok());

    // 3. Simulated Connection Loss & WAL Crash Recovery
    EXPECT_TRUE(db.begin_transaction().ok());
    EXPECT_TRUE(db.simulate_crash_and_recover().ok());

    // State is clean and ready for new transactions
    EXPECT_TRUE(db.begin_transaction().ok());
    EXPECT_TRUE(db.commit_transaction().ok());
}

// ============================================================================
// PKG-112C: Server-Side Registry Authorization & Search Filtering
// ============================================================================

TEST(PkgProductionSecurityTest, ServerSidePrivateRegistryAuthorization) {
    RegistryServer server;
    auto db = std::make_shared<PostgresPackageDatabase>();
    server.set_persistent_database(db);

    db->register_publisher("corp-admin", "admin@corp.internal", true);
    db->register_publisher("outsider", "dev@outside.org", false);
    ASSERT_TRUE(db->register_organization("corp", "corp-admin").ok());

    auto tok_res = server.publisher_security().create_token("corp-admin", {"pkg:read", "pkg:publish"});
    ASSERT_TRUE(tok_res.has_value());
    std::string admin_token = tok_res.value().plaintext_secret;

    auto out_tok_res = server.publisher_security().create_token("outsider", {"pkg:read"});
    ASSERT_TRUE(out_tok_res.has_value());
    std::string outsider_token = out_tok_res.value().plaintext_secret;

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

    // 2. Scoped package (@corp/auth) accessed with outsider token is denied
    HttpRequest scoped_outsider_req{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages/@corp/auth",
        .headers = {{"Authorization", "Bearer " + outsider_token}},
        .query_params = {},
        .body = {}
    };
    auto scoped_outsider_resp = server.handle_request(scoped_outsider_req);
    EXPECT_EQ(scoped_outsider_resp.status_code, 401);

    // 3. Authenticated request with authorized org admin Bearer token passes auth
    HttpRequest auth_req{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages/@corp/auth",
        .headers = {{"Authorization", "Bearer " + admin_token}},
        .query_params = {},
        .body = {}
    };
    auto auth_resp = server.handle_request(auth_req);
    EXPECT_NE(auth_resp.status_code, 401);

    // 4. Global Private Registry Mode: locks down all catalog discovery
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

    // Catalog request with authorized Bearer token succeeds
    HttpRequest catalog_auth{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages",
        .headers = {{"Authorization", "Bearer " + admin_token}},
        .query_params = {},
        .body = {}
    };
    auto catalog_auth_resp = server.handle_request(catalog_auth);
    EXPECT_EQ(catalog_auth_resp.status_code, 200);
}
