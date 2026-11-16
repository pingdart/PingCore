#pragma once

#include "seo_metadata.h"
#include "semantic_widget.h"
#include <string>
#include <functional>
#include <memory>

namespace pingdart::web {

struct StaticRoute {
    std::string path{"/"};
    SeoMetadata seo{};
    std::function<std::unique_ptr<SemanticWidget>()> widget_builder;
    std::string lastmod;
    bool is_canonical{true};
    bool is_indexable{true};
};

} // namespace pingdart::web
