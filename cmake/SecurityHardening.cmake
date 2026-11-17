# cmake/SecurityHardening.cmake
# PingDart Framework - Toolchain Security Hardening

option(PINGDART_ENABLE_SECURITY_HARDENING "Enable compiler & linker security hardening flags" ON)

add_library(pingdart_security_hardening INTERFACE)
add_library(PingDart::SecurityHardening ALIAS pingdart_security_hardening)

if(PINGDART_ENABLE_SECURITY_HARDENING)
    if(MSVC)
        target_compile_options(pingdart_security_hardening INTERFACE
            /GS          # Buffer security check
            /guard:cf    # Control Flow Guard
            /sdl         # Additional Security Development Lifecycle checks
        )
        target_link_options(pingdart_security_hardening INTERFACE
            /DYNAMICBASE # ASLR
            /HIGHENTROPYVA # 64-bit ASLR
            /NXCOMPAT    # Data Execution Prevention (DEP)
            /GUARD:CF    # Control Flow Guard
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|AppleClang")
        target_compile_options(pingdart_security_hardening INTERFACE
            -fstack-protector-strong
            -Wformat
            -Wformat-security
        )
        
        # _FORTIFY_SOURCE requires optimization (-O1+) to be active
        if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_definitions(pingdart_security_hardening INTERFACE
                _FORTIFY_SOURCE=2
            )
        endif()

        # Platform-specific linker hardening
        if(UNIX AND NOT APPLE)
            target_compile_options(pingdart_security_hardening INTERFACE
                -fPIE
                -fstack-clash-protection
            )
            target_link_options(pingdart_security_hardening INTERFACE
                -pie
                -Wl,-z,relro
                -Wl,-z,now
                -Wl,-z,noexecstack
            )
        endif()
    endif()
endif()
