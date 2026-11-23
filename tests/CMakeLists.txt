if (TARGET PingCore::pingcore_static)
    set(PINGCORE_TEST_LIBRARY PingCore::pingcore_static)
else ()
    set(PINGCORE_TEST_LIBRARY PingCore::pingcore)
endif ()

include("${PINGCORE_PROJECT_DIR}/3dparty/catch2/pingcore_3p.cmake")
if (EMSCRIPTEN)
    target_compile_options(pingcore_3p_catch2 PRIVATE -fexceptions)
endif ()

file(TO_CMAKE_PATH "${PINGCORE_BUILTIN_RESOURCE_PACKAGE}" PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE)
set(PINGCORE_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

function(pingcore_add_test_suite target_name test_name)
    cmake_parse_arguments(TEST "" "MAIN" "SOURCES;LIBRARIES;LABELS" ${ARGN})
    if (NOT TEST_MAIN)
        set(TEST_MAIN "${PINGCORE_TEST_DIR}/tests_main.cpp")
    endif ()
    add_executable(${target_name} "${TEST_MAIN}" ${TEST_SOURCES})
    target_compile_features(${target_name} PRIVATE cxx_std_20)
    target_compile_options(${target_name} PRIVATE
            "$<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive-;/utf-8>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic>"
    )
    target_include_directories(${target_name} PRIVATE
            "${PINGCORE_PROJECT_DIR}/src"
            "${PINGCORE_TEST_DIR}/support"
    )
    target_link_libraries(${target_name} PRIVATE PingCore3p::Catch2 ${TEST_LIBRARIES})
    if (EMSCRIPTEN)
        set_target_properties(${target_name} PROPERTIES _PINGCORE_WEB_TEST TRUE SUFFIX ".cjs")
        # Deep scene tests exceed Node's default V8 call stack with Asyncify-instrumented Wasm.
        # This host stack limit is separate from Asyncify's saved stack below.
        set_property(TARGET ${target_name} APPEND PROPERTY CROSSCOMPILING_EMULATOR --stack-size=4096)
        target_compile_options(${target_name} PRIVATE -fexceptions)
        target_link_options(${target_name} PRIVATE
                -fexceptions
                "-sMODULARIZE=0"
                "-sEXPORT_ES6=0"
                "-sENVIRONMENT=node"
                "-sNO_EXIT_RUNTIME=0"
                "-sNODERAWFS=1"
                "-sASYNCIFY=1"
                "-sASYNCIFY_STACK_SIZE=65536"
        )
    endif ()
    add_test(NAME ${test_name} COMMAND ${target_name})
    set_tests_properties(${test_name} PROPERTIES LABELS "${TEST_LABELS}")
endfunction()

add_subdirectory(unit)
add_subdirectory(runtime)
add_subdirectory(platform)
add_subdirectory(codegen)
add_subdirectory(resource_compiler)
add_subdirectory(cli)
add_subdirectory(cmake)
add_subdirectory(scripts)
include(support/header_checks.cmake)
