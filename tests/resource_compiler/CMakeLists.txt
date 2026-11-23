pingcore_add_test_suite(pingcore_resource_compiler_tests PingCoreResourceCompilerTests
        LABELS tools resources
        LIBRARIES ${PINGCORE_TEST_LIBRARY}
        SOURCES compiler.cpp
                "${PINGCORE_PROJECT_DIR}/tools/resource_compiler/compiler.cpp"
                "${PINGCORE_PROJECT_DIR}/tools/resource_compiler/svg_compiler.cpp"
)
target_include_directories(pingcore_resource_compiler_tests PRIVATE
        "${PINGCORE_PROJECT_DIR}/src/resources"
        "${PINGCORE_PROJECT_DIR}/tools/resource_compiler"
)
