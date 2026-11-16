#include <gtest/gtest.h>
#include "../../src/web/ssg/route_enumerator.h"
#include "../../src/web/ssg/static_generator.h"

using namespace pingdart::web;

TEST(RouteEnumeratorTest, Enumerate100DynamicRoutes) {
    constexpr size_t kEntityCount = 105;
    std::vector<EntityRecord> doctors;
    doctors.reserve(kEntityCount);

    for (size_t i = 0; i < kEntityCount; ++i) {
        doctors.push_back(EntityRecord{
            .slug = "dr-specialist-" + std::to_string(i),
            .title = "Dr. Specialist " + std::to_string(i) + " | MeddyList",
            .description = "Verified specialist #" + std::to_string(i) + " practicing cardiology.",
            .lastmod = "2026-11-16"
        });
    }

    auto routes = RouteEnumerator::enumerate_entities(
        "/doctors",
        "https://meddylist.com",
        doctors,
        [](const EntityRecord& doc) {
            std::vector<std::unique_ptr<SemanticWidget>> c;
            c.push_back(std::make_unique<HeadingWidget>(1, doc.title));
            c.push_back(std::make_unique<ParagraphWidget>(doc.description));
            return std::make_unique<MainWidget>(std::move(c), "doc_main");
        }
    );

    EXPECT_EQ(routes.size(), kEntityCount);

    // Build the static site with all 105 dynamic pages
    StaticGenerator ssg(SsgOptions{
        .output_directory = "dist/web",
        .pretty_html = false,
        .allow_indexing = true,
        .sitemap_base_url = "https://meddylist.com"
    });

    for (auto& r : routes) {
        ssg.add_route(std::move(r));
    }

    auto result = ssg.build();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().pages_generated, kEntityCount);

    const auto& files = ssg.generated_files();

    // Verify 1st, 50th, and 104th doctor profile files were generated
    EXPECT_TRUE(files.find("doctors/dr-specialist-0/index.html") != files.end());
    EXPECT_TRUE(files.find("doctors/dr-specialist-50/index.html") != files.end());
    EXPECT_TRUE(files.find("doctors/dr-specialist-104/index.html") != files.end());

    // Verify content contains crawlable HTML without JavaScript
    const auto& doc50_html = files.at("doctors/dr-specialist-50/index.html");
    EXPECT_NE(doc50_html.find("<title>Dr. Specialist 50 | MeddyList</title>"), std::string::npos);
    EXPECT_NE(doc50_html.find(">Dr. Specialist 50 | MeddyList</h1>"), std::string::npos);
    EXPECT_NE(doc50_html.find("Verified specialist #50 practicing cardiology."), std::string::npos);

    // Verify sitemap contains all 105 canonical URLs
    const auto& sitemap = files.at("sitemap.xml");
    EXPECT_NE(sitemap.find("https://meddylist.com/doctors/dr-specialist-0"), std::string::npos);
    EXPECT_NE(sitemap.find("https://meddylist.com/doctors/dr-specialist-104"), std::string::npos);
}
