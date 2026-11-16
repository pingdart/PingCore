#pragma once

#include <string>
#include <vector>
#include <pingdart/core/result.h>
#include <pingdart/core/status.h>

namespace pingdart::cli {

struct BundledFontInfo {
    std::string font_name;
    std::string source_path;
    std::string relative_dest_path;
    std::string license_type; // e.g. "OFL", "Apache-2.0", "Public Domain"
    bool is_redistributable{false};
};

class FontBundler {
public:
    FontBundler() = default;

    // Checks if a font is a system-provided proprietary font that cannot be legally bundled
    [[nodiscard]] static bool is_system_proprietary(const std::string& path);

    // Validates a font candidate for inclusion in application bundle
    [[nodiscard]] core::Result<BundledFontInfo> evaluate_font_for_bundle(
        const std::string& font_path,
        const std::string& explicit_license = ""
    );

    // Bundles approved fonts into target resource directory
    [[nodiscard]] core::Status copy_fonts_to_bundle(
        const std::vector<BundledFontInfo>& fonts,
        const std::string& destination_directory
    );
};

} // namespace pingdart::cli
