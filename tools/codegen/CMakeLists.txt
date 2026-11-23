cmake_minimum_required(VERSION 3.20)
project(pingcore_codegen LANGUAGES CXX)

add_executable(pingcore_codegen
        main.cpp
        transform.cpp
)

target_compile_features(pingcore_codegen PRIVATE cxx_std_20)
target_include_directories(pingcore_codegen PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(pingcore_codegen PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:/W4>"
        "$<$<CXX_COMPILER_ID:MSVC>:/permissive->"
        "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>"
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(pingcore_codegen PRIVATE -static-libstdc++ -static-libgcc)
endif ()

set_target_properties(pingcore_codegen PROPERTIES
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "hcg"
)

install(TARGETS pingcore_codegen RUNTIME DESTINATION ".")
