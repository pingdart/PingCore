#include <gtest/gtest.h>
#include <pingdart/web/seo_metadata.h>
#include <pingdart/web/i_html_serializer.h>
#include <pingdart/web/i_web_renderer.h>
#include "../../src/web/html/html_serializer.h"
#include "../../src/web/ssr/default_web_renderer.h"

using namespace pingdart::web;

TEST(WebSeoTest, SeoMetadataHeadGeneration) {
    SeoMetadata seo{
        .title = "Find Doctors | MeddyList",
        .description = "Browse verified doctor profiles & clinics near you.",
        .canonical_url = "https://example.com/doctors",
        .robots = "index, follow",
        .locale = "en_US",
        .keywords = "doctors, clinics, cardiology"
    };

    seo.alternate_locales.push_back({"es", "https://example.com/es/doctors"});

    seo.open_graph = OpenGraphMetadata{
        .title = "Find Doctors | MeddyList",
        .description = "Browse verified doctor profiles & clinics near you.",
        .url = "https://example.com/doctors",
        .image_url = "https://example.com/og-doctors.png",
        .type = "website",
        .site_name = "MeddyList"
    };

    seo.twitter_card = TwitterCardMetadata{
        .card = "summary_large_image",
        .site = "@meddylist",
        .creator = "@meddylist",
        .title = "Find Doctors | MeddyList",
        .description = "Browse verified doctor profiles & clinics near you.",
        .image_url = "https://example.com/twitter-doctors.png"
    };

    std::string head_tags = seo.render_head_tags();

    EXPECT_NE(head_tags.find("<title>Find Doctors | MeddyList</title>"), std::string::npos);
    EXPECT_NE(head_tags.find("<meta name=\"description\" content=\"Browse verified doctor profiles &amp; clinics near you.\">"), std::string::npos);
    EXPECT_NE(head_tags.find("<link rel=\"canonical\" href=\"https://example.com/doctors\">"), std::string::npos);
    EXPECT_NE(head_tags.find("<link rel=\"alternate\" hreflang=\"es\" href=\"https://example.com/es/doctors\">"), std::string::npos);
    EXPECT_NE(head_tags.find("<meta property=\"og:title\" content=\"Find Doctors | MeddyList\">"), std::string::npos);
    EXPECT_NE(head_tags.find("<meta property=\"og:image\" content=\"https://example.com/og-doctors.png\">"), std::string::npos);
    EXPECT_NE(head_tags.find("<meta name=\"twitter:card\" content=\"summary_large_image\">"), std::string::npos);
    EXPECT_NE(head_tags.find("<meta name=\"twitter:site\" content=\"@meddylist\">"), std::string::npos);
}

TEST(WebSeoTest, JsonLdStructuredDataInjection) {
    SeoMetadata seo{
        .title = "Dr. Jane Smith, MD | Cardiologist",
        .description = "Cardiology specialist with 15 years experience."
    };

    std::string doctor_schema = R"({
  "@context": "https://schema.org",
  "@type": "Physician",
  "name": "Dr. Jane Smith",
  "medicalSpecialty": "Cardiology",
  "telephone": "+1-555-0199",
  "address": {
    "@type": "PostalAddress",
    "addressLocality": "New York",
    "addressRegion": "NY"
  }
})";

    seo.json_ld_schemas.push_back(doctor_schema);

    std::string head_tags = seo.render_head_tags();
    EXPECT_NE(head_tags.find("<script type=\"application/ld+json\">"), std::string::npos);
    EXPECT_NE(head_tags.find("\"@type\": \"Physician\""), std::string::npos);
    EXPECT_NE(head_tags.find("\"name\": \"Dr. Jane Smith\""), std::string::npos);
}

