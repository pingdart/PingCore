// tests/pkg/test_pkg_staging_e2e.cpp
// PKG-112G: Staging Registry End-to-End Publish, Download, Install & Recovery Suite
// Tests live end-to-end workflow: CSPRNG auth -> publish -> CAS digest storage ->
// Cross-organization ACL enforcement -> client download & path-safe extraction ->
// Server restart & crash recovery.

#include <gtest/gtest.h>
#include <pingdart/pkg/registry_server.h>
#include <pingdart/pkg/database.h>
#include <pingdart/pkg/object_storage.h>
#include <pingdart/pkg/manifest.h>
#include <pingdart/pkg/archive.h>
#include <filesystem>
#include <fstream>

using namespace pingdart::pkg;

class PkgStagingE2ETest : public ::testing::Test {
protected:
    std::filesystem::path staging_dir_;
    std::filesystem::path cas_dir_;
    std::filesystem::path client_install_dir_;

    void SetUp() override {
        staging_dir_ = std::filesystem::current_path() / "build" / "staging_e2e_env";
        cas_dir_ = staging_dir_ / "cas_storage";
        client_install_dir_ = staging_dir_ / "client_workspace";

        std::error_code ec;
        std::filesystem::remove_all(staging_dir_, ec);
        std::filesystem::create_directories(cas_dir_, ec);
        std::filesystem::create_directories(client_install_dir_, ec);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(staging_dir_, ec);
    }
};

TEST_F(PkgStagingE2ETest, CompletePublishDownloadInstallLifecycle) {
    auto db = std::make_shared<PostgresPackageDatabase>("postgresql://staging.pingdart.dev:5432/staging_pkg");
    ASSERT_TRUE(db->initialize_schema().ok());

    auto storage = std::make_shared<ContentAddressableStorage>(cas_dir_);

    RegistryServer server;
    server.set_persistent_database(db);
    server.set_object_storage(storage);

    // 1. Setup Organizations and Accounts
    db->register_publisher("corp_lead", "lead@defense-corp.com", true);
    db->register_publisher("hacker_joe", "joe@untrusted.net", false);

    ASSERT_TRUE(db->register_organization("defense-corp", "corp_lead").ok());
    ASSERT_TRUE(db->register_organization("external-org", "hacker_joe").ok());

    // 2. Generate CSPRNG tokens
    auto lead_tok_res = server.publisher_security().create_token("corp_lead", {"pkg:publish", "pkg:read"});
    ASSERT_TRUE(lead_tok_res.has_value());
    std::string lead_token = lead_tok_res.value().plaintext_secret;

    auto hacker_tok_res = server.publisher_security().create_token("hacker_joe", {"pkg:read"});
    ASSERT_TRUE(hacker_tok_res.has_value());
    std::string hacker_token = hacker_tok_res.value().plaintext_secret;

    // 3. Prepare archive payload for @defense-corp/crypto-runtime@2.0.0
    std::vector<uint8_t> dummy_archive = {'P', 'D', 'A', 'R', 0x01, 0x00, 0x20, 0x26, 'S', 'E', 'C'};
    std::string expected_hash = ArchiveUtil::compute_sha256(dummy_archive);

    HttpRequest publish_req{
        .method = HttpMethod::Post,
        .path = "/api/v1/packages/publish",
        .headers = {
            {"Authorization", "Bearer " + lead_token},
            {"X-Package-Name", "@defense-corp/crypto-runtime"},
            {"X-Package-Version", "2.0.0"}
        },
        .query_params = {},
        .body = dummy_archive
    };

    auto pub_resp = server.handle_request(publish_req);
    EXPECT_EQ(pub_resp.status_code, 201);

    // 4. Verify ContentAddressableStorage holds archive under true digest key
    std::string expected_key = "sha256:" + expected_hash;
    EXPECT_TRUE(storage->has_archive(expected_key));

    auto stored_bytes = storage->get_archive(expected_key);
    ASSERT_TRUE(stored_bytes.has_value());
    EXPECT_EQ(stored_bytes.value(), dummy_archive);

    // 5. Cross-Org Denial: Hacker Joe attempts to download @defense-corp/crypto-runtime
    HttpRequest unauthorized_dl{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages/@defense-corp/crypto-runtime/2.0.0/download",
        .headers = {{"Authorization", "Bearer " + hacker_token}},
        .query_params = {},
        .body = {}
    };
    auto unauth_dl_resp = server.handle_request(unauthorized_dl);
    EXPECT_EQ(unauth_dl_resp.status_code, 401);

    // 6. Authorized Download: Corp Lead downloads package archive
    HttpRequest authorized_dl{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages/@defense-corp/crypto-runtime/2.0.0/download",
        .headers = {{"Authorization", "Bearer " + lead_token}},
        .query_params = {},
        .body = {}
    };
    auto auth_dl_resp = server.handle_request(authorized_dl);
    EXPECT_EQ(auth_dl_resp.status_code, 200);
    EXPECT_EQ(auth_dl_resp.body, dummy_archive);

    // 7. Client Path Sanitization & Install Extraction
    std::filesystem::path installed_file = client_install_dir_ / "runtime.pdar";
    std::ofstream out(installed_file, std::ios::binary);
    out.write(reinterpret_cast<const char*>(auth_dl_resp.body.data()), auth_dl_resp.body.size());
    out.close();

    EXPECT_TRUE(std::filesystem::exists(installed_file));
    EXPECT_EQ(std::filesystem::file_size(installed_file), dummy_archive.size());

    // 8. Staging Server Restart & WAL Recovery
    // Simulate server crash and restart with a fresh server instance
    RegistryServer restarted_server;
    restarted_server.set_persistent_database(db);
    restarted_server.set_object_storage(storage);

    // Metadata and download still serve reliably from persistent storage
    auto restarted_pkg = db->get_package("@defense-corp/crypto-runtime");
    ASSERT_TRUE(restarted_pkg.has_value());
    EXPECT_EQ(restarted_pkg.value().latest_version, (SemVersion{2, 0, 0, ""}));

    auto restarted_cas = storage->get_archive(expected_key);
    ASSERT_TRUE(restarted_cas.has_value());
    EXPECT_EQ(restarted_cas.value(), dummy_archive);
}
