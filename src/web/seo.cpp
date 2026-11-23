#include "pingcore/web/seo.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace pingcore::web {
namespace {

std::string Escape(std::string_view value) {
  std::string result;
  result.reserve(value.size());
  for (const char character : value) {
    switch (character) {
      case '&': result += "&amp;"; break;
      case '<': result += "&lt;"; break;
      case '>': result += "&gt;"; break;
      case '"': result += "&quot;"; break;
      case '\'': result += "&#39;"; break;
      default: result += character; break;
    }
  }
  return result;
}

std::string Attribute(std::string_view name, std::string_view value) {
  if (value.empty()) return "";
  return " " + std::string(name) + "=\"" + HtmlRenderer::EscapeAttribute(value) + "\"";
}

std::string SitemapDate(std::string_view value) {
  return Escape(value);
}

} // namespace

HtmlNode HtmlNode::Element(std::string tag) {
  HtmlNode node;
  node.tag = std::move(tag);
  return node;
}

HtmlNode HtmlNode::Text(std::string text_value) {
  HtmlNode node;
  node.text = std::move(text_value);
  return node;
}

HtmlNode HtmlNode::Raw(std::string html) {
  HtmlNode node;
  node.raw_html = true;
  node.text = std::move(html);
  return node;
}

HtmlNode& HtmlNode::Attr(std::string name, std::string value) {
  attributes.push_back({std::move(name), std::move(value)});
  return *this;
}

HtmlNode& HtmlNode::Add(HtmlNode child) {
  children.push_back(std::move(child));
  return *this;
}

std::string HtmlRenderer::EscapeText(std::string_view text) {
  return Escape(text);
}

std::string HtmlRenderer::EscapeAttribute(std::string_view value) {
  return Escape(value);
}

std::string HtmlRenderer::Render(const HtmlNode& node) {
  if (node.raw_html) return node.text;
  if (node.tag.empty()) return Escape(node.text);

  std::ostringstream output;
  output << "<" << node.tag;
  for (const auto& attribute : node.attributes) {
    output << Attribute(attribute.name, attribute.value);
  }
  output << ">";
  output << Escape(node.text);
  for (const auto& child : node.children) output << Render(child);

  static const std::vector<std::string> void_elements{
      "area", "base", "br", "col", "embed", "hr", "img", "input",
      "link", "meta", "source", "track", "wbr"};
  if (std::find(void_elements.begin(), void_elements.end(), node.tag) == void_elements.end()) {
    output << "</" << node.tag << ">";
  }
  return output.str();
}

