#include "default_web_renderer.h"

namespace pingdart::web {

std::string DefaultWebRenderer::render_document(
    const SeoMetadata& seo,
    std::string_view body_content) const {
    std::string doc;
    doc.reserve(body_content.size() + 1024);

    doc.append("<!doctype html>\n");
    doc.append("<html lang=\"");
    doc.append(seo.locale.empty() ? "en" : seo.locale.substr(0, 2));
    doc.append("\">\n");

    doc.append("<head>\n");
    doc.append("  <meta charset=\"utf-8\">\n");
    doc.append("  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n");
    doc.append(seo.render_head_tags());
    doc.append("</head>\n");

    doc.append("<body>\n");
    doc.append(body_content);
    if (!body_content.empty() && body_content.back() != '\n') {
        doc.push_back('\n');
    }
    doc.append("</body>\n");
    doc.append("</html>\n");

    return doc;
}

std::string DefaultWebRenderer::render_sitemap(
    const std::vector<std::pair<std::string, std::string>>& routes_and_lastmod) const {
    std::string xml;
    xml.reserve(1024);

    xml.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    xml.append("<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n");

    for (const auto& [loc, lastmod] : routes_and_lastmod) {
        xml.append("  <url>\n");
        xml.append("    <loc>");
        xml.append(SeoMetadata::escape_html(loc));
        xml.append("</loc>\n");
        if (!lastmod.empty()) {
            xml.append("    <lastmod>");
            xml.append(SeoMetadata::escape_html(lastmod));
            xml.append("</lastmod>\n");
        }
        xml.append("  </url>\n");
    }

    xml.append("</urlset>\n");
    return xml;
}

std::string DefaultWebRenderer::render_robots_txt(
    bool allow_indexing,
    std::string_view sitemap_url) const {
    std::string txt;
    txt.reserve(256);

    txt.append("User-agent: *\n");
    if (allow_indexing) {
        txt.append("Allow: /\n");
        if (!sitemap_url.empty()) {
            txt.append("Sitemap: ");
            txt.append(sitemap_url);
            txt.push_back('\n');
        }
    } else {
        txt.append("Disallow: /\n");
    }

    return txt;
}

} // namespace pingdart::web
