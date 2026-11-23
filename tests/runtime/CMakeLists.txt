pingcore_add_test_suite(pingcore_runtime_tests PingCoreRuntimeTests
        LABELS common runtime
        LIBRARIES ${PINGCORE_TEST_LIBRARY}
        SOURCES
            animation.cpp
            shared_transition.cpp
            application.cpp
            clipboard.cpp
            composition.cpp
            controls.cpp
            file_picker.cpp
            file_drop.cpp
            gestures.cpp
            http.cpp
            interaction.cpp
            layers.cpp
            layout.cpp
            split_layout.cpp
            dock_layout.cpp
            lifecycle.cpp
            navigation.cpp
            navigation_ui.cpp
            local_notification.cpp
            permission.cpp
            platform_module.cpp
            platform_view.cpp
            presentation.cpp
            presentation_semantics.cpp
            semantics.cpp
            profiling.cpp
            render_scene.cpp
            resources.cpp
            scrolling.cpp
            combo_box.cpp
            date_time_picker.cpp
            select.cpp
            tree.cpp
            system_tray.cpp
            system_task.cpp
            selection_area.cpp
            task.cpp
            text.cpp
            text_field.cpp
            text_input.cpp
            theme.cpp
            tooltip.cpp
            virtual_layout.cpp
            window.cpp
            "${PINGCORE_TEST_DIR}/support/builtin_resources.cpp"
)
if (TARGET PingCore::testing)
    pingcore_add_test_suite(pingcore_ui_tests PingCoreUiTests
            LABELS common runtime testing
            LIBRARIES PingCore::testing
            SOURCES ui_testing.cpp
    )
    pingcore_add_resources(pingcore_ui_tests
            ROOT "${PINGCORE_TEST_DIR}/libraries/application_resources"
            NAMESPACE ui_test_fixture
    )
    _pingcore_append_resource_package_input(pingcore_ui_tests "${PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE}")
    target_compile_definitions(pingcore_ui_tests PRIVATE
            "PINGCORE_UI_TEST_PACKAGE=\"${CMAKE_CURRENT_BINARY_DIR}/pingcore-resources/pingcore_ui_tests/package\""
    )
endif ()
target_compile_definitions(pingcore_runtime_tests PRIVATE
        PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE="${PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE}"
        $<$<BOOL:${PINGCORE_ENABLE_PROFILING}>:PINGCORE_ENABLE_PROFILING=1>
)

pingcore_add_test_suite(pingcore_library_tests PingCoreLibraryTests
        LABELS common runtime libraries
        LIBRARIES ${PINGCORE_TEST_LIBRARY}
        SOURCES libraries.cpp
)
pingcore_use_library(pingcore_library_tests
        TARGET PingCore::test_library
        PATH "${PINGCORE_TEST_DIR}/libraries/root_service"
)
pingcore_add_resources(pingcore_library_tests
        ROOT "${PINGCORE_TEST_DIR}/libraries/application_resources"
        NAMESPACE pingcore_test_library
)
target_compile_definitions(pingcore_library_tests PRIVATE
        "PINGCORE_LIBRARY_TEST_RESOURCE_PACKAGE=\"${CMAKE_CURRENT_BINARY_DIR}/pingcore-resources/pingcore_library_tests/package\""
)