std::string SeoDocument::Render(const HtmlDocumentOptions& options) const {
  std::ostringstream head;
  const std::string language = metadata.language.value_or(options.language);
  if (metadata.title) head << "<title>" << Escape(metadata.title.value()) << "</title>\n";
  if (metadata.description) {
    head << "<meta name=\"description\" content=\"" << Escape(metadata.description.value()) << "\">\n";
  }
  if (metadata.canonical) {
    head << "<link rel=\"canonical\" href=\"" << Escape(metadata.canonical.value()) << "\">\n";
  }
  if (metadata.robots) {
    head << "<meta name=\"robots\" content=\"" << Escape(metadata.robots.value()) << "\">\n";
  }
  if (metadata.theme_color) {
    head << "<meta name=\"theme-color\" content=\"" << Escape(metadata.theme_color.value()) << "\">\n";
  }
  if (metadata.open_graph.title) {
    head << "<meta property=\"og:title\" content=\"" << Escape(metadata.open_graph.title.value()) << "\">\n";
  }
  if (metadata.open_graph.description) {
    head << "<meta property=\"og:description\" content=\"" << Escape(metadata.open_graph.description.value()) << "\">\n";
  }
  if (metadata.open_graph.type) {
    head << "<meta property=\"og:type\" content=\"" << Escape(metadata.open_graph.type.value()) << "\">\n";
  }
  if (metadata.open_graph.image) {
    head << "<meta property=\"og:image\" content=\"" << Escape(metadata.open_graph.image.value()) << "\">\n";
  }
  if (metadata.open_graph.image_alt) {
    head << "<meta property=\"og:image:alt\" content=\"" << Escape(metadata.open_graph.image_alt.value()) << "\">\n";
  }
  if (metadata.open_graph.site_name) {
    head << "<meta property=\"og:site_name\" content=\"" << Escape(metadata.open_graph.site_name.value()) << "\">\n";
  }
  if (metadata.open_graph.locale) {
    head << "<meta property=\"og:locale\" content=\"" << Escape(metadata.open_graph.locale.value()) << "\">\n";
  }
  if (metadata.open_graph.url) {
    head << "<meta property=\"og:url\" content=\"" << Escape(metadata.open_graph.url.value()) << "\">\n";
  }
  if (metadata.twitter.card) {
    head << "<meta name=\"twitter:card\" content=\"" << Escape(metadata.twitter.card.value()) << "\">\n";
  }
  if (metadata.twitter.title) {
    head << "<meta name=\"twitter:title\" content=\"" << Escape(metadata.twitter.title.value()) << "\">\n";
  }
  if (metadata.twitter.description) {
    head << "<meta name=\"twitter:description\" content=\"" << Escape(metadata.twitter.description.value()) << "\">\n";
  }
  if (metadata.twitter.image) {
    head << "<meta name=\"twitter:image\" content=\"" << Escape(metadata.twitter.image.value()) << "\">\n";
  }
  if (metadata.twitter.image_alt) {
    head << "<meta name=\"twitter:image:alt\" content=\"" << Escape(metadata.twitter.image_alt.value()) << "\">\n";
  }
  if (metadata.twitter.site) {
    head << "<meta name=\"twitter:site\" content=\"" << Escape(metadata.twitter.site.value()) << "\">\n";
  }
  if (metadata.twitter.creator) {
    head << "<meta name=\"twitter:creator\" content=\"" << Escape(metadata.twitter.creator.value()) << "\">\n";
  }
  std::ostringstream html;
  html << "<!doctype html>\n<html lang=\"" << Escape(language) << "\">\n<head>\n"
       << "<meta charset=\"utf-8\">\n"
       << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
       << head.str();
  for (const auto& item : structured_data) {
    html << "<script type=\"application/ld+json\">";
    html << "{\"@context\":\"https://schema.org\",\"@type\":\"" << Escape(item.schema_type) << "\"";
    if (!item.data.empty()) {
      if (item.data.front() == '{' && item.data.back() == '}') {
        html << item.data.substr(1);
      } else {
        html << "," << item.data << "}";
      }
    } else {
      html << "}";
    }
    html << "</script>\n";
  }
  html << "</head>\n<body>\n";
  html << "<main id=\"" << Escape(options.root_id) << "\" data-pingcore-hydration=\"true\">";
  html << HtmlRenderer::Render(content);
  html << "</main>\n";
  if (options.include_hydration_script) {
    html << "<script type=\"application/json\" id=\"pingcore-state\">"
         << options.state_json << "</script>\n";
    html << "<script type=\"module\">\n";
    if (!options.module_url.empty()) {
      html << "import createPingCoreApp from " << '"' << Escape(options.module_url) << "\";\n";
    }
    html << "const state=document.getElementById('pingcore-state')?.textContent||'{}';\n";
    html << "window.pingcoreHydrationState=JSON.parse(state);\n";
    if (!options.module_url.empty()) {
      html << "const module=await createPingCoreApp({pingcoreHydration:true});\n";
      html << "if(module.mountPingCore) module.mountPingCore('#" << Escape(options.root_id) << "');\n";
    }
    html << "</script>\n";
  }
  html << "</body>\n</html>\n";
  return html.str();
}

std::string Sitemap::Render() const {
  std::ostringstream output;
  output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  output << "<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n";
  for (const auto& entry : entries) {
    output << "  <url><loc>" << Escape(entry.url) << "</loc>";
    if (entry.last_modified) output << "<lastmod>" << SitemapDate(*entry.last_modified) << "</lastmod>";
    if (entry.change_frequency) output << "<changefreq>" << SitemapDate(*entry.change_frequency) << "</changefreq>";
    if (entry.priority) {
      std::ostringstream priority;
      priority.precision(2);
      priority << std::fixed << *entry.priority;
      output << "<priority>" << priority.str() << "</priority>";
    }
    output << "</url>\n";
  }
  output << "</urlset>\n";
  return output.str();
}

