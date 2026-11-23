include_guard(GLOBAL)

add_library(pingcore_testing STATIC
        "${PINGCORE_PROJECT_DIR}/testing/ui_test.cpp"
        "${PINGCORE_PROJECT_DIR}/testing/testing_platform_adapter.cpp"
        "${PINGCORE_PROJECT_DIR}/testing/ui_snapshot.cpp"
)
add_library(PingCore::testing ALIAS pingcore_testing)
set_target_properties(pingcore_testing PROPERTIES EXPORT_NAME testing POSITION_INDEPENDENT_CODE ON)
if (WIN32)
    set_target_properties(pingcore_testing PROPERTIES DEBUG_POSTFIX "_debug")
endif ()
target_compile_features(pingcore_testing PUBLIC cxx_std_20)
target_include_directories(pingcore_testing PRIVATE "${PINGCORE_PROJECT_DIR}/src")
target_compile_options(pingcore_testing PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive-;/utf-8>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic>"
)
_pingcore_configure_optimization(pingcore_testing)
if (NOT TARGET PingCore::pingcore_static)
    target_link_libraries(pingcore_testing PUBLIC "$<BUILD_INTERFACE:PingCore::pingcore>")
else ()
    target_link_libraries(pingcore_testing PUBLIC "$<BUILD_INTERFACE:PingCore::pingcore_static>")
endif ()
