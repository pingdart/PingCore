# src/safety/CMakeLists.txt
# PingDart Framework - Safety Primitives Library

add_library(pingdart_safety STATIC
    secure_memory.cpp
)

target_include_directories(pingdart_safety
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pingdart_safety
    PUBLIC
        PingDart::Core
    PRIVATE
        $<BUILD_INTERFACE:PingDart::Sanitizers>
        $<BUILD_INTERFACE:PingDart::SecurityHardening>
)

add_library(PingDart::Safety ALIAS pingdart_safety)
