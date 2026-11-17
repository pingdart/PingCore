// include/pingdart/security/secure_storage.h
// PKG-115: Safe Key/Value Credential & Secret Storage
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <pingdart/core/result.h>
#include <pingdart/safety/secure_memory.h>

namespace pingdart::security {

class ISecureStorage {
public:
    virtual ~ISecureStorage() = default;

    [[nodiscard]] virtual core::Status set_secret(
        std::string_view key,
        const safety::SecureBuffer& secret
    ) = 0;

    [[nodiscard]] virtual core::Result<safety::SecureBuffer> get_secret(
        std::string_view key
    ) = 0;

    [[nodiscard]] virtual core::Status delete_secret(
        std::string_view key
    ) = 0;

    [[nodiscard]] virtual bool has_secret(
        std::string_view key
    ) const = 0;
};

/// Factory to create the platform-appropriate secure credential storage
[[nodiscard]] std::unique_ptr<ISecureStorage> create_secure_storage();

} // namespace pingdart::security
