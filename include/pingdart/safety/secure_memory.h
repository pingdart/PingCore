// include/pingdart/safety/secure_memory.h
// PKG-115: Secure Memory Scrubbing and Page Locking
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <pingdart/core/result.h>
#include <pingdart/safety/safe_buffer.h>

namespace pingdart::safety {

/// Safely wipes memory with guaranteed compiler optimization suppression
void secure_zero_memory(void* ptr, std::size_t len) noexcept;

/// Owning secure memory buffer that locks pages and zeroes contents on destruction
class SecureBuffer {
public:
    SecureBuffer() noexcept;
    explicit SecureBuffer(std::size_t size);
    SecureBuffer(const void* data, std::size_t size);
    explicit SecureBuffer(std::string_view str);

    ~SecureBuffer();

    SecureBuffer(const SecureBuffer&) = delete;
    SecureBuffer& operator=(const SecureBuffer&) = delete;

    SecureBuffer(SecureBuffer&& other) noexcept;
    SecureBuffer& operator=(SecureBuffer&& other) noexcept;

    [[nodiscard]] uint8_t* data() noexcept { return data_; }
    [[nodiscard]] const uint8_t* data() const noexcept { return data_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    [[nodiscard]] SafeSpan<uint8_t> as_span() noexcept {
        return SafeSpan<uint8_t>(data_, size_);
    }

    [[nodiscard]] SafeSpan<const uint8_t> as_span() const noexcept {
        return SafeSpan<const uint8_t>(data_, size_);
    }

    /// Locks buffer memory into RAM to prevent swapping to disk
    [[nodiscard]] core::Status lock_memory() noexcept;

    /// Cleans and releases memory immediately
    void wipe() noexcept;

private:
    uint8_t* data_{nullptr};
    std::size_t size_{0};
    bool is_locked_{false};
};

} // namespace pingdart::safety
