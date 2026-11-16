#include "route_enumerator.h"
#include <pingdart/web/url_security.h>

namespace pingdart::web {

std::vector<StaticRoute> RouteEnumerator::enumerate_entities(
    const std::string& route_prefix,
    const std::string& base_canonical_url,
    const std::vector<EntityRecord>& entities,
    EntityWidgetBuilder builder) {
    std::vector<StaticRoute> routes;
    routes.reserve(entities.size());

    std::string prefix = route_prefix;
    if (prefix.empty() || prefix.front() != '/') {
        prefix = "/" + prefix;
    }
    if (prefix.back() == '/') {
        prefix.pop_back();
    }

    for (const auto& entity : entities) {
        if (!UrlSecurity::is_safe_route_parameter(entity.slug)) {
            continue; // Skip invalid or unsafe slugs
        }

        std::string route_path = prefix + "/" + entity.slug;
        std::string canonical = base_canonical_url + route_path;

        SeoMetadata seo{
            .title = entity.title,
            .description = entity.description,
            .canonical_url = std::move(canonical),
            .robots = "index, follow"
        };

        StaticRoute route{
            .path = std::move(route_path),
            .seo = std::move(seo),
            .widget_builder = [&builder, entity]() {
                return builder(entity);
            },
            .lastmod = entity.lastmod,
            .is_canonical = true,
            .is_indexable = true
        };

        routes.push_back(std::move(route));
    }

    return routes;
}

} // namespace pingdart::web
