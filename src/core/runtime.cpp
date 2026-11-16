// src/core/runtime.cpp
#include <pingdart/core/runtime.h>
#include <atomic>

namespace pingdart::core {

namespace {
std::atomic<bool> g_initialized{false};
constexpr std::string_view kVersion = "0.1.0-alpha";
} // namespace

Status Runtime::initialize() {
    bool expected = false;
    if (!g_initialized.compare_exchange_strong(expected, true)) {
        return Status(StatusCode::AlreadyExists, "Runtime already initialized");
    }
    return Status::Ok();
}

void Runtime::shutdown() {
    g_initialized.store(false);
}

std::string_view Runtime::version() noexcept {
    return kVersion;
}

bool Runtime::is_initialized() noexcept {
    return g_initialized.load();
}

} // namespace pingdart::core
