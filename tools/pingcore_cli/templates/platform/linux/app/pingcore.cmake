function(pingcore_configure_linux_project_package target_name install_component)
    if (NOT PINGCORE_PACKAGE)
        return()
    endif ()

    set(package_directory usr)
    if (PINGCORE_LINUX_SYSTEM_PACKAGE)
        set(package_directory "usr/lib/@PROJECT_ID@")
    endif ()
    install(TARGETS ${target_name}
            RUNTIME DESTINATION "${package_directory}/bin"
            COMPONENT "${install_component}"
    )
    _pingcore_install_runtime_dependencies(${target_name} "${install_component}"
            "${package_directory}" "bin/$<TARGET_FILE_NAME:${target_name}>"
    )
    get_target_property(PINGCORE_LINUX_APP_RESOURCES
            ${target_name}
            PINGCORE_RESOURCE_PACKAGE
    )
    if (PINGCORE_LINUX_APP_RESOURCES
            AND NOT PINGCORE_LINUX_APP_RESOURCES MATCHES "-NOTFOUND$")
        install(DIRECTORY "${PINGCORE_LINUX_APP_RESOURCES}/"
                DESTINATION "${package_directory}/bin/$<TARGET_FILE_BASE_NAME:${target_name}>.resources"
                COMPONENT "${install_component}"
        )
    endif ()
    if (PINGCORE_LINUX_SYSTEM_PACKAGE)
        file(READ "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package/@TARGET_NAME@.desktop" desktop_entry)
        string(REGEX REPLACE "(^|\n)Exec=[^\n]*" "\\1Exec=/${package_directory}/bin/$<TARGET_FILE_NAME:${target_name}>"
                desktop_entry "${desktop_entry}")
        string(REGEX REPLACE "(^|\n)Icon=[^\n]*" "\\1Icon=@PROJECT_ID@" desktop_entry "${desktop_entry}")
        set(installed_desktop_entry "${CMAKE_CURRENT_BINARY_DIR}/pingcore-package/$<CONFIG>/@PROJECT_ID@.desktop")
        file(GENERATE OUTPUT "${installed_desktop_entry}" CONTENT "${desktop_entry}")
        install(FILES "${installed_desktop_entry}" DESTINATION usr/share/applications COMPONENT "${install_component}")
        install(FILES "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package/@TARGET_NAME@.svg"
                DESTINATION usr/share/icons/hicolor/scalable/apps
                RENAME "@PROJECT_ID@.svg" COMPONENT "${install_component}")
        file(CONFIGURE OUTPUT "${CMAKE_BINARY_DIR}/pingcore-package/LinuxCPack.cmake" CONTENT [=[
set(CPACK_PACKAGE_NAME "@PROJECT_ID@")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "@PROJECT_NAME@")
set(CPACK_PACKAGE_DESCRIPTION "@PROJECT_NAME@")
set(CPACK_PACKAGING_INSTALL_PREFIX "/")
set(CPACK_PACKAGE_RELOCATABLE OFF)

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "@PROJECT_ID@")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgtk-4-1 (>= 4.14), librsvg2-common")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_RPM_PACKAGE_DESCRIPTION "@PROJECT_NAME@")
set(CPACK_RPM_PACKAGE_REQUIRES "gtk4 >= 4.14")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        /usr/share/applications /usr/share/icons /usr/share/icons/hicolor
        /usr/share/icons/hicolor/scalable /usr/share/icons/hicolor/scalable/apps)
set(PINGCORE_PACKAGE_APP_DIRECTORY "@package_directory@")
set(CPACK_PRE_BUILD_SCRIPTS "@CMAKE_CURRENT_FUNCTION_LIST_DIR@/package/rpm_dependencies.cmake")
]=] @ONLY)
        return()
    endif ()
    install(PROGRAMS "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package/AppRun"
            DESTINATION .
            COMPONENT "${install_component}"
    )
    install(FILES
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package/@TARGET_NAME@.desktop"
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package/@TARGET_NAME@.svg"
            DESTINATION .
            COMPONENT "${install_component}"
    )
endfunction()
