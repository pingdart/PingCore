// src/pal/linux/linux_platform_host_factory.h
#pragma once
#include <pingdart/pal/i_platform_host.h>
#include <pingdart/core/result.h>
#include <memory>

namespace pingdart::pal::linux_pal {

/// Creates the appropriate Linux platform host, automatically preferring Wayland
/// when available and falling back to X11 when Wayland is absent.
core::Result<std::unique_ptr<IPlatformHost>> create_linux_platform_host();

} // namespace pingdart::pal::linux_pal
