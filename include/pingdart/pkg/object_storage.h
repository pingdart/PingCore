// include/pingdart/pkg/object_storage.h
// PKG-111D: Persistent Object Storage for Immutable .pdar Package Archives
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <pingdart/core/result.h>

namespace pingdart::pkg {

class IObjectStorage {
public:
    virtual ~IObjectStorage() = default;

    virtual core::Status put_archive(
        std::string_view storage_key,
        const std::vector<uint8_t>& archive_payload
    ) = 0;

    [[nodiscard]] virtual core::Result<std::vector<uint8_t>> get_archive(
        std::string_view storage_key
    ) = 0;

    [[nodiscard]] virtual bool has_archive(
        std::string_view storage_key
    ) = 0;

    [[nodiscard]] virtual std::string get_download_url(
        std::string_view storage_key
    ) const = 0;
};

// ============================================================================
// Content-Addressable Persistent Storage with File Immutability Locks
// ============================================================================

class ContentAddressableStorage final : public IObjectStorage {
public:
    explicit ContentAddressableStorage(std::filesystem::path storage_root = "build/storage/cas");

    core::Status put_archive(
        std::string_view storage_key,
        const std::vector<uint8_t>& archive_payload
    ) override;

    [[nodiscard]] core::Result<std::vector<uint8_t>> get_archive(
        std::string_view storage_key
    ) override;

    [[nodiscard]] bool has_archive(
        std::string_view storage_key
    ) override;

    [[nodiscard]] std::string get_download_url(
        std::string_view storage_key
    ) const override;

    [[nodiscard]] const std::filesystem::path& storage_root() const noexcept { return storage_root_; }

private:
    std::filesystem::path storage_root_;
    [[nodiscard]] std::filesystem::path resolve_key_path(std::string_view storage_key) const;
};

} // namespace pingdart::pkg
