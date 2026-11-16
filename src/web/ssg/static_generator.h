#pragma once

#include <pingdart/web/i_static_generator.h>
#include <pingdart/web/i_web_renderer.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace pingdart::web {

class StaticGenerator final : public IStaticGenerator {
public:
    explicit StaticGenerator(SsgOptions options = {});
    ~StaticGenerator() override = default;

    void add_route(StaticRoute route) override;
    void add_asset(std::string filename, std::string content) override;
    [[nodiscard]] core::Result<SsgResult> build() override;

    // Helper: Maps a route path to its standard static output file path
    //   "/"                  -> "index.html"
    //   "/about"             -> "about/index.html"
    //   "/doctors/jane-smith"-> "doctors/jane-smith/index.html"
    [[nodiscard]] static std::string route_to_relative_file_path(std::string_view path);

    // Access in-memory generated files (relative path -> file content)
    [[nodiscard]] const std::unordered_map<std::string, std::string>& generated_files() const noexcept {
        return generated_files_;
    }

    [[nodiscard]] const SsgOptions& options() const noexcept { return options_; }

    // Computes a deterministic 16-character hex content hash for asset fingerprinting
    [[nodiscard]] static std::string compute_asset_hash(std::string_view content);

private:
    SsgOptions options_;
    std::vector<StaticRoute> routes_;
    std::unordered_map<std::string, std::string> raw_assets_;
    std::unordered_map<std::string, std::string> generated_files_;
    std::unique_ptr<IWebRenderer> renderer_;
};

} // namespace pingdart::web
