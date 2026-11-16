#include <gtest/gtest.h>
#include <pingdart/web/static_route.h>
#include <pingdart/web/semantic_widget.h>
#include "../../src/web/ssg/static_generator.h"

using namespace pingdart::web;

TEST(StaticGeneratorTest, RouteMappingToRelativePaths) {
    EXPECT_EQ(StaticGenerator::route_to_relative_file_path("/"), "index.html");
    EXPECT_EQ(StaticGenerator::route_to_relative_file_path(""), "index.html");
    EXPECT_EQ(StaticGenerator::route_to_relative_file_path("/about"), "about/index.html");
    EXPECT_EQ(StaticGenerator::route_to_relative_file_path("/about/"), "about/index.html");
    EXPECT_EQ(StaticGenerator::route_to_relative_file_path("/doctors/jane-smith"), "doctors/jane-smith/index.html");
}

TEST(StaticGeneratorTest, BasicSiteGenerationAndAssetFingerprinting) {
    StaticGenerator ssg(SsgOptions{
        .output_directory = "dist/web",
        .pretty_html = false,
        .allow_indexing = true,
        .sitemap_base_url = "https://meddylist.com"
    });

    // 1. Add Homepage
    ssg.add_route(StaticRoute{
        .path = "/",
        .seo = SeoMetadata{.title = "MeddyList Home", .description = "Find verified doctors."},
        .widget_builder = []() {
            std::vector<std::unique_ptr<SemanticWidget>> c;
            c.push_back(std::make_unique<HeadingWidget>(1, "Welcome to MeddyList"));
            return std::make_unique<MainWidget>(std::move(c), "home_main");
        },
        .lastmod = "2026-11-16",
        .is_canonical = true,
        .is_indexable = true
    });

    // 2. Add About page
    ssg.add_route(StaticRoute{
        .path = "/about",
        .seo = SeoMetadata{.title = "About MeddyList", .description = "Our mission and team."},
        .widget_builder = []() {
            std::vector<std::unique_ptr<SemanticWidget>> c;
            c.push_back(std::make_unique<ParagraphWidget>("Connecting patients and doctors."));
            return std::make_unique<MainWidget>(std::move(c), "about_main");
        },
        .lastmod = "2026-11-15",
        .is_canonical = true,
        .is_indexable = true
    });

    // 3. Add CSS and WASM assets for fingerprinting
    ssg.add_asset("app.css", "body { margin: 0; font-family: sans-serif; }");
    ssg.add_asset("app.wasm", "\0asm\1\0\0\0");

    auto result = ssg.build();
    ASSERT_TRUE(result.has_value());

    const auto& res = result.value();
    EXPECT_EQ(res.pages_generated, 2u);
    EXPECT_EQ(res.assets_fingerprinted, 2u);

    const auto& files = ssg.generated_files();

    // Verify HTML pages exist
    ASSERT_TRUE(files.find("index.html") != files.end());
    EXPECT_NE(files.at("index.html").find("Welcome to MeddyList"), std::string::npos);
    EXPECT_NE(files.at("index.html").find("<title>MeddyList Home</title>"), std::string::npos);

    ASSERT_TRUE(files.find("about/index.html") != files.end());
    EXPECT_NE(files.at("about/index.html").find("Connecting patients and doctors."), std::string::npos);

    // Verify fingerprinted assets exist
    bool found_css = false;
    bool found_wasm = false;
    for (const auto& [name, _] : files) {
        if (name.find("assets/app.") == 0 && name.find(".css") != std::string::npos) {
            found_css = true;
        }
        if (name.find("assets/app.") == 0 && name.find(".wasm") != std::string::npos) {
            found_wasm = true;
        }
    }
    EXPECT_TRUE(found_css);
    EXPECT_TRUE(found_wasm);

    // Verify sitemap and robots
    ASSERT_TRUE(files.find("sitemap.xml") != files.end());
    EXPECT_NE(files.at("sitemap.xml").find("https://meddylist.com/about"), std::string::npos);

    ASSERT_TRUE(files.find("robots.txt") != files.end());
    EXPECT_NE(files.at("robots.txt").find("Allow: /"), std::string::npos);

    // Verify build manifest
    ASSERT_TRUE(files.find("build-manifest.json") != files.end());
    EXPECT_NE(files.at("build-manifest.json").find("\"pingdart_version\""), std::string::npos);
}
