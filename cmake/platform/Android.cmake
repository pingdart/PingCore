set(PINGCORE_PLATFORM_SOURCE_FILES
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_accessibility.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_adapter.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_application.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_external_texture.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_file.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_file_picker.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_gpu_texture.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_http.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_jni.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_platform_view.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_renderer.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp/android_text_layout.cpp"
)

set(PINGCORE_PLATFORM_LINK_LIBRARIES android EGL GLESv2 log)
