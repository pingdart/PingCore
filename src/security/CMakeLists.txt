# src/security/CMakeLists.txt
# PingDart Framework - Security Subsystem Library

add_library(pingdart_security STATIC
    input_validator.cpp
    secure_random.cpp
    secure_storage.cpp
)

target_include_directories(pingdart_security
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pingdart_security
    PUBLIC
        PingDart::Core
        PingDart::Safety
    PRIVATE
        $<BUILD_INTERFACE:PingDart::Sanitizers>
        $<BUILD_INTERFACE:PingDart::SecurityHardening>
)

if(APPLE)
    target_link_libraries(pingdart_security PUBLIC "-framework Security")
elseif(WIN32)
    target_link_libraries(pingdart_security PUBLIC bcrypt)
endif()

add_library(PingDart::Security ALIAS pingdart_security)
