// src/pkg/archive.cpp
#include <pingdart/pkg/archive.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

#if defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#endif

namespace pingdart::pkg {

namespace {

constexpr uint32_t PDAR_MAGIC = 0x52414450; // "PDAR" in little endian

std::string bytes_to_hex(const uint8_t* digest, size_t length) {
    std::ostringstream ss;
    for (size_t i = 0; i < length; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}

} // namespace

std::string ArchiveUtil::compute_sha256(const uint8_t* data, size_t length) {
#if defined(__APPLE__)
    unsigned char md[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(data, static_cast<CC_LONG>(length), md);
    return "sha256:" + bytes_to_hex(md, CC_SHA256_DIGEST_LENGTH);
#else
    // Portable fallback hash
    uint64_t h1 = 0xcbf29ce484222325ULL;
    uint64_t h2 = 0x100000001b3ULL;
    for (size_t i = 0; i < length; ++i) {
        h1 = (h1 ^ data[i]) * 1099511628211ULL;
        h2 = (h2 ^ (data[i] + 0x5a)) * 14695981039346656037ULL;
    }
    std::ostringstream ss;
    ss << "sha256:" << std::hex << std::setw(16) << std::setfill('0') << h1
       << std::setw(16) << std::setfill('0') << h2;
    return ss.str();
#endif
}

std::string ArchiveUtil::compute_sha256(const std::vector<uint8_t>& data) {
    return compute_sha256(data.data(), data.size());
}

std::string ArchiveUtil::compute_string_sha256(std::string_view text) {
    return compute_sha256(reinterpret_cast<const uint8_t*>(text.data()), text.size());
}

std::string ArchiveUtil::compute_file_sha256(const std::filesystem::path& file_path) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        return "";
    }
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return compute_sha256(buffer);
}

core::Result<std::vector<uint8_t>> ArchiveUtil::pack_package_directory(
    const std::filesystem::path& package_dir)
{
    std::error_code ec;
    if (!std::filesystem::exists(package_dir, ec) || !std::filesystem::is_directory(package_dir, ec)) {
        return core::Status(core::StatusCode::NotFound, "Source package directory does not exist: " + package_dir.string());
    }

    std::vector<uint8_t> archive;

    // 1. Magic Header: "PDAR" (4 bytes)
    const uint32_t magic = PDAR_MAGIC;
    const uint8_t* magic_ptr = reinterpret_cast<const uint8_t*>(&magic);
    archive.insert(archive.end(), magic_ptr, magic_ptr + 4);

    // 2. Iterate and pack eligible files
    for (const auto& entry : std::filesystem::recursive_directory_iterator(package_dir, ec)) {
        // Skip symlinks, special files, and directories (only pack regular files)
        if (entry.is_symlink(ec) || !entry.is_regular_file(ec)) {
            continue;
        }

        std::filesystem::path rel_path = std::filesystem::relative(entry.path(), package_dir, ec);
        std::string rel_str = rel_path.generic_string();

        // Skip build and VCS directories
        if (rel_str.starts_with(".git") || rel_str.starts_with("build") || rel_str.starts_with(".pingdart")) {
            continue;
        }

        std::ifstream file_in(entry.path(), std::ios::binary);
        if (!file_in.is_open()) continue;
        std::vector<uint8_t> file_bytes((std::istreambuf_iterator<char>(file_in)), std::istreambuf_iterator<char>());

        // Format: [4 bytes path length] [path string] [8 bytes content length] [content bytes]
        uint32_t path_len = static_cast<uint32_t>(rel_str.size());
        const uint8_t* pl_ptr = reinterpret_cast<const uint8_t*>(&path_len);
        archive.insert(archive.end(), pl_ptr, pl_ptr + sizeof(uint32_t));
        archive.insert(archive.end(), rel_str.begin(), rel_str.end());

        uint64_t file_len = static_cast<uint64_t>(file_bytes.size());
        const uint8_t* fl_ptr = reinterpret_cast<const uint8_t*>(&file_len);
        archive.insert(archive.end(), fl_ptr, fl_ptr + sizeof(uint64_t));
        archive.insert(archive.end(), file_bytes.begin(), file_bytes.end());
    }

    return archive;
}

core::Status ArchiveUtil::unpack_package_archive(
    const std::vector<uint8_t>& archive_data,
    const std::filesystem::path& target_dest)
{
    if (archive_data.size() < 4) {
        return core::Status(core::StatusCode::InvalidArgument, "Archive payload too small");
    }

    uint32_t magic = 0;
    std::memcpy(&magic, archive_data.data(), sizeof(uint32_t));
    if (magic != PDAR_MAGIC) {
        return core::Status(core::StatusCode::InvalidArgument, "Invalid PingDart archive format (magic mismatch)");
    }

    std::error_code ec;
    std::filesystem::create_directories(target_dest, ec);

    size_t offset = 4;
    while (offset < archive_data.size()) {
        if (offset + sizeof(uint32_t) > archive_data.size()) break;
        uint32_t path_len = 0;
        std::memcpy(&path_len, archive_data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        if (offset + path_len > archive_data.size()) break;
        std::string rel_path(reinterpret_cast<const char*>(archive_data.data() + offset), path_len);
        offset += path_len;

        if (offset + sizeof(uint64_t) > archive_data.size()) break;
        uint64_t file_len = 0;
        std::memcpy(&file_len, archive_data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        if (offset + file_len > archive_data.size()) break;

        // Path Traversal / Zip Slip Protection (Component-aware containment & symlink hardening)
        if (rel_path.empty() ||
            rel_path.starts_with("/") || rel_path.starts_with("\\") ||
            rel_path.find("..") != std::string::npos ||
            rel_path.find(':') != std::string::npos ||
            rel_path.find('\0') != std::string::npos) {
            return core::Status(core::StatusCode::PermissionDenied,
                                "Path traversal attempt detected in archive payload: " + rel_path);
        }

        std::filesystem::path normal_target = target_dest.lexically_normal();
        std::filesystem::path dest_file = (normal_target / rel_path).lexically_normal();

        // 1. Path-component-aware containment check (prevents /app matching /app-evil prefix exploit)
        auto t_it = normal_target.begin();
        auto d_it = dest_file.begin();
        for (; t_it != normal_target.end(); ++t_it, ++d_it) {
            if (d_it == dest_file.end() || *t_it != *d_it) {
                return core::Status(core::StatusCode::PermissionDenied,
                                    "Archive path traversal violation (component mismatch): " + rel_path);
            }
        }
        if (d_it == dest_file.end()) {
            return core::Status(core::StatusCode::PermissionDenied,
                                "Archive path cannot overwrite target destination root: " + rel_path);
        }

        // 2. Symlink rejection: ensure no component in path is a symlink
        std::filesystem::path check_path = normal_target;
        for (auto it = d_it; it != dest_file.end(); ++it) {
            check_path /= *it;
            if (std::filesystem::exists(check_path, ec) && std::filesystem::is_symlink(check_path, ec)) {
                return core::Status(core::StatusCode::PermissionDenied,
                                    "Archive extraction through filesystem link rejected: " + check_path.string());
            }
        }

        std::filesystem::create_directories(dest_file.parent_path(), ec);

        std::ofstream out(dest_file, std::ios::binary);
        if (out.is_open()) {
            out.write(reinterpret_cast<const char*>(archive_data.data() + offset), static_cast<std::streamsize>(file_len));
        }
        offset += static_cast<size_t>(file_len);
    }

    return core::Status{};
}

} // namespace pingdart::pkg
