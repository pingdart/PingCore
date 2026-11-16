#pragma once

#include "static_route.h"
#include "build_manifest.h"
#include <pingdart/core/result.h>
#include <string>
#include <vector>

namespace pingdart::web {

struct SsgOptions {
    std::string output_directory{"dist/web"};
    bool pretty_html{true};
    bool allow_indexing{true};
    bool write_to_disk{false};
    std::string sitemap_base_url{"https://example.com"};
};

struct SsgResult {
    size_t pages_generated{0};
    size_t assets_fingerprinted{0};
    std::string output_directory;
    BuildManifest manifest;
};

class IStaticGenerator {
public:
    virtual ~IStaticGenerator() = default;

    virtual void add_route(StaticRoute route) = 0;
    virtual void add_asset(std::string filename, std::string content) = 0;
    [[nodiscard]] virtual core::Result<SsgResult> build() = 0;
};

} // namespace pingdart::web
