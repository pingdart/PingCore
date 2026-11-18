#include <pingdart/cli/font_bundler.h>
#include <filesystem>
#include <algorithm>

namespace pingdart::cli {

bool FontBundler::is_system_proprietary(const std::string& path) {
    std::string lower_path = path;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    // macOS System Font directories
    if (lower_path.rfind("/system/library/fonts", 0) == 0 ||
        lower_path.rfind("/library/fonts", 0) == 0) {
        return true;
    }

    // Windows System Fonts
    if (lower_path.rfind("c:\\windows\\fonts", 0) == 0 ||
        lower_path.rfind("c:/windows/fonts", 0) == 0) {
        return true;
    }

    // Specific Apple proprietary font names
    if (lower_path.find("kohinoor") != std::string::npos ||
        lower_path.find("helvetica") != std::string::npos ||
        lower_path.find("san francisco") != std::string::npos ||
        lower_path.find("arial.ttf") != std::string::npos ||
        lower_path.find("times new roman") != std::string::npos) {
        // If loaded directly from system directory, prohibit
        if (lower_path.find("/system/") != std::string::npos || lower_path.find("c:") != std::string::npos) {
            return true;
        }
    }

    return false;
}

core::Result<BundledFontInfo> FontBundler::evaluate_font_for_bundle(
    const std::string& font_path,
    const std::string& explicit_license)
{
    if (font_path.empty()) {
        return core::Status(core::StatusCode::InvalidArgument, "Font path cannot be empty");
    }

    if (is_system_proprietary(font_path)) {
        std::string filename = std::filesystem::path(font_path).filename().string();
        return core::Status(
            core::StatusCode::PermissionDenied,
            "Refusing to bundle proprietary system font '" + filename +
            "' into distributable package. System fonts are protected by OS copyright and licensing terms. "
            "Please use a legally redistributable open font (e.g. Google Noto, SIL Open Font License, or Apache 2.0) "
            "bundled in your project's assets/fonts directory."
        );
    }

    std::error_code ec;
    if (!std::filesystem::exists(font_path, ec)) {
        return core::Status(core::StatusCode::NotFound, "Font file does not exist: " + font_path);
    }

    std::string ext = std::filesystem::path(font_path).extension().string();
    if (ext != ".ttf" && ext != ".otf" && ext != ".ttc" && ext != ".woff2") {
        return core::Status(core::StatusCode::InvalidArgument,
                            "Unsupported font format extension: " + ext);
    }

    BundledFontInfo info;
    info.source_path = font_path;
    info.font_name = std::filesystem::path(font_path).filename().string();
    info.relative_dest_path = "fonts/" + info.font_name;
    info.license_type = explicit_license.empty() ? "OFL / Apache-2.0 (Declared)" : explicit_license;
    info.is_redistributable = true;

    return info;
}

core::Status FontBundler::copy_fonts_to_bundle(
    const std::vector<BundledFontInfo>& fonts,
    const std::string& destination_directory)
{
    std::error_code ec;
    std::filesystem::path dest_dir(destination_directory);
    std::filesystem::path font_dir = dest_dir / "fonts";
    std::filesystem::create_directories(font_dir, ec);
    if (ec) {
        return core::Status(core::StatusCode::Internal, "Failed to create font target directory: " + ec.message());
    }

    for (const auto& font : fonts) {
        if (!font.is_redistributable) continue;
        std::filesystem::path target = dest_dir / font.relative_dest_path;
        std::filesystem::copy_file(font.source_path, target,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            return core::Status(core::StatusCode::Internal,
                                "Failed to copy font " + font.font_name + ": " + ec.message());
        }
    }

    return core::Status::Ok();
}

} // namespace pingdart::cli
