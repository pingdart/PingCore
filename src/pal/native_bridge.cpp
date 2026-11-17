// src/pal/native_bridge.cpp
// PingCore Stable C ABI Implementation for Platform Bridge (PKG-121 / PKG-122)
#include <pingcore/native_bridge.h>

extern "C" {

void pingcore_native_register_platform_handlers(
    pingcore_platform_request_perm_fn request_perm_fn,
    pingcore_platform_invoke_method_fn invoke_method_fn
) {
    pingcore::NativeBridge::instance().register_handlers(request_perm_fn, invoke_method_fn);
}

void pingcore_native_register_async_handlers(
    pingcore_platform_invoke_async_fn invoke_async_fn,
    pingcore_platform_cancel_fn cancel_fn
) {
    pingcore::NativeBridge::instance().register_async_handlers(invoke_async_fn, cancel_fn);
}

uint64_t pingcore_native_request_permission(
    pingcore_native_perm_t perm,
    pingcore_perm_cb_t cb,
    void* user_data
) {
    return pingcore::NativeBridge::instance().request_permission_raw(perm, cb, user_data);
}

void pingcore_native_notify_permission_result(
    uint64_t request_id,
    pingcore_native_perm_t perm,
    pingcore_native_perm_status_t status
) {
    pingcore::NativeBridge::instance().notify_permission_result(request_id, perm, status);
}

int pingcore_native_invoke_platform(
    const char* method,
    const char* json_args,
    char* out_buf,
    size_t out_len
) {
    return pingcore::NativeBridge::instance().invoke_platform_raw(method, json_args, out_buf, out_len);
}

uint64_t pingcore_native_invoke_async(
    const char* method,
    const char* json_args,
    pingcore_async_invoke_cb_t cb,
    void* user_data
) {
    return pingcore::NativeBridge::instance().invoke_async(
        method ? method : "",
        json_args ? json_args : "{}",
        [cb, user_data](int status, std::string res) {
            if (cb) {
                cb(0, status, res.c_str(), user_data);
            }
        }
    );
}

void pingcore_native_notify_async_result(
    uint64_t request_id,
    int status,
    const char* json_result
) {
    pingcore::NativeBridge::instance().notify_async_result(request_id, status, json_result);
}

void pingcore_native_cancel_request(uint64_t request_id) {
    pingcore::NativeBridge::instance().cancel_request(request_id);
}

} // extern "C"
