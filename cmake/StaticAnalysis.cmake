# cmake/StaticAnalysis.cmake
# PingDart Framework - Static Analysis Integration

option(PINGDART_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)
option(PINGDART_ENABLE_CPPCHECK "Enable cppcheck static analysis" OFF)

if(PINGDART_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-18 clang-tidy-17)
    if(CLANG_TIDY_EXE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY
            "${CLANG_TIDY_EXE};-checks=-*,bugprone-*,cert-*,clang-analyzer-*,misc-*,performance-*,readability-*;--warnings-as-errors=bugprone-*,cert-*"
        )
    else()
        message(WARNING "clang-tidy requested but executable not found in PATH")
    endif()
endif()

if(PINGDART_ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES cppcheck)
    if(CPPCHECK_EXE)
        message(STATUS "Found cppcheck: ${CPPCHECK_EXE}")
        set(CMAKE_CXX_CPPCHECK
            "${CPPCHECK_EXE};--enable=warning,style,performance,portability;--inline-suppr;--suppress=missingIncludeSystem"
        )
    else()
        message(WARNING "cppcheck requested but executable not found in PATH")
    endif()
endif()