TEST(WebSeoTest, HtmlSerializerSemanticTree) {
    HtmlSerializer serializer(true);

    serializer.open_tag("main", {{"role", "main"}});
    serializer.open_tag("article", {{"class", "pd-doctor-card"}});
    serializer.open_tag("header");
    serializer.open_tag("h1");
    serializer.write_text("Dr. Jane Smith");
    serializer.close_tag("h1");
    serializer.close_tag("header");

    serializer.open_tag("p");
    serializer.write_text("Cardiology & Pediatric Care");
    serializer.close_tag("p");

    serializer.open_tag("img", {
        {"src", "/images/dr-jane.jpg"},
        {"alt", "Photo of Dr. Jane Smith"},
        {"loading", "lazy"}
    }, /*self_closing=*/true);

    serializer.open_tag("a", {{"href", "/doctors/jane-smith"}, {"class", "pd-btn"}});
    serializer.write_text("View Profile");
    serializer.close_tag("a");

    serializer.close_tag("article");
    serializer.close_tag("main");

    std::string html = serializer.to_string();

    EXPECT_NE(html.find("<main role=\"main\">"), std::string::npos);
    EXPECT_NE(html.find("<article class=\"pd-doctor-card\">"), std::string::npos);
    EXPECT_NE(html.find("<h1>"), std::string::npos);
    EXPECT_NE(html.find("Dr. Jane Smith"), std::string::npos);
    EXPECT_NE(html.find("Cardiology &amp; Pediatric Care"), std::string::npos);
    EXPECT_NE(html.find("<img src=\"/images/dr-jane.jpg\" alt=\"Photo of Dr. Jane Smith\" loading=\"lazy\" />"), std::string::npos);
    EXPECT_NE(html.find("<a href=\"/doctors/jane-smith\" class=\"pd-btn\">"), std::string::npos);
}

TEST(WebSeoTest, FullDocumentSsrRendering) {
    DefaultWebRenderer renderer;

    SeoMetadata seo{
        .title = "Find Doctors | MeddyList",
        .description = "Find doctors and clinics near you.",
        .canonical_url = "https://example.com/doctors",
        .locale = "en_US"
    };

    std::string body = R"(  <main>
    <h1>Find Doctors</h1>
    <p>Browse available doctor profiles.</p>
    <a href="/doctors/example-doctor">View doctor profile</a>
  </main>)";

    std::string document = renderer.render_document(seo, body);

    EXPECT_NE(document.find("<!doctype html>"), std::string::npos);
    EXPECT_NE(document.find("<html lang=\"en\">"), std::string::npos);
    EXPECT_NE(document.find("<meta charset=\"utf-8\">"), std::string::npos);
    EXPECT_NE(document.find("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"), std::string::npos);
    EXPECT_NE(document.find("<title>Find Doctors | MeddyList</title>"), std::string::npos);
    EXPECT_NE(document.find("<main>"), std::string::npos);
    EXPECT_NE(document.find("<h1>Find Doctors</h1>"), std::string::npos);
    EXPECT_NE(document.find("</html>"), std::string::npos);
}

TEST(WebSeoTest, SitemapXmlGeneration) {
    DefaultWebRenderer renderer;

    std::vector<std::pair<std::string, std::string>> routes = {
        {"https://example.com/", "2026-11-16"},
        {"https://example.com/doctors", "2026-11-16"},
        {"https://example.com/doctors/jane-smith", "2026-11-15"}
    };

    std::string sitemap = renderer.render_sitemap(routes);

    EXPECT_NE(sitemap.find("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"), std::string::npos);
    EXPECT_NE(sitemap.find("<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">"), std::string::npos);
    EXPECT_NE(sitemap.find("<loc>https://example.com/doctors</loc>"), std::string::npos);
    EXPECT_NE(sitemap.find("<lastmod>2026-11-16</lastmod>"), std::string::npos);
    EXPECT_NE(sitemap.find("<loc>https://example.com/doctors/jane-smith</loc>"), std::string::npos);
}

TEST(WebSeoTest, RobotsTxtGeneration) {
    DefaultWebRenderer renderer;

    std::string allowed = renderer.render_robots_txt(true, "https://example.com/sitemap.xml");
    EXPECT_NE(allowed.find("User-agent: *"), std::string::npos);
    EXPECT_NE(allowed.find("Allow: /"), std::string::npos);
    EXPECT_NE(allowed.find("Sitemap: https://example.com/sitemap.xml"), std::string::npos);

    std::string blocked = renderer.render_robots_txt(false);
    EXPECT_NE(blocked.find("User-agent: *"), std::string::npos);
    EXPECT_NE(blocked.find("Disallow: /"), std::string::npos);
    EXPECT_EQ(blocked.find("Allow: /"), std::string::npos);
}
