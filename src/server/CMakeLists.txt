# src/server/CMakeLists.txt
add_library(pingcore_server STATIC
    server.cpp
)

target_include_directories(pingcore_server
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pingcore_server
    PUBLIC
        PingDart::Core
)

# PKG-128: OpenSSL HTTPS Support
find_package(OpenSSL QUIET)
if(NOT OpenSSL_FOUND)
    if(EXISTS "/opt/homebrew/include/openssl/ssl.h")
        set(OPENSSL_INCLUDE_DIR "/opt/homebrew/include")
        find_library(OPENSSL_SSL_LIBRARY NAMES ssl libssl PATHS "/opt/homebrew/lib")
        find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto libcrypto PATHS "/opt/homebrew/lib")
        set(OPENSSL_LIBRARIES ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
        set(OpenSSL_FOUND TRUE)
    endif()
endif()

if(OpenSSL_FOUND)
    target_include_directories(pingcore_server PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(pingcore_server PUBLIC ${OPENSSL_LIBRARIES})
    target_compile_definitions(pingcore_server PUBLIC PINGCORE_ENABLE_OPENSSL=1)
    message(STATUS "[PingCore] OpenSSL HTTPS support enabled: ${OPENSSL_INCLUDE_DIR}")
endif()

add_library(PingCore::Server ALIAS pingcore_server)
