# cmake/Sanitizers.cmake
# PingDart Framework - Memory & Concurrency Sanitizer Configuration

option(PINGDART_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(PINGDART_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(PINGDART_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

add_library(pingdart_sanitizers INTERFACE)
add_library(PingDart::Sanitizers ALIAS pingdart_sanitizers)

set(SANITIZER_FLAGS "")

if(PINGDART_ENABLE_TSAN AND (PINGDART_ENABLE_ASAN OR PINGDART_ENABLE_UBSAN))
    message(FATAL_ERROR "ThreadSanitizer (TSan) cannot be combined with AddressSanitizer (ASan) or UBSan.")
endif()

if(PINGDART_ENABLE_ASAN)
    list(APPEND SANITIZER_FLAGS -fsanitize=address)
endif()

if(PINGDART_ENABLE_UBSAN)
    list(APPEND SANITIZER_FLAGS -fsanitize=undefined)
endif()

if(PINGDART_ENABLE_TSAN)
    list(APPEND SANITIZER_FLAGS -fsanitize=thread)
endif()

if(SANITIZER_FLAGS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|AppleClang")
        message(FATAL_ERROR "Selected sanitizers require Clang or GCC compiler.")
    endif()

    target_compile_options(pingdart_sanitizers INTERFACE
        ${SANITIZER_FLAGS}
        -fno-omit-frame-pointer
        -g
    )

    target_link_options(pingdart_sanitizers INTERFACE
        ${SANITIZER_FLAGS}
    )
endif()
