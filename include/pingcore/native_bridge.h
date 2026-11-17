// include/pingcore/native_bridge.h
// PingCore Stable C ABI Native Bridge & Platform Extension SDK (PKG-121 / PKG-122 / PKG-123)
#pragma once

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32)
  #define PINGCORE_API __declspec(dllexport)
#else
  #define PINGCORE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PINGCORE_PERM_CAMERA = 0,
    PINGCORE_PERM_MICROPHONE = 1,
    PINGCORE_PERM_LOCATION = 2,
    PINGCORE_PERM_NOTIFICATIONS = 3,
    PINGCORE_PERM_STORAGE = 4,
    PINGCORE_PERM_BLUETOOTH = 5
} pingcore_native_perm_t;

typedef enum {
    PINGCORE_PERM_STATUS_GRANTED = 0,
    PINGCORE_PERM_STATUS_DENIED = 1,
    PINGCORE_PERM_STATUS_RESTRICTED = 2,
    PINGCORE_PERM_STATUS_NOT_DETERMINED = 3
} pingcore_native_perm_status_t;

typedef enum {
    PINGCORE_INVOKE_SUCCESS = 0,
    PINGCORE_INVOKE_BUFFER_TOO_SMALL = -1,
    PINGCORE_INVOKE_METHOD_NOT_FOUND = -2,
    PINGCORE_INVOKE_INVALID_ARGUMENTS = -3,
    PINGCORE_INVOKE_PLATFORM_ERROR = -4,
    PINGCORE_INVOKE_CANCELLED = -5
} pingcore_invoke_status_t;

// Asynchronous callback function pointer types
typedef void (*pingcore_perm_cb_t)(
    uint64_t request_id,
    pingcore_native_perm_t perm,
    pingcore_native_perm_status_t status,
    void* user_data
);

typedef void (*pingcore_async_invoke_cb_t)(
    uint64_t request_id,
    int status,
    const char* json_result,
    void* user_data
);

// Function pointer types implemented by platform layer (JNI / Swift / AppKit / Win32)
typedef void (*pingcore_platform_request_perm_fn)(
    uint64_t request_id,
    pingcore_native_perm_t perm,
    pingcore_perm_cb_t cb,
    void* user_data
);

typedef int (*pingcore_platform_invoke_method_fn)(
    const char* method,
    const char* json_args,
    char* out_buf,
    size_t out_len
);

typedef void (*pingcore_platform_invoke_async_fn)(
    uint64_t request_id,
    const char* method,
    const char* json_args,
    pingcore_async_invoke_cb_t cb,
    void* user_data
);

typedef void (*pingcore_platform_cancel_fn)(uint64_t request_id);

// C ABI Platform Bridge Registration & Invocation
PINGCORE_API void pingcore_native_register_platform_handlers(
    pingcore_platform_request_perm_fn request_perm_fn,
    pingcore_platform_invoke_method_fn invoke_method_fn
);

PINGCORE_API void pingcore_native_register_async_handlers(
    pingcore_platform_invoke_async_fn invoke_async_fn,
    pingcore_platform_cancel_fn cancel_fn
);

PINGCORE_API uint64_t pingcore_native_request_permission(
    pingcore_native_perm_t perm,
    pingcore_perm_cb_t cb,
    void* user_data
);

PINGCORE_API void pingcore_native_notify_permission_result(
    uint64_t request_id,
    pingcore_native_perm_t perm,
    pingcore_native_perm_status_t status
);

PINGCORE_API int pingcore_native_invoke_platform(
    const char* method,
    const char* json_args,
    char* out_buf,
    size_t out_len
);

PINGCORE_API uint64_t pingcore_native_invoke_async(
    const char* method,
    const char* json_args,
    pingcore_async_invoke_cb_t cb,
    void* user_data
);

PINGCORE_API void pingcore_native_notify_async_result(
    uint64_t request_id,
    int status,
    const char* json_result
);

PINGCORE_API void pingcore_native_cancel_request(uint64_t request_id);

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <cstring>
#include <atomic>
#include <memory>

