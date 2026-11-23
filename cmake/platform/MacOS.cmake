set(PINGCORE_PLATFORM_SOURCE_FILES
        "${PINGCORE_PROJECT_DIR}/platform/macos/appkit_accessibility.mm"
        "${PINGCORE_PROJECT_DIR}/platform/macos/appkit_adapter.mm"
        "${PINGCORE_PROJECT_DIR}/platform/macos/appkit_platform_view.mm"
        "${PINGCORE_PROJECT_DIR}/platform/macos/appkit_renderer.mm"
        "${PINGCORE_PROJECT_DIR}/platform/macos/appkit_system_tray.mm"
        "${PINGCORE_PROJECT_DIR}/platform/macos/appkit_text_input.mm"
        "${PINGCORE_PROJECT_DIR}/platform/macos/macos_application.mm"
        "${PINGCORE_PROJECT_DIR}/platform/macos/macos_external_texture.mm"
        "${PINGCORE_PROJECT_DIR}/platform/macos/macos_file.mm"
        "${PINGCORE_PROJECT_DIR}/platform/macos/macos_file_picker.mm"
        "${PINGCORE_PROJECT_DIR}/platform/macos/macos_http.mm"
        "${PINGCORE_PROJECT_DIR}/platform/macos/macos_platform_registry.mm"
)
set(PINGCORE_PLATFORM_COMPILE_OPTIONS
        -fobjc-arc
)
set(PINGCORE_PLATFORM_LINK_LIBRARIES
        "-framework AppKit"
        "-framework AVFoundation"
        "-framework Carbon"
        "-framework Contacts"
        "-framework CoreBluetooth"
        "-framework CoreGraphics"
        "-framework CoreImage"
        "-framework CoreLocation"
        "-framework CoreText"
        "-framework CoreVideo"
        "-framework EventKit"
        "-framework ImageIO"
        "-framework Foundation"
        "-framework Metal"
        "-framework MetalPerformanceShaders"
        "-framework Photos"
        "-framework QuartzCore"
        "-framework UserNotifications"
        "-weak_framework UniformTypeIdentifiers"
)
