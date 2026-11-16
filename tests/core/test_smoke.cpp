// tests/core/test_smoke.cpp
// Core Runtime Smoke Test for ENG-101 verification

#include <pingdart/core/runtime.h>
#include <pingdart/core/result.h>
#include <iostream>
#include <cassert>

int main() {
    using namespace pingdart::core;

    std::cout << "[Test] Running PingDart Core Smoke Test..." << std::endl;

    // 1. Initial state
    assert(!Runtime::is_initialized());

    // 2. Initialize
    Status s1 = Runtime::initialize();
    assert(s1.ok());
    assert(Runtime::is_initialized());

    // 3. Duplicate initialize
    Status s2 = Runtime::initialize();
    assert(!s2.ok());
    assert(s2.code() == StatusCode::AlreadyExists);

    // 4. Version check
    std::string_view ver = Runtime::version();
    assert(!ver.empty());
    std::cout << "[Test] PingDart Version: " << ver << std::endl;

    // 5. Result<T> test
    Result<int> res_ok(42);
    assert(res_ok.has_value());
    assert(res_ok.value() == 42);

    Result<int> res_err(Status(StatusCode::NotFound, "Value not found"));
    assert(!res_err.has_value());
    assert(res_err.status().code() == StatusCode::NotFound);

    // 6. Shutdown
    Runtime::shutdown();
    assert(!Runtime::is_initialized());

    std::cout << "[Test] All Core Smoke Tests Passed Cleanly!" << std::endl;
    return 0;
}