namespace pingcore {

enum class PermissionType {
    Camera = PINGCORE_PERM_CAMERA,
    Microphone = PINGCORE_PERM_MICROPHONE,
    Location = PINGCORE_PERM_LOCATION,
    Notifications = PINGCORE_PERM_NOTIFICATIONS,
    Storage = PINGCORE_PERM_STORAGE,
    Bluetooth = PINGCORE_PERM_BLUETOOTH
};

enum class PermissionStatus {
    Granted = PINGCORE_PERM_STATUS_GRANTED,
    Denied = PINGCORE_PERM_STATUS_DENIED,
    Restricted = PINGCORE_PERM_STATUS_RESTRICTED,
    NotDetermined = PINGCORE_PERM_STATUS_NOT_DETERMINED
};

enum class RequestState {
    Pending,
    Completed,
    Cancelled
};

using PermissionCallback = std::function<void(PermissionStatus)>;
using AsyncInvokeCallback = std::function<void(int status, std::string result)>;

struct AsyncRequestRecord {
    RequestState state{RequestState::Pending};
    AsyncInvokeCallback callback;
};

struct PermRequestRecord {
    RequestState state{RequestState::Pending};
    PermissionCallback callback;
};

class NativeBridge {
public:
    static NativeBridge& instance() {
        static NativeBridge s_bridge;
        return s_bridge;
    }

    void register_handlers(pingcore_platform_request_perm_fn perm_fn, pingcore_platform_invoke_method_fn invoke_fn) {
        std::lock_guard<std::mutex> lock(mutex_);
        perm_handler_ = perm_fn;
        invoke_handler_ = invoke_fn;
    }

    void register_async_handlers(pingcore_platform_invoke_async_fn async_fn, pingcore_platform_cancel_fn cancel_fn) {
        std::lock_guard<std::mutex> lock(mutex_);
        async_handler_ = async_fn;
        cancel_handler_ = cancel_fn;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        perm_handler_ = nullptr;
        invoke_handler_ = nullptr;
        async_handler_ = nullptr;
        cancel_handler_ = nullptr;
        next_req_id_ = 0;
        pending_perm_requests_.clear();
        pending_raw_perm_callbacks_.clear();
        pending_async_requests_.clear();
        recorded_permission_results_.clear();
    }

    uint64_t request_permission(PermissionType type, PermissionCallback callback) {
        uint64_t req_id = 0;
        pingcore_platform_request_perm_fn handler = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            req_id = ++next_req_id_;
            pending_perm_requests_[req_id] = {RequestState::Pending, std::move(callback)};
            handler = perm_handler_;
        } // Mutex released!

        if (handler) {
            handler(
                req_id,
                static_cast<pingcore_native_perm_t>(type),
                [](uint64_t id, pingcore_native_perm_t, pingcore_native_perm_status_t status, void*) {
                    NativeBridge::instance().notify_permission_result(id, PINGCORE_PERM_CAMERA, status);
                },
                nullptr
            );
        } else {
            notify_permission_result(req_id, static_cast<pingcore_native_perm_t>(type), PINGCORE_PERM_STATUS_GRANTED);
        }
        return req_id;
    }

    uint64_t request_permission_raw(pingcore_native_perm_t perm, pingcore_perm_cb_t cb, void* user_data) {
        uint64_t req_id = 0;
        pingcore_platform_request_perm_fn handler = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            req_id = ++next_req_id_;
            if (cb) {
                pending_raw_perm_callbacks_[req_id] = {cb, user_data};
            }
            handler = perm_handler_;
        } // Mutex released!

