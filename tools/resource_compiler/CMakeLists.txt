cmake_minimum_required(VERSION 3.20)
project(pingcore_resource_compiler LANGUAGES CXX)

add_executable(pingcore_resource_compiler
        main.cpp
        compiler.cpp
        svg_compiler.cpp
)

target_compile_features(pingcore_resource_compiler PRIVATE cxx_std_20)
target_include_directories(pingcore_resource_compiler PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../src/resources")
target_compile_options(pingcore_resource_compiler PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:/W4>"
        "$<$<CXX_COMPILER_ID:MSVC>:/permissive->"
        "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>"
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(pingcore_resource_compiler PRIVATE -static-libstdc++ -static-libgcc)
endif ()

set_target_properties(pingcore_resource_compiler PROPERTIES
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "hrc"
)

install(TARGETS pingcore_resource_compiler RUNTIME DESTINATION ".")
