#pragma once

#include <pingdart/web/static_route.h>
#include <vector>
#include <string>
#include <functional>

namespace pingdart::web {

struct EntityRecord {
    std::string slug;
    std::string title;
    std::string description;
    std::string lastmod;
};

using EntityWidgetBuilder = std::function<std::unique_ptr<SemanticWidget>(const EntityRecord&)>;

class RouteEnumerator {
public:
    // Enumerates dynamic paths (e.g. /doctors/{slug}) into fully configured StaticRoute instances
    static std::vector<StaticRoute> enumerate_entities(
        const std::string& route_prefix,
        const std::string& base_canonical_url,
        const std::vector<EntityRecord>& entities,
        EntityWidgetBuilder builder);
};

} // namespace pingdart::web