        if (handler) {
            handler(req_id, perm, cb, user_data);
        } else {
            if (cb) {
                cb(req_id, perm, PINGCORE_PERM_STATUS_GRANTED, user_data);
                std::lock_guard<std::mutex> lock(mutex_);
                pending_raw_perm_callbacks_.erase(req_id);
            }
        }
        return req_id;
    }

    bool notify_permission_result(uint64_t req_id, pingcore_native_perm_t perm, pingcore_native_perm_status_t status) {
        PermissionCallback cb;
        std::pair<pingcore_perm_cb_t, void*> raw_cb{nullptr, nullptr};

        {
            std::lock_guard<std::mutex> lock(mutex_);
            recorded_permission_results_[perm] = status;

            if (req_id > 0) {
                auto it = pending_perm_requests_.find(req_id);
                if (it != pending_perm_requests_.end()) {
                    if (it->second.state == RequestState::Pending) {
                        it->second.state = RequestState::Completed;
                        cb = std::move(it->second.callback);
                        pending_perm_requests_.erase(it);
                    }
                }
                auto it_raw = pending_raw_perm_callbacks_.find(req_id);
                if (it_raw != pending_raw_perm_callbacks_.end()) {
                    raw_cb = it_raw->second;
                    pending_raw_perm_callbacks_.erase(it_raw);
                }
            } else {
                for (auto it = pending_perm_requests_.begin(); it != pending_perm_requests_.end();) {
                    if (it->second.state == RequestState::Pending) {
                        it->second.state = RequestState::Completed;
                        cb = std::move(it->second.callback);
                        it = pending_perm_requests_.erase(it);
                        break;
                    } else {
                        ++it;
                    }
                }
            }
        } // Mutex released BEFORE calling developer callbacks!

        if (cb) {
            cb(static_cast<PermissionStatus>(status));
            return true;
        }
        if (raw_cb.first) {
            raw_cb.first(req_id, perm, status, raw_cb.second);
            return true;
        }
        return false;
    }

    bool cancel_request(uint64_t req_id) {
        pingcore_platform_cancel_fn c_handler = nullptr;
        PermissionCallback p_cb;
        AsyncInvokeCallback a_cb;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            c_handler = cancel_handler_;

            // Strict atomic state transition: Pending -> Cancelled
            auto it_perm = pending_perm_requests_.find(req_id);
            if (it_perm != pending_perm_requests_.end()) {
                if (it_perm->second.state == RequestState::Pending) {
                    it_perm->second.state = RequestState::Cancelled;
                    p_cb = std::move(it_perm->second.callback);
                    pending_perm_requests_.erase(it_perm);
                }
            }

            auto it_async = pending_async_requests_.find(req_id);
            if (it_async != pending_async_requests_.end()) {
                if (it_async->second.state == RequestState::Pending) {
                    it_async->second.state = RequestState::Cancelled;
                    a_cb = std::move(it_async->second.callback);
                    pending_async_requests_.erase(it_async);
                }
            }
            pending_raw_perm_callbacks_.erase(req_id);
        } // Mutex released BEFORE invoking callbacks!

        if (c_handler) {
            c_handler(req_id);
        }
        if (p_cb) {
            p_cb(PermissionStatus::Denied);
            return true;
        }
        if (a_cb) {
            a_cb(PINGCORE_INVOKE_CANCELLED, "{\"error\":\"Cancelled by caller\"}");
            return true;
        }
        return false; // Already completed or already cancelled!
    }

    using SyncDispatcherFn = std::function<bool(std::string_view, std::string_view, std::string&)>;
    using AsyncDispatcherFn = std::function<bool(std::string_view, std::string_view, std::function<void(int, std::string)>)>;

    void set_module_dispatchers(SyncDispatcherFn sync_disp, AsyncDispatcherFn async_disp) {
        std::lock_guard<std::mutex> lock(mutex_);
        sync_dispatcher_ = std::move(sync_disp);
        async_dispatcher_ = std::move(async_disp);
    }

    std::string invoke_platform_method(std::string_view method, std::string_view json_args) {
        pingcore_platform_invoke_method_fn handler = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            handler = invoke_handler_;
        }

        if (!handler) {
            SyncDispatcherFn disp;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                disp = sync_dispatcher_;
            }
            if (disp) {
                std::string result;
                if (disp(method, json_args, result)) {
                    return result;
                }
            }
            return "{\"success\":true,\"mock\":true}";
        }

        char buffer[4096]{};
        int rc = handler(std::string(method).c_str(), std::string(json_args).c_str(), buffer, sizeof(buffer));
        if (rc == PINGCORE_INVOKE_SUCCESS) {
            return std::string(buffer);
        } else if (rc == PINGCORE_INVOKE_BUFFER_TOO_SMALL) {
            return "{\"success\":false,\"error\":\"Buffer too small\"}";
        } else if (rc == PINGCORE_INVOKE_METHOD_NOT_FOUND) {
            return "{\"success\":false,\"error\":\"Method not found\"}";
        }
        return "{\"success\":false,\"error\":\"Native bridge invocation failed\",\"status\":" + std::to_string(rc) + "}";
    }

    int invoke_platform_raw(const char* method, const char* json_args, char* out_buf, size_t out_len) {
        if (!out_buf || out_len == 0) {
            return PINGCORE_INVOKE_INVALID_ARGUMENTS;
        }

        pingcore_platform_invoke_method_fn handler = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            handler = invoke_handler_;
        }

        if (!handler) {
            const char* default_resp = "{\"success\":true,\"mock\":true}";
            size_t len = std::strlen(default_resp);
            if (out_len <= len) {
                return PINGCORE_INVOKE_BUFFER_TOO_SMALL;
            }
            std::memcpy(out_buf, default_resp, len + 1);
            return PINGCORE_INVOKE_SUCCESS;
        }
        return handler(method, json_args, out_buf, out_len);
    }

    uint64_t invoke_async(std::string_view method, std::string_view json_args, AsyncInvokeCallback cb) {
        uint64_t req_id = 0;
        pingcore_platform_invoke_async_fn handler = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            req_id = ++next_req_id_;
            pending_async_requests_[req_id] = {RequestState::Pending, std::move(cb)};
            handler = async_handler_;
        } // Mutex released!

        if (handler) {
            handler(
                req_id,
                std::string(method).c_str(),
                std::string(json_args).c_str(),
                [](uint64_t id, int status, const char* json_result, void*) {
                    NativeBridge::instance().notify_async_result(id, status, json_result ? json_result : "{}");
                },
                nullptr
            );
        } else {
            AsyncDispatcherFn disp;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                disp = async_dispatcher_;
            }
            if (disp) {
                bool handled = disp(method, json_args, [this, req_id](int status, std::string result) {
                    this->notify_async_result(req_id, status, result.c_str());
                });
                if (handled) {
                    return req_id;
                }
            }
            notify_async_result(req_id, PINGCORE_INVOKE_SUCCESS, "{\"success\":true,\"mock\":true}");
        }
        return req_id;
    }

    bool notify_async_result(uint64_t req_id, int status, const char* json_result) {
        AsyncInvokeCallback cb;
        std::string payload = json_result ? json_result : "{}";

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = pending_async_requests_.find(req_id);
            if (it != pending_async_requests_.end()) {
                // Strict atomic state transition: Pending -> Completed
                if (it->second.state == RequestState::Pending) {
                    it->second.state = RequestState::Completed;
                    cb = std::move(it->second.callback);
                    pending_async_requests_.erase(it);
                }
            }
        } // Mutex released BEFORE invoking callback!

        if (cb) {
            cb(status, std::move(payload));
            return true;
        }
        return false; // Request was already completed or cancelled!
    }

    bool is_handler_registered() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return perm_handler_ != nullptr || invoke_handler_ != nullptr || async_handler_ != nullptr;
    }

