if (CMAKE_CROSSCOMPILING)
    return()
endif ()

if (WIN32)
    find_program(PINGCORE_POWERSHELL_COMMAND NAMES pwsh powershell)
    if (PINGCORE_POWERSHELL_COMMAND)
        add_test(NAME PingCoreToolBuildScriptTests
                COMMAND "${PINGCORE_POWERSHELL_COMMAND}"
                        -NoProfile
                        -ExecutionPolicy Bypass
                        -File "${PINGCORE_PROJECT_DIR}/scripts/build_tools.ps1"
                        -h
        )
        add_test(NAME PingCoreSdkPackageScriptTests
                COMMAND "${PINGCORE_POWERSHELL_COMMAND}"
                        -NoProfile
                        -ExecutionPolicy Bypass
                        -File "${PINGCORE_PROJECT_DIR}/scripts/package_sdk.ps1"
                        -Help
        )
    endif ()
elseif (UNIX)
    add_test(NAME PingCoreToolBuildScriptTests
            COMMAND sh "${PINGCORE_PROJECT_DIR}/scripts/build_tools.sh" -h
    )
    add_test(NAME PingCoreToolBuildGeneratorTests
            COMMAND sh
                    "${PINGCORE_PROJECT_DIR}/tests/scripts/build_tools_generator.sh"
                    "${PINGCORE_PROJECT_DIR}"
    )
    add_test(NAME PingCoreSdkPackageScriptTests
            COMMAND sh "${PINGCORE_PROJECT_DIR}/scripts/package_sdk.sh" --help
    )
endif ()

if (TARGET pingcore_cli_core AND UNIX)
    add_test(NAME PingCoreSdkUpdateTests
            COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/sdk_update.sh"
                    "${PINGCORE_PROJECT_DIR}/install.sh" "$<TARGET_FILE:pingcore_cli>"
    )
endif ()
get_property(script_tests DIRECTORY PROPERTY TESTS)
foreach (script_test IN LISTS script_tests)
    set_tests_properties(${script_test} PROPERTIES LABELS "tools;scripts")
endforeach ()
