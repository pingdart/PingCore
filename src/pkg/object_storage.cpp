// src/pkg/object_storage.cpp
// PKG-111D: Persistent Object Storage Implementation
#include <pingdart/pkg/object_storage.h>
#include <fstream>
#include <algorithm>

namespace pingdart::pkg {

ContentAddressableStorage::ContentAddressableStorage(std::filesystem::path storage_root)
    : storage_root_(std::move(storage_root)) {
    std::error_code ec;
    std::filesystem::create_directories(storage_root_, ec);
}

std::filesystem::path ContentAddressableStorage::resolve_key_path(std::string_view storage_key) const {
    std::string clean(storage_key);
    // Sanitize any URL prefixes if passed as s3://...
    if (clean.starts_with("s3://")) {
        size_t next_slash = clean.find('/', 5);
        if (next_slash != std::string::npos) {
            clean = clean.substr(next_slash + 1);
        }
    }
    return storage_root_ / clean;
}

core::Status ContentAddressableStorage::put_archive(
    std::string_view storage_key,
    const std::vector<uint8_t>& archive_payload)
{
    std::filesystem::path dest_path = resolve_key_path(storage_key);
    std::error_code ec;

    // Immutability Check: existing objects can NEVER be overwritten
    if (std::filesystem::exists(dest_path, ec)) {
        return core::Status(core::StatusCode::AlreadyExists,
                            "Immutable storage violation: archive already exists at key: " + std::string(storage_key));
    }

    std::filesystem::create_directories(dest_path.parent_path(), ec);

    // Atomic write via temporary file
    std::filesystem::path tmp_path = dest_path;
    tmp_path += ".tmp";

    {
        std::ofstream out(tmp_path, std::ios::binary);
        if (!out.is_open()) {
            return core::Status(core::StatusCode::Internal, "Failed to create temporary archive file: " + tmp_path.string());
        }
        out.write(reinterpret_cast<const char*>(archive_payload.data()), static_cast<std::streamsize>(archive_payload.size()));
    }

    // Atomic move to final key destination
    std::filesystem::rename(tmp_path, dest_path, ec);
    if (ec) {
        std::filesystem::remove(tmp_path, ec);
        return core::Status(core::StatusCode::Internal, "Atomic commit failed: " + ec.message());
    }

    // Lock file as read-only to enforce immutability at the OS filesystem level
    std::filesystem::permissions(dest_path,
        std::filesystem::perms::owner_read | std::filesystem::perms::group_read | std::filesystem::perms::others_read,
        std::filesystem::perm_options::replace, ec);

    return core::Status{};
}

core::Result<std::vector<uint8_t>> ContentAddressableStorage::get_archive(std::string_view storage_key) {
    std::filesystem::path p = resolve_key_path(storage_key);
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        return core::Status(core::StatusCode::NotFound, "Archive not found in object storage: " + std::string(storage_key));
    }

    std::ifstream in(p, std::ios::binary);
    if (!in.is_open()) {
        return core::Status(core::StatusCode::Internal, "Failed to open archive file: " + p.string());
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return data;
}

bool ContentAddressableStorage::has_archive(std::string_view storage_key) {
    std::filesystem::path p = resolve_key_path(storage_key);
    std::error_code ec;
    return std::filesystem::exists(p, ec);
}

std::string ContentAddressableStorage::get_download_url(std::string_view storage_key) const {
    return "https://cdn.registry.pingdart.com/archives/" + std::string(storage_key);
}

} // namespace pingdart::pkg
