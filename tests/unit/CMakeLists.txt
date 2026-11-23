pingcore_add_test_suite(pingcore_unit_tests PingCoreUnitTests
        LABELS common unit
        LIBRARIES ${PINGCORE_TEST_LIBRARY}
        SOURCES
            animation.cpp
            application.cpp
            buffer_reference_example.cpp
            data.cpp
            external_texture.cpp
            file.cpp
            geometry.cpp
            platform_frame.cpp
            paint.cpp
            path.cpp
            platform_payload.cpp
            resource.cpp
            seo.cpp
            semantics.cpp
            stream.cpp
            text.cpp
            text_input.cpp
            text_selection.cpp
            validation.cpp
            vector.cpp
            "${PINGCORE_PROJECT_DIR}/examples/buffer_reference/frame_source.cpp"
            "${PINGCORE_TEST_DIR}/support/builtin_resources.cpp"
)
target_compile_definitions(pingcore_unit_tests PRIVATE
        PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE="${PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE}"
)
