if (ANDROID)
    add_library(pingcore_ui_testing_smoke SHARED android.cpp smoke.cpp
            ../android/ui_testing/file_reference.cpp ../android/ui_testing/activity_result.cpp)
    target_include_directories(pingcore_ui_testing_smoke PRIVATE
            "${PINGCORE_PROJECT_DIR}/src" "${PINGCORE_PROJECT_DIR}/platform/android/pingcore/src/main/cpp")
elseif (IOS)
    find_package(XCTest REQUIRED)
    add_executable(pingcore_ui_testing_host MACOSX_BUNDLE ../ios/host.m)
    target_link_libraries(pingcore_ui_testing_host PRIVATE "-framework UIKit")
    set_target_properties(pingcore_ui_testing_host PROPERTIES
            XCODE_GENERATE_SCHEME TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER org.pingcore.testing.host
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER org.pingcore.testing.host
            XCODE_ATTRIBUTE_INFOPLIST_KEY_UILaunchScreen_Generation YES
    )
    xctest_add_bundle(pingcore_ui_testing_smoke pingcore_ui_testing_host ios.mm smoke.cpp
            ../ios/file_reference.mm ../ios/presentation.mm)
    target_include_directories(pingcore_ui_testing_smoke PRIVATE
            "${PINGCORE_PROJECT_DIR}/src" "${PINGCORE_PROJECT_DIR}/platform/ios")
    set_source_files_properties(../ios/file_reference.mm ../ios/presentation.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    # Xcode embeds the test bundle itself; making PlugIns a build directory duplicates its mkdir output.
    set_target_properties(pingcore_ui_testing_smoke PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
            XCODE_GENERATE_SCHEME TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER org.pingcore.testing.smoke
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER org.pingcore.testing.smoke
    )
elseif (EMSCRIPTEN)
    add_executable(pingcore_ui_testing_smoke web.cpp smoke.cpp)
else ()
    add_executable(pingcore_ui_testing_smoke desktop.cpp smoke.cpp)
    enable_testing()
    add_test(NAME PingCoreUiTestingSmoke COMMAND pingcore_ui_testing_smoke)
    set_tests_properties(PingCoreUiTestingSmoke PROPERTIES TIMEOUT 30 LABELS "testing;smoke")
endif ()
target_link_libraries(pingcore_ui_testing_smoke PRIVATE PingCore::testing)
target_compile_options(pingcore_ui_testing_smoke PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive-;/utf-8>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic>"
)
if (ANDROID AND PINGCORE_TESTING_RESOURCE_OUTPUT)
    set_target_properties(pingcore_ui_testing_smoke PROPERTIES
            PINGCORE_RESOURCE_OUTPUT_DIRECTORY "${PINGCORE_TESTING_RESOURCE_OUTPUT}/${ANDROID_ABI}")
endif ()
pingcore_add_resources(pingcore_ui_testing_smoke
        ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/application_resources"
        NAMESPACE ui_testing_smoke
)
_pingcore_append_resource_package_input(pingcore_ui_testing_smoke "${PINGCORE_BUILTIN_RESOURCE_PACKAGE}")
get_target_property(_PINGCORE_SMOKE_PACKAGE pingcore_ui_testing_smoke PINGCORE_RESOURCE_PACKAGE)
target_compile_definitions(pingcore_ui_testing_smoke PRIVATE "PINGCORE_SMOKE_PACKAGE=\"${_PINGCORE_SMOKE_PACKAGE}\"")
if (IOS)
    set(_PINGCORE_SMOKE_BUNDLE "$<TARGET_BUNDLE_CONTENT_DIR:pingcore_ui_testing_smoke>")
    if (XCODE)
        set(_PINGCORE_SMOKE_BUNDLE
                "$<TARGET_BUNDLE_DIR:pingcore_ui_testing_host>/PlugIns/pingcore_ui_testing_smoke.xctest")
    endif ()
    add_custom_command(TARGET pingcore_ui_testing_smoke POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "${_PINGCORE_SMOKE_PACKAGE}"
                    "${_PINGCORE_SMOKE_BUNDLE}/package"
    )
    unset(_PINGCORE_SMOKE_BUNDLE)
elseif (EMSCRIPTEN)
    target_link_options(pingcore_ui_testing_smoke PRIVATE "SHELL:--preload-file \"${_PINGCORE_SMOKE_PACKAGE}@/package\"")
    add_custom_command(TARGET pingcore_ui_testing_smoke POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/web.html"
                    "$<TARGET_FILE_DIR:pingcore_ui_testing_smoke>/index.html"
    )
endif ()
unset(_PINGCORE_SMOKE_PACKAGE)
