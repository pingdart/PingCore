// src/security/secure_storage.cpp
// PKG-115: Safe Key/Value Credential & Secret Storage Implementation
#include <pingdart/security/secure_storage.h>
#include <mutex>
#include <unordered_map>

namespace pingdart::security {

namespace {

class InMemorySecureStorage final : public ISecureStorage {
public:
    core::Status set_secret(std::string_view key, const safety::SecureBuffer& secret) override {
        std::lock_guard<std::mutex> lock(mutex_);
        secrets_.insert_or_assign(std::string(key), safety::SecureBuffer(secret.data(), secret.size()));
        return core::Status::Ok();
    }

    core::Result<safety::SecureBuffer> get_secret(std::string_view key) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = secrets_.find(std::string(key));
        if (it == secrets_.end()) {
            return core::Status(core::StatusCode::NotFound, "Secret key not found");
        }
        return safety::SecureBuffer(it->second.data(), it->second.size());
    }

    core::Status delete_secret(std::string_view key) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = secrets_.find(std::string(key));
        if (it != secrets_.end()) {
            it->second.wipe();
            secrets_.erase(it);
        }
        return core::Status::Ok();
    }

    bool has_secret(std::string_view key) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return secrets_.contains(std::string(key));
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, safety::SecureBuffer> secrets_;
};

} // namespace

std::unique_ptr<ISecureStorage> create_secure_storage() {
    return std::make_unique<InMemorySecureStorage>();
}

} // namespace pingdart::security
