# cmake/Fuzzing.cmake
# PingDart Coverage-Guided Fuzzing Support (Clang libFuzzer / OSS-Fuzz)

option(PINGDART_BUILD_LIBFUZZER "Enable Clang coverage-guided libFuzzer targets" OFF)

if(PINGDART_BUILD_LIBFUZZER)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Coverage-guided libFuzzer requires Clang compiler")
    endif()

    set(FUZZER_FLAGS "-fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FUZZER_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FUZZER_FLAGS}")

    message(STATUS "Coverage-guided libFuzzer enabled with Address/Undefined sanitizers")
endif()
