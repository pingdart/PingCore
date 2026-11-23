#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pingcore::web {

struct OpenGraphMetadata {
  std::optional<std::string> title;
  std::optional<std::string> description;
  std::optional<std::string> type;
  std::optional<std::string> image;
  std::optional<std::string> image_alt;
  std::optional<std::string> site_name;
  std::optional<std::string> locale;
  std::optional<std::string> url;
};

struct TwitterCardMetadata {
  std::optional<std::string> card;
  std::optional<std::string> title;
  std::optional<std::string> description;
  std::optional<std::string> image;
  std::optional<std::string> image_alt;
  std::optional<std::string> site;
  std::optional<std::string> creator;
};

struct SeoMetadata {
  std::optional<std::string> title;
  std::optional<std::string> description;
  std::optional<std::string> canonical;
  std::optional<std::string> robots;
  std::optional<std::string> language;
  std::optional<std::string> theme_color;
  OpenGraphMetadata open_graph;
  TwitterCardMetadata twitter;
};

struct JsonLd {
  std::string schema_type;
  std::string data; // JSON object encoded without surrounding braces.
};

struct HtmlAttribute {
  std::string name;
  std::string value;
};

struct HtmlNode {
  std::string tag;
  std::vector<HtmlAttribute> attributes;
  std::vector<HtmlNode> children;
  std::string text;
  bool raw_html = false;

  static HtmlNode Element(std::string tag);
  static HtmlNode Text(std::string text);
  static HtmlNode Raw(std::string html);

  HtmlNode& Attr(std::string name, std::string value);
  HtmlNode& Add(HtmlNode child);
};

struct HydrationOptions {
  std::string root_id = "pingcore-root";
  std::string module_url;
  std::string route;
  std::string state_json = "{}";
};

struct HtmlDocumentOptions {
  std::string language = "en";
  std::string base_url;
  std::string module_url;
  std::string root_id = "pingcore-root";
  std::string state_json = "{}";
  bool include_hydration_script = true;
};

class HtmlRenderer {
public:
  [[nodiscard]] static std::string EscapeText(std::string_view text);
  [[nodiscard]] static std::string EscapeAttribute(std::string_view value);
  [[nodiscard]] static std::string Render(const HtmlNode& node);
};

class SeoDocument {
public:
  SeoMetadata metadata;
  std::vector<JsonLd> structured_data;
  HtmlNode content;

  [[nodiscard]] std::string Render(const HtmlDocumentOptions& options = {}) const;
};

struct SitemapEntry {
  std::string url;
  std::optional<std::string> last_modified;
  std::optional<std::string> change_frequency;
  std::optional<double> priority;
};

class Sitemap {
public:
  std::string base_url;
  std::vector<SitemapEntry> entries;

  [[nodiscard]] std::string Render() const;
};

struct RobotsRule {
  std::string user_agent = "*";
  std::vector<std::string> allow;
  std::vector<std::string> disallow;
};

struct RobotsTxt {
  std::vector<RobotsRule> rules;
  std::optional<std::string> sitemap_url;

  [[nodiscard]] std::string Render() const;
};

struct SeoRoute {
  std::string path;
  SeoMetadata metadata;
  std::vector<JsonLd> structured_data;
  HtmlNode content;
};

class SsgSite {
public:
  std::string output_directory;
  std::string base_url;
  std::vector<SeoRoute> routes;
  Sitemap sitemap;
  RobotsTxt robots;

  [[nodiscard]] bool Generate() const;
};

class SsrResult {
public:
  SeoDocument document;
  [[nodiscard]] std::string Render(const HtmlDocumentOptions& options = {}) const;
};

inline HtmlNode Main(HtmlNode content) {
  HtmlNode node = HtmlNode::Element("main");
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Article(HtmlNode content) {
  HtmlNode node = HtmlNode::Element("article");
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Section(HtmlNode content) {
  HtmlNode node = HtmlNode::Element("section");
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Nav(HtmlNode content) {
  HtmlNode node = HtmlNode::Element("nav");
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Header(HtmlNode content) {
  HtmlNode node = HtmlNode::Element("header");
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Footer(HtmlNode content) {
  HtmlNode node = HtmlNode::Element("footer");
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Aside(HtmlNode content) {
  HtmlNode node = HtmlNode::Element("aside");
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Div(HtmlNode content) {
  HtmlNode node = HtmlNode::Element("div");
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Span(HtmlNode content) {
  HtmlNode node = HtmlNode::Element("span");
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Heading(int level, HtmlNode content) {
  if (level < 1) level = 1;
  if (level > 6) level = 6;
  HtmlNode node = HtmlNode::Element("h" + std::to_string(level));
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Paragraph(HtmlNode content) {
  HtmlNode node = HtmlNode::Element("p");
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Button(HtmlNode content) {
  HtmlNode node = HtmlNode::Element("button");
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Link(std::string href, HtmlNode content) {
  HtmlNode node = HtmlNode::Element("a");
  node.Attr("href", std::move(href));
  node.Add(std::move(content));
  return node;
}
inline HtmlNode Image(std::string source, std::string alt) {
  HtmlNode node = HtmlNode::Element("img");
  node.Attr("src", std::move(source));
  node.Attr("alt", std::move(alt));
  return node;
}

class SeoRouter {
public:
  std::vector<SeoRoute> routes;

  [[nodiscard]] const SeoRoute* Match(std::string_view path) const {
    for (const auto& route : routes) {
      if (route.path == path) return &route;
    }
    for (const auto& route : routes) {
      if (MatchDynamic(route.path, path)) return &route;
    }
    return nullptr;
  }

private:
  [[nodiscard]] static bool MatchDynamic(std::string_view pattern, std::string_view path);
};

class StaticGenerator {
public:
  [[nodiscard]] static bool Generate(const SsgSite& site) {
    return site.Generate();
  }
};

class SsrServer {
public:
  SeoRouter router;

  [[nodiscard]] SsrResult Render(std::string_view path) const {
    SsrResult result;
    if (const SeoRoute* route = router.Match(path)) {
      result.document.metadata = route->metadata;
      result.document.structured_data = route->structured_data;
      result.document.content = route->content;
    }
    return result;
  }
};

} // namespace pingcore::web

namespace pingcore {
using SeoMetadata = web::SeoMetadata;
using OpenGraphMetadata = web::OpenGraphMetadata;
using TwitterCardMetadata = web::TwitterCardMetadata;
using JsonLd = web::JsonLd;
using SitemapEntry = web::SitemapEntry;
using Sitemap = web::Sitemap;
using RobotsTxt = web::RobotsTxt;
using SeoRoute = web::SeoRoute;
using SsgSite = web::SsgSite;
using SsrResult = web::SsrResult;
using HtmlNode = web::HtmlNode;
using HtmlRenderer = web::HtmlRenderer;
using SeoDocument = web::SeoDocument;
using SeoRouter = web::SeoRouter;
using StaticGenerator = web::StaticGenerator;
using SsrServer = web::SsrServer;
}

