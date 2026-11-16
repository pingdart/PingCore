#pragma once

#include <pingdart/web/i_web_renderer.h>

namespace pingdart::web {

class DefaultWebRenderer final : public IWebRenderer {
public:
    DefaultWebRenderer() = default;
    ~DefaultWebRenderer() override = default;

    [[nodiscard]] std::string render_document(
        const SeoMetadata& seo,
        std::string_view body_content) const override;

    [[nodiscard]] std::string render_sitemap(
        const std::vector<std::pair<std::string, std::string>>& routes_and_lastmod) const override;

    [[nodiscard]] std::string render_robots_txt(
        bool allow_indexing,
        std::string_view sitemap_url = "") const override;
};

} // namespace pingdart::web
