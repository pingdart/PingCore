#include <catch2/catch_amalgamated.hpp>

#include <pingcore/pingcore.h>

using namespace pingcore;

TEST_CASE("SEO document renders crawlable HTML and metadata", "[seo][web]") {
  SeoDocument document;
  document.metadata.title = "Example Page";
  document.metadata.description = "An SEO-friendly page";
  document.metadata.canonical = "https://example.com/page";
  document.metadata.robots = "index,follow";
  document.metadata.open_graph.title = "Example Page";
  document.metadata.twitter.card = "summary_large_image";
  document.structured_data.push_back({"Article", "\"headline\":\"Example Page\",\"author\":\"PingCore\""});
  document.content = HtmlNode::Element("article");
  document.content.Add(HtmlNode::Element("h1").Add(HtmlNode::Text("Example & useful")));

  const std::string html = document.Render();

  REQUIRE(html.find("<title>Example Page</title>") != std::string::npos);
  REQUIRE(html.find("name=\"description\"") != std::string::npos);
  REQUIRE(html.find("rel=\"canonical\"") != std::string::npos);
  REQUIRE(html.find("property=\"og:title\"") != std::string::npos);
  REQUIRE(html.find("name=\"twitter:card\"") != std::string::npos);
  REQUIRE(html.find("application/ld+json") != std::string::npos);
  REQUIRE(html.find("<h1>Example &amp; useful</h1>") != std::string::npos);
  REQUIRE(html.find("data-pingcore-hydration=\"true\"") != std::string::npos);
}

TEST_CASE("Sitemap and robots produce valid crawl directives", "[seo][web]") {
  Sitemap sitemap;
  sitemap.entries.push_back({
      "https://example.com/",
      "2026-11-23",
      "weekly",
      1.0,
  });

  REQUIRE(sitemap.Render().find("<urlset") != std::string::npos);
  REQUIRE(sitemap.Render().find("<loc>https://example.com/</loc>") != std::string::npos);
  REQUIRE(sitemap.Render().find("<changefreq>weekly</changefreq>") != std::string::npos);

  RobotsTxt robots;
  robots.rules.push_back({"*", {"/public/"}, {"/private/"}});
  robots.sitemap_url = "https://example.com/sitemap.xml";
  const std::string text = robots.Render();

  REQUIRE(text.find("Allow: /public/") != std::string::npos);
  REQUIRE(text.find("Disallow: /private/") != std::string::npos);
  REQUIRE(text.find("Sitemap: https://example.com/sitemap.xml") != std::string::npos);
}

