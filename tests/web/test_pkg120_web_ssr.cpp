// tests/web/test_pkg120_web_ssr.cpp
// PKG-120: Unit Tests for PingCore Web Runtime, SSR, SEO, Route Compiler & Security Hardening

#include <gtest/gtest.h>
#include <pingcore/web_ssr.h>
#include <pingcore/route_compiler.h>
#include <pingdart/web/url_security.h>
#include <filesystem>
#include <fstream>

using namespace pingcore;

class Pkg120WebSsrTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::error_code ec;
        std::filesystem::remove_all("dist/test_pkg120_app", ec);
        std::filesystem::remove_all("dist/test_ssg_output", ec);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all("dist/test_pkg120_app", ec);
        std::filesystem::remove_all("dist/test_ssg_output", ec);
    }
};

TEST_F(Pkg120WebSsrTest, RouteCompilerCollisionFreeGeneration) {
    std::filesystem::path app_dir = "dist/test_pkg120_app/app";
    std::filesystem::create_directories(app_dir / "login");
    std::filesystem::create_directories(app_dir / "dashboard");
    std::filesystem::create_directories(app_dir / "users" / "[id]");
    std::filesystem::create_directories(app_dir / "api" / "users");

    // All pages define the exact same function name: Widget Page()
    std::ofstream(app_dir / "layout.cpp") << "Widget Layout(Widget c) { return c; }\n";
    std::ofstream(app_dir / "page.cpp") << "Widget Page() { return Center(Text(\"Home\")); }\n";
    std::ofstream(app_dir / "login" / "page.cpp") << "Widget Page() { return Center(Text(\"Login\")); }\n";
    std::ofstream(app_dir / "dashboard" / "page.cpp") << "Widget Page() { return Center(Text(\"Dashboard\")); }\n";
    std::ofstream(app_dir / "users" / "[id]" / "page.cpp") << "Widget Page() { return Center(Text(\"User\")); }\n";
    std::ofstream(app_dir / "api" / "users" / "route.cpp") << "Response GET(Request& req) { (void)req; return Response::json({{\"ok\", true}}); }\n";

    auto res = RouteCompiler::compile("dist/test_pkg120_app");
    ASSERT_TRUE(res.success) << "Route compiler failed: " << res.error_message;
    EXPECT_EQ(res.page_count, 4u);
    EXPECT_EQ(res.api_route_count, 1u);
    EXPECT_TRUE(std::filesystem::exists(res.client_manifest_path));
    EXPECT_TRUE(std::filesystem::exists(res.server_manifest_path));
    EXPECT_TRUE(std::filesystem::exists("dist/test_pkg120_app/.pingcore/generated/routes_manifest.json"));

    // Verify client manifest isolates pages into namespaces and has main()
    std::ifstream cf(res.client_manifest_path);
    std::string client_src((std::istreambuf_iterator<char>(cf)), std::istreambuf_iterator<char>());
    EXPECT_NE(client_src.find("namespace pingcore_routes::page_0"), std::string::npos);
    EXPECT_NE(client_src.find("namespace pingcore_routes::page_1"), std::string::npos);
    EXPECT_NE(client_src.find("int main(int argc, char* argv[])"), std::string::npos);

    // Verify server manifest has main() and API registration
    std::ifstream sf(res.server_manifest_path);
    std::string server_src((std::istreambuf_iterator<char>(sf)), std::istreambuf_iterator<char>());
    EXPECT_NE(server_src.find("namespace pingcore_api::api_0"), std::string::npos);
    EXPECT_NE(server_src.find("server.get(\"/api/users\""), std::string::npos);
    EXPECT_NE(server_src.find("int main()"), std::string::npos);
}

TEST_F(Pkg120WebSsrTest, SecurityNegativePathTraversalDetection) {
    // Unsafe route path containing directory traversal
    EXPECT_FALSE(pingdart::web::UrlSecurity::is_safe_route_parameter(".."));
    EXPECT_FALSE(pingdart::web::UrlSecurity::is_safe_route_parameter("../secret"));
    EXPECT_FALSE(pingdart::web::UrlSecurity::is_safe_route_parameter("../../etc/passwd"));
    EXPECT_FALSE(pingdart::web::UrlSecurity::is_safe_route_parameter("user<script>"));
    EXPECT_FALSE(pingdart::web::UrlSecurity::is_safe_route_parameter(""));

    // Safe route parameters
    EXPECT_TRUE(pingdart::web::UrlSecurity::is_safe_route_parameter("42"));
    EXPECT_TRUE(pingdart::web::UrlSecurity::is_safe_route_parameter("alice-smith"));
    EXPECT_TRUE(pingdart::web::UrlSecurity::is_safe_route_parameter("post_100.html"));
}

