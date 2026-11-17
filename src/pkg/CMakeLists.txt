# src/pkg/CMakeLists.txt
# PingDart Package Manager Library (PKG-101..PKG-110)

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
    scoped_package.cpp
    multi_registry.cpp
    build_sandbox.cpp
    os_sandbox.cpp
    database.cpp
    object_storage.cpp
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

if(WIN32)
    target_link_libraries(pingdart_pkg PUBLIC bcrypt)
endif()

add_library(PingDart::PKG ALIAS pingdart_pkg)
