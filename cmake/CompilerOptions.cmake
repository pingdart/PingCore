# cmake/CompilerOptions.cmake
# PingDart Framework - Compiler & Language Configuration

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for Clangd / IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Strict warnings and diagnostic flags
if(MSVC)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
    add_compile_options(
        /W4
        /permissive-
        /Zc:__cplusplus
        /utf-8
    )
    # Enable deterministic compilation if supported
    add_compile_options(/experimental:deterministic)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wnon-virtual-dtor
    )
    # Reproducible build path remapping
    add_compile_options("-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
endif()

# Position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
