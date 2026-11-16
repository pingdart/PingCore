// include/pingdart/core/runtime.h
#pragma once
#include "status.h"
#include <string_view>

namespace pingdart::core {

class Runtime {
public:
    // Initializes the core runtime subsystem
    static Status initialize();

    // Shuts down the core runtime subsystem
    static void shutdown();

    // Returns framework version string
    [[nodiscard]] static std::string_view version() noexcept;

    // Checks whether core runtime is currently initialized
    [[nodiscard]] static bool is_initialized() noexcept;
};

} // namespace pingdart::core
