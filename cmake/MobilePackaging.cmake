# cmake/MobilePackaging.cmake
# PingDart Mobile Packaging Configuration (Android Gradle/NDK & iOS Xcode/Metal)

option(PINGDART_BUILD_ANDROID "Enable Android platform packaging and NDK targets" OFF)
option(PINGDART_BUILD_IOS "Enable iOS platform packaging and Xcode targets" OFF)

if(ANDROID)
    set(PINGDART_BUILD_ANDROID ON)
    add_compile_definitions(PINGDART_PLATFORM_ANDROID=1)
    message(STATUS "[MobilePackaging] Target is Android (NDK ABI: ${ANDROID_ABI})")
endif()

if(IOS)
    set(PINGDART_BUILD_IOS ON)
    add_compile_definitions(PINGDART_PLATFORM_IOS=1)
    message(STATUS "[MobilePackaging] Target is iOS (SDK: ${CMAKE_OSX_SYSROOT})")
endif()

function(pingdart_package_android TARGET_NAME)
    cmake_parse_arguments(ARG "" "PACKAGE_NAME;VERSION" "ASSETS;LIBRARIES" ${ARGN})
    if(NOT ARG_PACKAGE_NAME)
        set(ARG_PACKAGE_NAME "dev.pingdart.${TARGET_NAME}")
    endif()

    add_custom_target(${TARGET_NAME}_android_apk
        COMMAND ${CMAKE_COMMAND} -E echo "Packaging Android APK for ${TARGET_NAME} (${ARG_PACKAGE_NAME})..."
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/dist/android"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/platforms/android"
        COMMENT "Assembling Android APK for ${TARGET_NAME}"
    )
endfunction()

function(pingdart_package_ios TARGET_NAME)
    cmake_parse_arguments(ARG "" "BUNDLE_ID;VERSION" "ASSETS" ${ARGN})
    if(NOT ARG_BUNDLE_ID)
        set(ARG_BUNDLE_ID "dev.pingdart.${TARGET_NAME}")
    endif()

    add_custom_target(${TARGET_NAME}_ios_app
        COMMAND ${CMAKE_COMMAND} -E echo "Packaging iOS Application Bundle for ${TARGET_NAME} (${ARG_BUNDLE_ID})..."
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/dist/ios"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/platforms/ios"
        COMMENT "Assembling iOS .app for ${TARGET_NAME}"
    )
endfunction()
