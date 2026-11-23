set(PINGCORE_WINDOWS_MANIFEST "${CMAKE_CURRENT_LIST_DIR}/app.manifest")
set(PINGCORE_WINDOWS_ICON "${CMAKE_CURRENT_LIST_DIR}/app.ico")
set(PINGCORE_WINDOWS_RESOURCE_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/pingcore-platform/windows"
)
file(MAKE_DIRECTORY "${PINGCORE_WINDOWS_RESOURCE_DIRECTORY}")
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/app.rc.in"
        "${PINGCORE_WINDOWS_RESOURCE_DIRECTORY}/app.rc"
        @ONLY
)
set(PINGCORE_WINDOWS_RESOURCE
        "${PINGCORE_WINDOWS_RESOURCE_DIRECTORY}/app.rc"
)

function(pingcore_configure_windows_project_package target_name install_component)
    if (NOT PINGCORE_PACKAGE)
        return()
    endif ()

    install(TARGETS ${target_name}
            RUNTIME DESTINATION .
            COMPONENT "${install_component}"
    )
    _pingcore_install_runtime_dependencies(${target_name} "${install_component}"
            . "$<TARGET_FILE_NAME:${target_name}>"
    )
    get_target_property(PINGCORE_WINDOWS_APP_RESOURCES
            ${target_name}
            PINGCORE_RESOURCE_PACKAGE
    )
    if (PINGCORE_WINDOWS_APP_RESOURCES
            AND NOT PINGCORE_WINDOWS_APP_RESOURCES MATCHES "-NOTFOUND$")
        install(DIRECTORY "${PINGCORE_WINDOWS_APP_RESOURCES}/"
                DESTINATION "${target_name}.resources"
                COMPONENT "${install_component}"
        )
    endif ()

    string(UUID PINGCORE_WINDOWS_MSI_UPGRADE_CODE
            NAMESPACE 6ba7b810-9dad-11d1-80b4-00c04fd430c8
            NAME "@PROJECT_ID@.msi"
            TYPE SHA1
            UPPER
    )
    string(UUID PINGCORE_WINDOWS_BUNDLE_UPGRADE_CODE
            NAMESPACE 6ba7b810-9dad-11d1-80b4-00c04fd430c8
            NAME "@PROJECT_ID@.bundle"
            TYPE SHA1
            UPPER
    )
    set(PINGCORE_WINDOWS_PACKAGE_DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}/pingcore-package/windows"
    )
    set(PINGCORE_WINDOWS_PROJECT_VERSION "${PROJECT_VERSION}")
    file(MAKE_DIRECTORY "${PINGCORE_WINDOWS_PACKAGE_DIRECTORY}")
    configure_file(
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package/Package.wxs.in"
            "${PINGCORE_WINDOWS_PACKAGE_DIRECTORY}/Package.wxs"
            @ONLY
    )
    configure_file(
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package/Bundle.wxs.in"
            "${PINGCORE_WINDOWS_PACKAGE_DIRECTORY}/Bundle.wxs"
            @ONLY
    )

    file(GLOB_RECURSE PINGCORE_WINDOWS_INSTALLER_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package/src/*.cpp"
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package/src/*.cc"
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package/src/*.cxx"
    )
    pingcore_add_windows_installer(${target_name}_installer
            SOURCES
                ${PINGCORE_WINDOWS_INSTALLER_SOURCES}
                "${PINGCORE_WINDOWS_RESOURCE}"
            RESOURCES
                "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package/resources"
            RESOURCE_NAMESPACE
                installer
            INTEGRATION_OUTPUT
                "${PINGCORE_WINDOWS_PACKAGE_DIRECTORY}/$<CONFIG>/installer.json"
    )
    set_target_properties(${target_name}_installer PROPERTIES
            OUTPUT_NAME "@TARGET_NAME@-Installer"
    )
    file(GENERATE
            OUTPUT "${PINGCORE_WINDOWS_PACKAGE_DIRECTORY}/$<CONFIG>/package.json"
            CONTENT "{\n  \"schema\": 1,\n  \"name\": \"@PROJECT_NAME@\",\n  \"target\": \"@TARGET_NAME@\",\n  \"version\": \"${PROJECT_VERSION}\",\n  \"installComponent\": \"${install_component}\",\n  \"packageSource\": \"${PINGCORE_WINDOWS_PACKAGE_DIRECTORY}/Package.wxs\",\n  \"bundleSource\": \"${PINGCORE_WINDOWS_PACKAGE_DIRECTORY}/Bundle.wxs\",\n  \"installerPlan\": \"${PINGCORE_WINDOWS_PACKAGE_DIRECTORY}/$<CONFIG>/installer.json\"\n}\n"
    )
endfunction()
