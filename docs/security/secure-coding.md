# PingDart Secure Coding Guidelines
**PKG-116: Safe C++ Development & Security Assurance Standards**

---

## 1. Core Principles
1. **Never Assume Buffer Bounds**: Use `SafeSpan<T>` or `SafeBuffer<T>` instead of raw C-pointers and manual length tracking.
2. **Checked Arithmetic for Dynamic Sizing**: Never multiply dimensions, strides, or buffer sizes without `checked_mul` or `Checked<T>`.
3. **Explicit Non-Null Guarantees**: Use `NonNull<T*>` or `NonNullPtr<unique_ptr<T>>` to enforce non-null invariants at API boundaries.
4. **Scrub Cryptographic Secrets**: Store tokens, passphrases, and keys in `SecureBuffer`, not `std::string`.
5. **Fail Closed**: If permissions, sandbox profiles, or network security constraints cannot be verified, fail with `StatusCode::PermissionDenied` or `StatusCode::Unauthenticated`.
6. **Strict RefCount Adoption Invariants**: Never call `adopt_ref(new T(...))`. PingDart's `RefCounted` initializes count to 0; `adopt_ref` expects a pre-claimed reference count (`ref_count >= 1`).
7. **Connection-Time Destination Verification**: Never rely solely on URL hostnames to prevent SSRF. Always inspect the resolved socket IP address immediately prior to connection.

---

## 2. API Usage Reference

### Safe Spans
```cpp
#include <pingdart/safety/safe_buffer.h>

void process_pixels(pingdart::safety::SafeSpan<const uint8_t> pixels) {
    for (size_t i = 0; i < pixels.size(); ++i) {
        uint8_t byte = pixels.at(i); // Guaranteed bounds checked
    }
}
```

### Checked Math
```cpp
#include <pingdart/safety/checked_arithmetic.h>

auto total_bytes = pingdart::safety::checked_mul(width, height);
if (!total_bytes.has_value()) {
    return StatusCode::InvalidArgument;
}
```

### Reference Counting & `adopt_ref` Guidelines
```cpp
#include <pingdart/core/ref_ptr.h>

// CORRECT: Creating a newly allocated RefCounted object
auto widget = pingdart::core::make_ref<MyWidget>();
// Or explicit RefPtr construction (increments count from 0 to 1):
pingdart::core::RefPtr<MyWidget> widget2(new MyWidget());

// DANGEROUS / FORBIDDEN:
// auto bad = pingdart::core::adopt_ref(new MyWidget()); // Causes Use-After-Free!

// CORRECT USE OF adopt_ref:
// Only when taking ownership of a pointer whose reference count was already incremented,
// such as a C/FFI API returning an already-retained object or after ptr.detach().
MyWidget* detached = widget.detach();
auto reacquired = pingdart::core::adopt_ref(detached);
```

### Path Sanitization & Anti-Zip Slip
```cpp
#include <pingdart/security/input_validator.h>

auto safe_path = pingdart::security::InputValidator::validate_path_within_root(
    untrusted_user_input,
    allowed_workspace_directory
);
if (!safe_path.has_value()) {
    // Path traversal attempt detected and blocked
}
```

### SSRF & Connection-Time IP Validation
```cpp
#include <pingdart/security/input_validator.h>

// 1. Initial URL validation
auto parsed = pingdart::security::InputValidator::validate_package_url(untrusted_url);
if (!parsed.has_value()) {
    return StatusCode::InvalidArgument;
}

// 2. Connection-time resolved IP inspection (defeats DNS rebinding)
if (!pingdart::security::InputValidator::validate_socket_destination(resolved_ip)) {
    // Blocked: RFC1918, loopback, link-local, cloud metadata, or ULA
    return StatusCode::PermissionDenied;
}

// 3. HTTP redirect safety
if (!pingdart::security::InputValidator::validate_redirect(original_url, redirect_url)) {
    // Blocked: scheme downgrade (https -> http) or redirect to private network
    return StatusCode::PermissionDenied;
}
```

### Secure Secrets Storage
```cpp
#include <pingdart/safety/secure_memory.h>

pingdart::safety::SecureBuffer secret(api_key);
secret.lock_memory(); // Page-locked in RAM (mlock / VirtualLock)
// Memory is automatically zeroed via explicit_bzero/memset_s on destruction
```
