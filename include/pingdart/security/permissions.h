// include/pingdart/security/permissions.h
// PKG-115: Granular Capability-Based Permissions Management
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <pingdart/core/result.h>

namespace pingdart::security {

enum class Permission : uint32_t {
    NetworkClient    = 1 << 0,
    NetworkServer    = 1 << 1,
    FilesystemRead   = 1 << 2,
    FilesystemWrite  = 1 << 3,
    Camera           = 1 << 4,
    Microphone       = 1 << 5,
    Geolocation      = 1 << 6,
    ProcessSpawn     = 1 << 7,
    SecureStorage    = 1 << 8,
};

enum class PermissionState {
    NotDetermined,
    Granted,
    Denied
};

class PermissionManager {
public:
    static PermissionManager& instance() {
        static PermissionManager s_instance;
        return s_instance;
    }

    void grant(Permission perm) {
        std::lock_guard<std::mutex> lock(mutex_);
        permissions_[perm] = PermissionState::Granted;
    }

    void deny(Permission perm) {
        std::lock_guard<std::mutex> lock(mutex_);
        permissions_[perm] = PermissionState::Denied;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        permissions_.clear();
    }

    [[nodiscard]] PermissionState check(Permission perm) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = permissions_.find(perm);
        if (it != permissions_.end()) {
            return it->second;
        }
        return PermissionState::NotDetermined;
    }

    [[nodiscard]] bool is_granted(Permission perm) const {
        return check(perm) == PermissionState::Granted;
    }

    [[nodiscard]] core::Status enforce(Permission perm) const {
        if (!is_granted(perm)) {
            return core::Status(core::StatusCode::PermissionDenied, "Permission denied for requested capability");
        }
        return core::Status::Ok();
    }

private:
    PermissionManager() = default;
    mutable std::mutex mutex_;
    std::unordered_map<Permission, PermissionState> permissions_;
};

} // namespace pingdart::security
