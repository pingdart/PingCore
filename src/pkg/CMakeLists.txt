# src/pkg/CMakeLists.txt
# PingDart Package Manager Library (PKG-101..PKG-109)

add_library(pingdart_pkg STATIC
    semver.cpp
    manifest.cpp
    lockfile.cpp
    archive.cpp
    registry_client.cpp
    resolver.cpp
    package_manager.cpp
    security_scanner.cpp
    registry_server.cpp
)

target_include_directories(pingdart_pkg
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pingdart_pkg
    PUBLIC
        PingDart::Core
)

add_library(PingDart::PKG ALIAS pingdart_pkg)
