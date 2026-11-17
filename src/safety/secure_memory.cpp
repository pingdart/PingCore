// src/safety/secure_memory.cpp
// PKG-115: Secure Memory Scrubbing and Page Locking Implementation
#define __STDC_WANT_LIB_EXT1__ 1
#include <pingdart/safety/secure_memory.h>
#include <cstdlib>
#include <cstring>
#if !defined(_WIN32)
#include <strings.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#include <memoryapi.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace pingdart::safety {

void secure_zero_memory(void* ptr, std::size_t len) noexcept {
    if (!ptr || len == 0) return;
#if defined(_WIN32)
    SecureZeroMemory(ptr, len);
#elif defined(__APPLE__)
    memset_s(ptr, len, 0, len);
#else
    // Volatile pointer memory scrubber prevents compiler dead-store elimination
    volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
    while (len--) {
        *p++ = 0;
    }
#endif
}

SecureBuffer::SecureBuffer() noexcept : data_(nullptr), size_(0), is_locked_(false) {}

SecureBuffer::SecureBuffer(std::size_t size) : size_(size), is_locked_(false) {
    if (size_ > 0) {
        data_ = static_cast<uint8_t*>(std::malloc(size_));
        if (data_) {
            secure_zero_memory(data_, size_);
        }
    }
}

SecureBuffer::SecureBuffer(const void* data, std::size_t size) : size_(size), is_locked_(false) {
    if (size_ > 0 && data) {
        data_ = static_cast<uint8_t*>(std::malloc(size_));
        if (data_) {
            std::memcpy(data_, data, size_);
        }
    }
}

SecureBuffer::SecureBuffer(std::string_view str) : size_(str.size()), is_locked_(false) {
    if (size_ > 0) {
        data_ = static_cast<uint8_t*>(std::malloc(size_));
        if (data_) {
            std::memcpy(data_, str.data(), size_);
        }
    }
}

SecureBuffer::~SecureBuffer() {
    wipe();
}

SecureBuffer::SecureBuffer(SecureBuffer&& other) noexcept
    : data_(other.data_), size_(other.size_), is_locked_(other.is_locked_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.is_locked_ = false;
}

SecureBuffer& SecureBuffer::operator=(SecureBuffer&& other) noexcept {
    if (this != &other) {
        wipe();
        data_ = other.data_;
        size_ = other.size_;
        is_locked_ = other.is_locked_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.is_locked_ = false;
    }
    return *this;
}

core::Status SecureBuffer::lock_memory() noexcept {
    if (!data_ || size_ == 0) return core::Status::Ok();
#if defined(_WIN32)
    if (!VirtualLock(data_, size_)) {
        return core::Status(core::StatusCode::Internal, "VirtualLock failed");
    }
#else
    if (mlock(data_, size_) != 0) {
        return core::Status(core::StatusCode::Internal, "mlock failed to lock memory pages into RAM");
    }
#endif
    is_locked_ = true;
    return core::Status::Ok();
}

void SecureBuffer::wipe() noexcept {
    if (data_) {
        if (is_locked_) {
#if defined(_WIN32)
            VirtualUnlock(data_, size_);
#else
            munlock(data_, size_);
#endif
            is_locked_ = false;
        }
        secure_zero_memory(data_, size_);
        std::free(data_);
        data_ = nullptr;
    }
    size_ = 0;
}

} // namespace pingdart::safety
