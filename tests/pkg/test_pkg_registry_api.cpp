// tests/pkg/test_pkg_registry_api.cpp
// PKG-108: Public Registry API and Developer Portal Tests
#include <gtest/gtest.h>
#include <pingdart/pkg/registry_server.h>
#include <pingdart/pkg/archive.h>
#include <fstream>

using namespace pingdart::pkg;

// ============================================================================
// PKG-108: REST API Tests
// ============================================================================

TEST(RegistryApiGoogleTest, HealthCheckEndpoint) {
    RegistryServer server;
    HttpRequest req{
        .method = HttpMethod::Get,
        .path = "/api/v1/health"
    };

    auto resp = server.handle_request(req);
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_NE(resp.body_as_string().find("\"status\": \"healthy\""), std::string::npos);
    EXPECT_NE(resp.body_as_string().find("\"operational\""), std::string::npos);
}

TEST(RegistryApiGoogleTest, PackageSearchApi) {
    RegistryServer server;
    HttpRequest req{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages",
        .query_params = {{"q", "charts"}}
    };

    auto resp = server.handle_request(req);
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_NE(resp.body_as_string().find("\"name\": \"pingdart-charts\""), std::string::npos);
    EXPECT_NE(resp.body_as_string().find("\"count\": 1"), std::string::npos);
}

TEST(RegistryApiGoogleTest, PackageDetailAndVersionInspectionApi) {
    RegistryServer server;

    // 1. Package details
    HttpRequest req1{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages/pingdart-charts"
    };
    auto resp1 = server.handle_request(req1);
    EXPECT_EQ(resp1.status_code, 200);
    EXPECT_NE(resp1.body_as_string().find("\"latest_version\": \"1.2.0\""), std::string::npos);
    EXPECT_NE(resp1.body_as_string().find("\"pingdart-team\""), std::string::npos);

    // 2. Specific version details
    HttpRequest req2{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages/pingdart-charts/1.2.0"
    };
    auto resp2 = server.handle_request(req2);
    EXPECT_EQ(resp2.status_code, 200);
    EXPECT_NE(resp2.body_as_string().find("sha256:7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926"), std::string::npos);

    // 3. Binary archive download
    HttpRequest req3{
        .method = HttpMethod::Get,
        .path = "/api/v1/packages/pingdart-charts/1.2.0/download"
    };
    auto resp3 = server.handle_request(req3);
    EXPECT_EQ(resp3.status_code, 200);
    EXPECT_FALSE(resp3.body.empty());
}

TEST(RegistryApiGoogleTest, ScopedTokenGenerationAndAuthentication) {
    RegistryServer server;

    // Request new token
    HttpRequest req{
        .method = HttpMethod::Post,
        .path = "/api/v1/auth/tokens",
        .query_params = {{"username", "pingdart-team"}}
    };
    auto resp = server.handle_request(req);
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_NE(resp.body_as_string().find("\"token\": \"pd_live_"), std::string::npos);
}

TEST(RegistryApiGoogleTest, PublishingEnforcesAuthenticationAndOwnership) {
    RegistryServer server;

    // Create synthetic clean archive payload
    std::filesystem::path temp_dir = "build/test_pub_api";
    std::error_code ec;
    std::filesystem::create_directories(temp_dir / "include", ec);
    std::filesystem::create_directories(temp_dir / "src", ec);
    {
        std::ofstream h(temp_dir / "include" / "test.h"); h << "#pragma once\n";
        std::ofstream c(temp_dir / "src" / "test.cpp"); c << "void test() {}\n";
    }
    auto pack_res = ArchiveUtil::pack_package_directory(temp_dir);
    ASSERT_TRUE(pack_res.has_value());

    // 1. Publish without token -> 401 Unauthorized
    HttpRequest unauth_req{
        .method = HttpMethod::Post,
        .path = "/api/v1/packages/publish",
        .headers = {
            {"X-Package-Name", "pingdart-test-lib"},
            {"X-Package-Version", "1.0.0"}
        },
        .body = pack_res.value()
    };
    auto unauth_resp = server.handle_request(unauth_req);
    EXPECT_EQ(unauth_resp.status_code, 401);

    // 2. Publish with valid token for pingdart-team -> 201 Created
    HttpRequest valid_req = unauth_req;
    valid_req.headers["Authorization"] = "Bearer pd_token_live_sec_99a8b7c6d5e4f3a2b1";
    auto valid_resp = server.handle_request(valid_req);
    EXPECT_EQ(valid_resp.status_code, 201);

    // 3. Immutability: Re-publishing identical version -> 409 Conflict
    auto dupe_resp = server.handle_request(valid_req);
    EXPECT_EQ(dupe_resp.status_code, 409);
    EXPECT_NE(dupe_resp.body_as_string().find("immutable"), std::string::npos);

    // 4. Ownership: Another publisher tries to publish to pingdart-test-lib -> 403 Forbidden
    auto tok_res = server.publisher_security().create_token("community-dev", {"pkg:publish"});
    ASSERT_TRUE(tok_res.has_value());

    HttpRequest hijack_req = valid_req;
    hijack_req.headers["Authorization"] = "Bearer " + tok_res.value().plaintext_secret;
    hijack_req.headers["X-Package-Version"] = "1.1.0";
    auto hijack_resp = server.handle_request(hijack_req);
    EXPECT_EQ(hijack_resp.status_code, 403);
    EXPECT_NE(hijack_resp.body_as_string().find("not an authorized maintainer"), std::string::npos);
}

// ============================================================================
// PKG-108: Developer Portal HTML Rendering Tests
// ============================================================================

TEST(RegistryApiGoogleTest, DeveloperPortalHomepageRendering) {
    RegistryServer server;
    HttpRequest req{
        .method = HttpMethod::Get,
        .path = "/"
    };

    auto resp = server.handle_request(req);
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_EQ(resp.headers["Content-Type"], "text/html; charset=utf-8");

    std::string html = resp.body_as_string();
    EXPECT_NE(html.find("PingDart Package Registry"), std::string::npos);
    EXPECT_NE(html.find("Verified Packages"), std::string::npos);
    EXPECT_NE(html.find("pd add pingdart-charts"), std::string::npos);
}

TEST(RegistryApiGoogleTest, DeveloperPortalPackageDetailRendering) {
    RegistryServer server;
    HttpRequest req{
        .method = HttpMethod::Get,
        .path = "/packages/pingdart-charts"
    };

    auto resp = server.handle_request(req);
    EXPECT_EQ(resp.status_code, 200);

    std::string html = resp.body_as_string();
    EXPECT_NE(html.find("<h1>pingdart-charts"), std::string::npos);
    EXPECT_NE(html.find("Verified Publisher"), std::string::npos);
    EXPECT_NE(html.find("Automated Checks Passed"), std::string::npos);
    EXPECT_NE(html.find("v1.2.0"), std::string::npos);
}
