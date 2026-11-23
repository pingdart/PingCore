include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/PingCoreLibraries.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/PingCoreSeo.cmake")

set(PINGCORE_BUILD_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(pingcore_configure_compile_target target_name)
    target_compile_features(${target_name} PRIVATE cxx_std_20)
    target_include_directories(${target_name} PRIVATE
            "${PINGCORE_PUBLIC_INCLUDE_DIR}"
            "${PINGCORE_PROJECT_DIR}/src"
            ${PINGCORE_PLATFORM_INCLUDE_DIRECTORIES}
    )
    target_compile_options(${target_name} PRIVATE
            "$<$<CXX_COMPILER_ID:MSVC>:/W4>"
            "$<$<CXX_COMPILER_ID:MSVC>:/permissive->"
            "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>"
            ${PINGCORE_PLATFORM_COMPILE_OPTIONS}
    )
    target_compile_definitions(${target_name} PRIVATE
            ${PINGCORE_PLATFORM_COMPILE_DEFINITIONS}
    )
    if (PINGCORE_ENABLE_PROFILING)
        target_compile_definitions(${target_name} PRIVATE PINGCORE_ENABLE_PROFILING=1)
    endif ()
    _pingcore_configure_optimization(${target_name})
endfunction()

function(pingcore_configure_public_target target_name resource_package)
    set_property(TARGET ${target_name} PROPERTY
            PINGCORE_RESOURCE_PACKAGE "${resource_package}"
    )
    target_include_directories(${target_name}
            PUBLIC
            $<BUILD_INTERFACE:${PINGCORE_PUBLIC_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    target_compile_options(${target_name} INTERFACE ${PINGCORE_PLATFORM_INTERFACE_COMPILE_OPTIONS})
    if (PINGCORE_ENABLE_PROFILING)
        set_property(TARGET ${target_name} PROPERTY PINGCORE_SOURCE_PROFILING ON)
        target_compile_definitions(${target_name} INTERFACE $<BUILD_INTERFACE:PINGCORE_ENABLE_PROFILING=1>)
    endif ()
    target_link_libraries(${target_name} PRIVATE ${PINGCORE_PLATFORM_LINK_LIBRARIES})
    target_link_options(${target_name} INTERFACE ${PINGCORE_PLATFORM_LINK_OPTIONS})
    get_target_property(PINGCORE_TARGET_TYPE ${target_name} TYPE)
    if (ANDROID AND PINGCORE_TARGET_TYPE STREQUAL "STATIC_LIBRARY")
        _pingcore_configure_android_static_target(${target_name})
    endif ()
    if (EMSCRIPTEN)
        set_property(TARGET ${target_name} APPEND PROPERTY
                INTERFACE_LINK_DEPENDS
                "$<BUILD_INTERFACE:${PINGCORE_PROJECT_DIR}/platform/web/web_platform_registry.js>"
                "$<BUILD_INTERFACE:${PINGCORE_PROJECT_DIR}/platform/web/web_file.js>"
        )
    endif ()

    if (WIN32)
        get_target_property(PINGCORE_TARGET_TYPE ${target_name} TYPE)
        if (PINGCORE_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
            set_target_properties(${target_name} PROPERTIES
                    WINDOWS_EXPORT_ALL_SYMBOLS ON
            )
        endif ()
    endif ()
endfunction()

function(pingcore_configure_targets)
    if (PINGCORE_LIBRARY_GRAPH_ONLY)
        _pingcore_configure_library_graph_framework()
        return()
    endif ()
    if (NOT PINGCORE_BUILD_SHARED AND NOT PINGCORE_BUILD_STATIC)
        message(FATAL_ERROR "At least one PingCore library target must be enabled")
    endif ()
    if ((EMSCRIPTEN OR IOS) AND PINGCORE_BUILD_SHARED)
        message(FATAL_ERROR "PingCore Web and iOS support the static library target only")
    endif ()

    set(PINGCORE_PLATFORM_SOURCE_FILES)
    set(PINGCORE_PLATFORM_COMPILE_OPTIONS)
    set(PINGCORE_PLATFORM_INTERFACE_COMPILE_OPTIONS)
    set(PINGCORE_PLATFORM_COMPILE_DEFINITIONS)
    set(PINGCORE_PLATFORM_LINK_LIBRARIES)
    set(PINGCORE_PLATFORM_INCLUDE_DIRECTORIES)
    set(PINGCORE_PLATFORM_LINK_OPTIONS)

    if (EMSCRIPTEN)
        include("${PINGCORE_BUILD_CMAKE_DIR}/platform/Web.cmake")
    elseif (ANDROID)
        include("${PINGCORE_BUILD_CMAKE_DIR}/platform/Android.cmake")
    elseif (IOS)
        include("${PINGCORE_BUILD_CMAKE_DIR}/platform/IOS.cmake")
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        include("${PINGCORE_BUILD_CMAKE_DIR}/platform/MacOS.cmake")
    elseif (WIN32)
        include("${PINGCORE_BUILD_CMAKE_DIR}/platform/Windows.cmake")
    elseif (UNIX AND NOT APPLE)
        include("${PINGCORE_BUILD_CMAKE_DIR}/platform/Linux.cmake")
    else ()
        message(FATAL_ERROR "PingCore currently supports Android, iOS, macOS, Windows, Linux, and Web only")
    endif ()

    file(GLOB PINGCORE_CORE_SOURCE_FILES CONFIGURE_DEPENDS
            "${PINGCORE_PROJECT_DIR}/src/*/*.cpp"
    )
    if (NOT PINGCORE_ENABLE_PROFILING)
        list(REMOVE_ITEM PINGCORE_CORE_SOURCE_FILES "${PINGCORE_PROJECT_DIR}/src/runtime/profiling.cpp")
    endif ()

    if (IOS)
        set(PINGCORE_LIBRARY_SOURCE_FILES
                ${PINGCORE_CORE_SOURCE_FILES}
                ${PINGCORE_PLATFORM_SOURCE_FILES}
        )
    else ()
        add_library(pingcore_core_objects OBJECT
                ${PINGCORE_CORE_SOURCE_FILES}
                ${PINGCORE_PLATFORM_SOURCE_FILES}
        )
        set_target_properties(pingcore_core_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)
        pingcore_configure_compile_target(pingcore_core_objects)
        _pingcore_compile_builtin_resources(
                pingcore_core_objects
                PINGCORE_BUILTIN_RESOURCE_PACKAGE
        )
        set(PINGCORE_LIBRARY_SOURCE_FILES $<TARGET_OBJECTS:pingcore_core_objects>)
    endif ()

    if (PINGCORE_BUILD_SHARED)
        add_library(${PINGCORE_SHARED_LIB_NAME} SHARED
                ${PINGCORE_LIBRARY_SOURCE_FILES}
        )
        if (WIN32)
            set_target_properties(${PINGCORE_SHARED_LIB_NAME} PROPERTIES DEBUG_POSTFIX "_debug")
        endif ()
        pingcore_configure_public_target(
                ${PINGCORE_SHARED_LIB_NAME}
                "${PINGCORE_BUILTIN_RESOURCE_PACKAGE}"
        )
        _pingcore_configure_optimization(${PINGCORE_SHARED_LIB_NAME})
        add_library(PingCore::pingcore ALIAS ${PINGCORE_SHARED_LIB_NAME})
    endif ()

    if (PINGCORE_BUILD_STATIC)
        add_library(${PINGCORE_STATIC_LIB_NAME} STATIC
                ${PINGCORE_LIBRARY_SOURCE_FILES}
        )
        if (IOS)
            set_target_properties(${PINGCORE_STATIC_LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
            pingcore_configure_compile_target(${PINGCORE_STATIC_LIB_NAME})
            _pingcore_compile_builtin_resources(
                    ${PINGCORE_STATIC_LIB_NAME}
                    PINGCORE_BUILTIN_RESOURCE_PACKAGE
            )
        endif ()
        if (WIN32)
            set_target_properties(${PINGCORE_STATIC_LIB_NAME} PROPERTIES DEBUG_POSTFIX "_debug")
        endif ()
        pingcore_configure_public_target(
                ${PINGCORE_STATIC_LIB_NAME}
                "${PINGCORE_BUILTIN_RESOURCE_PACKAGE}"
        )
        if (NOT IOS)
            _pingcore_configure_optimization(${PINGCORE_STATIC_LIB_NAME})
        endif ()
        add_library(PingCore::pingcore_static ALIAS ${PINGCORE_STATIC_LIB_NAME})
        if (NOT TARGET PingCore::pingcore)
            add_library(PingCore::pingcore ALIAS ${PINGCORE_STATIC_LIB_NAME})
        endif ()
    endif ()

    set(PINGCORE_BUILTIN_RESOURCE_PACKAGE "${PINGCORE_BUILTIN_RESOURCE_PACKAGE}" PARENT_SCOPE)
endfunction()

function(pingcore_install_libraries)
    include(GNUInstallDirs)
    if (PINGCORE_ENABLE_PROFILING)
        install(CODE
                "message(FATAL_ERROR \"PingCore SDK installation requires PINGCORE_ENABLE_PROFILING=OFF\")"
                COMPONENT PingCoreLibraries)
    endif ()

    # The Termux host SDK consumes the separately assembled Android target libraries.
    if (ANDROID AND PINGCORE_BUILD_CLI)
        return()
    endif ()

    if (TARGET ${PINGCORE_SHARED_LIB_NAME})
        install(TARGETS ${PINGCORE_SHARED_LIB_NAME} EXPORT PingCoreSharedTargets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT PingCoreLibraries
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT PingCoreLibraries
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT PingCoreLibraries)
    endif ()
    if (TARGET ${PINGCORE_STATIC_LIB_NAME})
        install(TARGETS ${PINGCORE_STATIC_LIB_NAME} EXPORT PingCoreStaticTargets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT PingCoreLibraries
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT PingCoreLibraries
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT PingCoreLibraries)
    endif ()
    if (TARGET pingcore_testing)
        install(TARGETS pingcore_testing EXPORT PingCoreTestingTargets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT PingCoreLibraries
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT PingCoreLibraries
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT PingCoreLibraries)
    endif ()
endfunction()