TEST_F(Pkg120WebSsrTest, SsrDocumentStructureAndSeoTags) {
    Widget sample_widget = Center(
        Column({
            Text("Welcome to PingCore").fontSize(30.0f).bold(),
            Text("Next.js-style cross-platform C++"),
            Button("Action").onClick([] {})
        })
    );

    SsrRenderOptions opts;
    opts.title = "PingCore Enterprise App";
    opts.description = "Production SSR and SEO Engine for Native C++";
    opts.canonical_url = "https://app.pingcore.dev/home";

    std::string html = SsrHtmlRenderer::render_document(sample_widget, opts);

    // Check DOCTYPE and standard tags
    EXPECT_NE(html.find("<!doctype html>"), std::string::npos);
    EXPECT_NE(html.find("<title>PingCore Enterprise App</title>"), std::string::npos);
    EXPECT_NE(html.find("<meta name=\"description\" content=\"Production SSR and SEO Engine for Native C++\" />"), std::string::npos);
    EXPECT_NE(html.find("<meta property=\"og:title\" content=\"PingCore Enterprise App\" />"), std::string::npos);
    EXPECT_NE(html.find("<link rel=\"canonical\" href=\"https://app.pingcore.dev/home\" />"), std::string::npos);
    EXPECT_NE(html.find("id=\"__pingcore_root\""), std::string::npos);
    EXPECT_NE(html.find("<style>"), std::string::npos);
    EXPECT_NE(html.find(".pingcore-center"), std::string::npos);
    EXPECT_NE(html.find("/_pingcore/client.js"), std::string::npos);
}

TEST_F(Pkg120WebSsrTest, StaticSiteGenerationWithSitemapAndRobots) {
    Router test_router;
    test_router.register_page("/", []() -> Widget { return Center(Text("Home")); });
    test_router.register_page("/login", []() -> Widget { return Center(Text("Login")); });
    test_router.register_page("/dashboard", []() -> Widget { return Center(Text("Dashboard")); });

    std::filesystem::path ssg_dir = "dist/test_ssg_output";
    bool success = SsrHtmlRenderer::generate_static_site(test_router, ssg_dir, "MyPingCoreApp");
    ASSERT_TRUE(success);

    // Verify generated HTML pages
    EXPECT_TRUE(std::filesystem::exists(ssg_dir / "index.html"));
    EXPECT_TRUE(std::filesystem::exists(ssg_dir / "login.html"));
    EXPECT_TRUE(std::filesystem::exists(ssg_dir / "dashboard.html"));

    // Verify sitemap.xml
    std::filesystem::path sm_path = ssg_dir / "sitemap.xml";
    ASSERT_TRUE(std::filesystem::exists(sm_path));
    std::ifstream sm(sm_path);
    std::string sm_content((std::istreambuf_iterator<char>(sm)), std::istreambuf_iterator<char>());
    EXPECT_NE(sm_content.find("<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">"), std::string::npos);
    EXPECT_NE(sm_content.find("<loc>https://example.com/</loc>"), std::string::npos);
    EXPECT_NE(sm_content.find("<loc>https://example.com/login</loc>"), std::string::npos);
    EXPECT_NE(sm_content.find("<loc>https://example.com/dashboard</loc>"), std::string::npos);

    // Verify robots.txt
    std::filesystem::path rb_path = ssg_dir / "robots.txt";
    ASSERT_TRUE(std::filesystem::exists(rb_path));
    std::ifstream rb(rb_path);
    std::string rb_content((std::istreambuf_iterator<char>(rb)), std::istreambuf_iterator<char>());
    EXPECT_NE(rb_content.find("User-agent: *"), std::string::npos);
    EXPECT_NE(rb_content.find("Allow: /"), std::string::npos);
    EXPECT_NE(rb_content.find("Sitemap: https://example.com/sitemap.xml"), std::string::npos);
}
