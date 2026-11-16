// include/pingdart/pkg/archive.h
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <pingdart/core/result.h>

namespace pingdart::pkg {

class ArchiveUtil {
public:
    [[nodiscard]] static std::string compute_sha256(const uint8_t* data, size_t length);
    [[nodiscard]] static std::string compute_sha256(const std::vector<uint8_t>& data);
    [[nodiscard]] static std::string compute_string_sha256(std::string_view text);
    [[nodiscard]] static std::string compute_file_sha256(const std::filesystem::path& file_path);

    // Creates an archive package payload from a directory
    [[nodiscard]] static core::Result<std::vector<uint8_t>> pack_package_directory(
        const std::filesystem::path& package_dir
    );

    // Unpacks an archive package payload into a destination directory
    [[nodiscard]] static core::Status unpack_package_archive(
        const std::vector<uint8_t>& archive_data,
        const std::filesystem::path& target_dest
    );
};

} // namespace pingdart::pkg
