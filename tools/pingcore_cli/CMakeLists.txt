cmake_minimum_required(VERSION 3.20)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(pingcore_cli VERSION 0.5.0 LANGUAGES CXX)
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/../../3dparty/cmrc/CMakeRC.cmake")

file(GLOB_RECURSE PINGCORE_CLI_TEMPLATE_FILES CONFIGURE_DEPENDS
        LIST_DIRECTORIES FALSE
        "${CMAKE_CURRENT_SOURCE_DIR}/templates/*"
)
cmrc_add_resource_library(pingcore_cli_template_resources
        NAMESPACE pingcore_cli_templates
        WHENCE "${CMAKE_CURRENT_SOURCE_DIR}/templates"
        ${PINGCORE_CLI_TEMPLATE_FILES}
)

# The mcpp package templates are embedded from the repository root rather than
# copied here. `mcpp new --template` reads that same tree once PingCore is
# published, so the CLI and mcpp instantiate the same files; only the renderer
# differs. A second copy under this directory would be the drift this project
# spends its parity check preventing.
file(GLOB_RECURSE PINGCORE_CLI_PACKAGE_TEMPLATE_FILES CONFIGURE_DEPENDS
        LIST_DIRECTORIES FALSE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../templates/*"
)
cmrc_add_resources(pingcore_cli_template_resources
        WHENCE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
        ${PINGCORE_CLI_PACKAGE_TEMPLATE_FILES}
)
cmrc_add_resource_library(pingcore_cli_installer_resources
        NAMESPACE pingcore_cli_installers
        WHENCE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
        "${CMAKE_CURRENT_SOURCE_DIR}/../../install.sh"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../install.ps1"
)

add_library(pingcore_cli_core STATIC
        cli.cpp
        platform.cpp
        platform/android.cpp
        platform/ios.cpp
        platform/linux.cpp
        platform/macos.cpp
        platform/web.cpp
        platform/windows.cpp
        mcpp/mcpp.cpp
        process_runner.cpp
        project.cpp
        prompt.cpp
        prompt_terminal.cpp
        sdk.cpp
        toolchain.cpp
        template.cpp
        watch.cpp
)
target_compile_features(pingcore_cli_core PUBLIC cxx_std_20)
target_compile_definitions(pingcore_cli_core PRIVATE
        PINGCORE_CLI_VERSION="${PROJECT_VERSION}"
)
target_include_directories(pingcore_cli_core PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(pingcore_cli_core PRIVATE pingcore_cli_template_resources pingcore_cli_installer_resources)
target_compile_options(pingcore_cli_core PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:/W4>"
        "$<$<CXX_COMPILER_ID:MSVC>:/permissive->"
        "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>"
)
set_target_properties(pingcore_cli_core PROPERTIES CXX_EXTENSIONS OFF)

add_executable(pingcore_cli main.cpp)
target_link_libraries(pingcore_cli PRIVATE pingcore_cli_core)
target_compile_options(pingcore_cli PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:/W4>"
        "$<$<CXX_COMPILER_ID:MSVC>:/permissive->"
        "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>"
)
set_target_properties(pingcore_cli PROPERTIES
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "pingcore"
)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(GNUInstallDirs)
    install(TARGETS pingcore_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()
