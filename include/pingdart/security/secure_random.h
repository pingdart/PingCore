// include/pingdart/security/secure_random.h
// PKG-115: Cryptographically Secure Random Number Generation
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <pingdart/core/result.h>
#include <pingdart/safety/safe_buffer.h>

namespace pingdart::security {

class SecureRandom {
public:
    /// Fills destination buffer with cryptographically secure random bytes
    [[nodiscard]] static core::Status fill_bytes(void* dest, std::size_t len) noexcept;

    /// Fills SafeSpan with cryptographically secure random bytes
    [[nodiscard]] static core::Status fill_bytes(safety::SafeSpan<uint8_t> span) noexcept {
        return fill_bytes(span.data(), span.size());
    }

    /// Generates a vector of secure random bytes
    [[nodiscard]] static core::Result<std::vector<uint8_t>> generate_bytes(std::size_t len);

    /// Generates a random 32-bit unsigned integer
    [[nodiscard]] static uint32_t next_uint32();

    /// Generates a random 64-bit unsigned integer
    [[nodiscard]] static uint64_t next_uint64();

    /// Generates a secure random hex string of given byte length (e.g., 16 bytes = 32 hex chars)
    [[nodiscard]] static std::string generate_hex_token(std::size_t byte_count = 32);
};

} // namespace pingdart::security
