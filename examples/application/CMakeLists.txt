pingcore_add_example(
        example_application
        "PingCore Application"
        "com.pingcore.application"
)

if (WIN32)
    target_compile_definitions(example_application PRIVATE PINGCORE_EXAMPLE_WINDOWS_URL_SCHEME=1)
endif ()

if (APPLE AND NOT IOS)
    set_target_properties(example_application PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    )
endif ()
