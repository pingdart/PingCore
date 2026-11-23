pingcore_add_test_suite(pingcore_android_logic_tests PingCoreAndroidLogicTests
        LABELS platform android portable
        LIBRARIES ${PINGCORE_TEST_LIBRARY}
        SOURCES accessibility.cpp text_input.cpp
                "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_accessibility.cpp"
)
target_include_directories(pingcore_android_logic_tests PRIVATE
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp"
)

if (ANDROID)
    pingcore_add_test_suite(pingcore_android_texture_tests PingCoreAndroidTextureTests
            LABELS platform android native
            LIBRARIES ${PINGCORE_TEST_LIBRARY}
            SOURCES external_texture.cpp
    )
endif ()

if (NOT CMAKE_CROSSCOMPILING)
    find_package(Java 11 QUIET COMPONENTS Development)
    find_package(JNI QUIET)
    if (Java_FOUND AND JNI_FOUND)
        add_library(pingcore_android_http_test_jni SHARED http_jni.cpp)
        target_include_directories(pingcore_android_http_test_jni PRIVATE ${JNI_INCLUDE_DIRS})
        target_compile_options(pingcore_android_http_test_jni PRIVATE
                "$<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->"
                "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic>")
        set(http_classes "${CMAKE_CURRENT_BINARY_DIR}/http_classes")
        set(http_request "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/java/org/pingcore/PingCoreHttpRequest.java")
        set(http_test "${CMAKE_CURRENT_SOURCE_DIR}/java/PingCoreHttpRequestTest.java")
        add_custom_command(
                OUTPUT "${http_classes}/org/pingcore/PingCoreHttpRequestTest.class"
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${http_classes}"
                COMMAND "${Java_JAVAC_EXECUTABLE}" --release 8 -d "${http_classes}" "${http_request}" "${http_test}"
                DEPENDS "${http_request}" "${http_test}"
                VERBATIM
        )
        add_custom_target(pingcore_android_http_tests ALL
                DEPENDS "${http_classes}/org/pingcore/PingCoreHttpRequestTest.class" pingcore_android_http_test_jni)
        add_test(NAME PingCoreAndroidHttpTests COMMAND "${Java_JAVA_EXECUTABLE}" -cp "${http_classes}"
                org.pingcore.PingCoreHttpRequestTest "$<TARGET_FILE:pingcore_android_http_test_jni>")
        set_tests_properties(PingCoreAndroidHttpTests PROPERTIES LABELS "platform;android;http" TIMEOUT 45)
    endif ()
endif ()
