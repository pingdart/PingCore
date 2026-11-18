#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

int run_web_dev_server(
    const std::filesystem::path& project_root,
    const std::string& example_name,
    uint16_t port);
