// src/security/secure_random.cpp
// PKG-115: Cryptographically Secure Random Number Generation Implementation
#include <pingdart/security/secure_random.h>
#include <iomanip>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <stdlib.h>
#else
#include <sys/random.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace pingdart::security {

core::Status SecureRandom::fill_bytes(void* dest, std::size_t len) noexcept {
    if (!dest || len == 0) return core::Status::Ok();

#if defined(_WIN32)
    NTSTATUS status = BCryptGenRandom(
        NULL,
        static_cast<PUCHAR>(dest),
        static_cast<ULONG>(len),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
    if (!BCRYPT_SUCCESS(status)) {
        return core::Status(core::StatusCode::Internal, "BCryptGenRandom failed with status: " + std::to_string(status));
    }
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    arc4random_buf(dest, len);
#else
    ssize_t ret = getrandom(dest, len, 0);
    if (ret != static_cast<ssize_t>(len)) {
        int fd = open("/dev/urandom", O_RDONLY);
        if (fd < 0) {
            return core::Status(core::StatusCode::Internal, "Failed to access /dev/urandom");
        }
        size_t total_read = 0;
        while (total_read < len) {
            ssize_t r = read(fd, static_cast<char*>(dest) + total_read, len - total_read);
            if (r <= 0) {
                close(fd);
                return core::Status(core::StatusCode::Internal, "Failed to read from /dev/urandom");
            }
            total_read += static_cast<size_t>(r);
        }
        close(fd);
    }
#endif
    return core::Status::Ok();
}

core::Result<std::vector<uint8_t>> SecureRandom::generate_bytes(std::size_t len) {
    std::vector<uint8_t> buffer(len);
    auto res = fill_bytes(buffer.data(), len);
    if (!res.ok()) {
        return res;
    }
    return buffer;
}

uint32_t SecureRandom::next_uint32() {
    uint32_t val = 0;
    auto res = fill_bytes(&val, sizeof(val));
    if (!res.ok()) {
        throw std::runtime_error("SecureRandom::next_uint32 failed: " + std::string(res.message()));
    }
    return val;
}

uint64_t SecureRandom::next_uint64() {
    uint64_t val = 0;
    auto res = fill_bytes(&val, sizeof(val));
    if (!res.ok()) {
        throw std::runtime_error("SecureRandom::next_uint64 failed: " + std::string(res.message()));
    }
    return val;
}

std::string SecureRandom::generate_hex_token(std::size_t byte_count) {
    auto bytes_res = generate_bytes(byte_count);
    if (!bytes_res.has_value()) {
        throw std::runtime_error("generate_hex_token failed: " + std::string(bytes_res.status().message()));
    }
    const auto& bytes = bytes_res.value();
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t b : bytes) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

} // namespace pingdart::security
