#include <gtest/gtest.h>
#include <pingdart/web/ssr_server.h>
#include <pingdart/web/semantic_widget.h>
#include "../../src/web/html/html_serializer.h"
#include <thread>
#include <vector>

using namespace pingdart::web;

TEST(SsrServerTest, ExactAndParameterizedRouteResolution) {
    SsrServer server;

    // Register /
    server.get("/", [](const HttpRequest&, const auto&) {
        return HttpResponse::Ok("<h1>Home Page</h1>");
    });

    // Register /doctors
    server.get("/doctors", [](const HttpRequest&, const auto&) {
        return HttpResponse::Ok("<h1>Doctors Directory</h1>");
    });

    // Register dynamic route /doctors/{slug}
    server.get("/doctors/{slug}", [](const HttpRequest&, const auto& params) {
        auto it = params.find("slug");
        std::string slug = (it != params.end()) ? it->second : "unknown";
        std::string body = "<h1>Profile: " + slug + "</h1>";
        return HttpResponse::Ok(std::move(body));
    });

    // 1. Home
    auto res_home = server.handle_request(HttpRequest{.path = "/"});
    EXPECT_EQ(res_home.status_code, 200);
    EXPECT_NE(res_home.body.find("Home Page"), std::string::npos);

    // 2. Directory
    auto res_docs = server.handle_request(HttpRequest{.path = "/doctors"});
    EXPECT_EQ(res_docs.status_code, 200);
    EXPECT_NE(res_docs.body.find("Doctors Directory"), std::string::npos);

    // 3. Dynamic slug
    auto res_doctor = server.handle_request(HttpRequest{.path = "/doctors/dr-jane-smith"});
    EXPECT_EQ(res_doctor.status_code, 200);
    EXPECT_NE(res_doctor.body.find("Profile: dr-jane-smith"), std::string::npos);

    // 4. Missing route -> 404
    auto res_404 = server.handle_request(HttpRequest{.path = "/nonexistent"});
    EXPECT_EQ(res_404.status_code, 404);
}

TEST(SsrServerTest, SeoEndpointsConfiguration) {
    SsrServer server;

    std::vector<std::pair<std::string, std::string>> routes = {
        {"https://example.com/", "2026-11-16"},
        {"https://example.com/doctors", "2026-11-16"}
    };

    server.configure_seo(routes, true, "https://example.com/sitemap.xml");

    // /sitemap.xml
    auto sitemap_res = server.handle_request(HttpRequest{.path = "/sitemap.xml"});
    EXPECT_EQ(sitemap_res.status_code, 200);
    EXPECT_EQ(sitemap_res.content_type, "application/xml; charset=utf-8");
    EXPECT_NE(sitemap_res.body.find("<urlset"), std::string::npos);

    // /robots.txt
    auto robots_res = server.handle_request(HttpRequest{.path = "/robots.txt"});
    EXPECT_EQ(robots_res.status_code, 200);
    EXPECT_EQ(robots_res.content_type, "text/plain; charset=utf-8");
    EXPECT_NE(robots_res.body.find("User-agent: *"), std::string::npos);
    EXPECT_NE(robots_res.body.find("Sitemap: https://example.com/sitemap.xml"), std::string::npos);
}

TEST(SsrServerTest, SecurityPathTraversalRejection) {
    SsrServer server;

    // Directory traversal attempt
    auto res_traversal = server.handle_request(HttpRequest{.path = "/doctors/../../etc/passwd"});
    EXPECT_EQ(res_traversal.status_code, 400);

    // Script injection attempt in path
    auto res_injection = server.handle_request(HttpRequest{.path = "/<script>alert(1)</script>"});
    EXPECT_EQ(res_injection.status_code, 400);
}

TEST(SsrServerTest, ConcurrentRequestsSafety) {
    SsrServer server;

    server.get("/profile/{user}", [](const HttpRequest&, const auto& params) {
        auto it = params.find("user");
        std::string user = (it != params.end()) ? it->second : "none";
        return HttpResponse::Ok("User: " + user);
    });

    constexpr int kNumThreads = 8;
    constexpr int kRequestsPerThread = 50;
    std::vector<std::thread> workers;

    for (int t = 0; t < kNumThreads; ++t) {
        workers.emplace_back([&server, t]() {
            for (int r = 0; r < kRequestsPerThread; ++r) {
                std::string path = "/profile/doctor_" + std::to_string(t);
                auto res = server.handle_request(HttpRequest{.path = path});
                EXPECT_EQ(res.status_code, 200);
                EXPECT_NE(res.body.find("doctor_" + std::to_string(t)), std::string::npos);
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }
}