private:
    mutable std::mutex mutex_;
    pingcore_platform_request_perm_fn perm_handler_{nullptr};
    pingcore_platform_invoke_method_fn invoke_handler_{nullptr};
    pingcore_platform_invoke_async_fn async_handler_{nullptr};
    pingcore_platform_cancel_fn cancel_handler_{nullptr};

    uint64_t next_req_id_{0};
    std::unordered_map<uint64_t, PermRequestRecord> pending_perm_requests_;
    std::unordered_map<uint64_t, std::pair<pingcore_perm_cb_t, void*>> pending_raw_perm_callbacks_;
    std::unordered_map<uint64_t, AsyncRequestRecord> pending_async_requests_;
    std::unordered_map<int, pingcore_native_perm_status_t> recorded_permission_results_;
    SyncDispatcherFn sync_dispatcher_{nullptr};
    AsyncDispatcherFn async_dispatcher_{nullptr};
};

inline uint64_t request_permission(PermissionType type, PermissionCallback callback) {
    return NativeBridge::instance().request_permission(type, std::move(callback));
}

inline void cancel_permission_request(uint64_t request_id) {
    NativeBridge::instance().cancel_request(request_id);
}

inline std::string invoke_native(std::string_view method, std::string_view json_args = "{}") {
    return NativeBridge::instance().invoke_platform_method(method, json_args);
}

inline uint64_t invoke_native_async(std::string_view method, std::string_view json_args, AsyncInvokeCallback callback) {
    return NativeBridge::instance().invoke_async(method, json_args, std::move(callback));
}

inline void cancel_native_operation(uint64_t request_id) {
    NativeBridge::instance().cancel_request(request_id);
}

inline void cancel_native_request(uint64_t request_id) {
    NativeBridge::instance().cancel_request(request_id);
}

} // namespace pingcore
#endif