std::string RobotsTxt::Render() const {
  std::ostringstream output;
  if (rules.empty()) {
    output << "User-agent: *\nAllow: /\n";
  } else {
    for (const auto& rule : rules) {
      output << "User-agent: " << rule.user_agent << "\n";
      for (const auto& allow : rule.allow) output << "Allow: " << allow << "\n";
      for (const auto& disallow : rule.disallow) output << "Disallow: " << disallow << "\n";
    }
  }
  if (sitemap_url) output << "Sitemap: " << *sitemap_url << "\n";
  return output.str();
}

bool SsgSite::Generate() const {
  std::error_code error;
  std::filesystem::create_directories(output_directory, error);
  if (error) return false;
  Sitemap generated_sitemap = sitemap;
  if (generated_sitemap.entries.empty()) {
    for (const auto& route : routes) {
      std::string url = base_url;
      if (!url.empty() && url.back() == '/') url.pop_back();
      if (!route.path.empty() && route.path.front() != '/') url += "/";
      else url += route.path;
      generated_sitemap.entries.push_back({url, std::nullopt, std::nullopt, std::nullopt});
    }
  }
  for (const auto& route : routes) {
    SeoDocument document;
    document.metadata = route.metadata;
    document.structured_data = route.structured_data;
    document.content = route.content;
    std::string relative = route.path;
    if (relative.empty() || relative == "/") relative = "index.html";
    else {
      if (relative.front() == '/') relative.erase(relative.begin());
      if (relative.find('.') == std::string::npos) relative += "/index.html";
    }
    std::filesystem::path file = std::filesystem::path(output_directory) / relative;
    std::filesystem::create_directories(file.parent_path(), error);
    if (error) return false;
    HtmlDocumentOptions options;
    options.base_url = base_url;
    options.language = route.metadata.language.value_or("en");
    std::ofstream output(file, std::ios::binary | std::ios::trunc);
    if (!output) return false;
    output << document.Render(options);
  }
  {
    std::ofstream output(std::filesystem::path(output_directory) / "sitemap.xml", std::ios::binary | std::ios::trunc);
    if (!output) return false;
    output << generated_sitemap.Render();
  }
  {
    std::ofstream output(std::filesystem::path(output_directory) / "robots.txt", std::ios::binary | std::ios::trunc);
    if (!output) return false;
    RobotsTxt generated_robots = robots;
    if (!generated_robots.sitemap_url && !base_url.empty()) {
      generated_robots.sitemap_url = base_url;
      if (generated_robots.sitemap_url->back() == '/') generated_robots.sitemap_url->append("sitemap.xml");
      else generated_robots.sitemap_url->append("/sitemap.xml");
    }
    output << generated_robots.Render();
  }
  return true;
}

bool SeoRouter::MatchDynamic(std::string_view pattern, std::string_view path) {
  if (pattern.empty() || path.empty()) return pattern == path;
  std::size_t pattern_position = 0;
  std::size_t path_position = 0;
  while (pattern_position < pattern.size() && path_position < path.size()) {
    if (pattern[pattern_position] == ':') {
      while (pattern_position < pattern.size() && pattern[pattern_position] != '/') ++pattern_position;
      while (path_position < path.size() && path[path_position] != '/') ++path_position;
      if (pattern_position < pattern.size() && pattern[pattern_position] == '/') {
        ++pattern_position;
        if (path_position < path.size() && path[path_position] == '/') ++path_position;
      }
      continue;
    }
    if (pattern[pattern_position] != path[path_position]) return false;
    ++pattern_position;
    ++path_position;
  }
  return pattern_position == pattern.size() && path_position == path.size();
}

std::string SsrResult::Render(const HtmlDocumentOptions& options) const {
  return document.Render(options);
}

} // namespace pingcore::web
