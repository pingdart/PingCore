// include/pingdart/pkg/sha256.h
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace pingdart::pkg {

class Sha256 {
public:
    Sha256();
    void update(const uint8_t* data, size_t length);
    void update(std::string_view text);
    std::string finalize();

    static std::string hash_string(std::string_view text);
    static std::string hash_bytes(const std::vector<uint8_t>& data);
    static std::string hash_file(const std::string& file_path);

private:
    uint32_t state_[8];
    uint64_t count_;
    uint8_t buffer_[64];

    void transform(const uint8_t block[64]);
};

} // namespace pingdart::pkg
