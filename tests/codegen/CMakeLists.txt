pingcore_add_test_suite(pingcore_codegen_tests PingCoreCodegenTests
        LABELS tools codegen
        SOURCES transform.cpp "${PINGCORE_PROJECT_DIR}/tools/codegen/transform.cpp"
)
target_include_directories(pingcore_codegen_tests PRIVATE "${PINGCORE_PROJECT_DIR}/tools/codegen")

pingcore_add_test_suite(pingcore_codegen_runtime_tests PingCoreCodegenRuntimeTests
        LABELS tools codegen runtime
        LIBRARIES ${PINGCORE_TEST_LIBRARY}
        SOURCES runtime.cpp
)
pingcore_enable_codegen(pingcore_codegen_runtime_tests)
