#pragma once

#include "seo_metadata.h"
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace pingdart::web {

class IWebRenderer {
public:
    virtual ~IWebRenderer() = default;

    // Produces a complete, crawlable HTML5 document with <head> SEO metadata and <body> content
    [[nodiscard]] virtual std::string render_document(
        const SeoMetadata& seo,
        std::string_view body_content) const = 0;

    // Generates standard sitemap.xml for search engines
    [[nodiscard]] virtual std::string render_sitemap(
        const std::vector<std::pair<std::string, std::string>>& routes_and_lastmod) const = 0;

    // Generates standard robots.txt
    [[nodiscard]] virtual std::string render_robots_txt(
        bool allow_indexing,
        std::string_view sitemap_url = "") const = 0;
};

} // namespace pingdart::web
