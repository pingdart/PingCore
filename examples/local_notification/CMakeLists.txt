pingcore_add_example(example_local_notification "PingCore Local Notifications" "com.pingcore.localnotification")

if (WIN32 AND NOT PINGCORE_WINDOWS_7_COMPAT)
    target_compile_definitions(example_local_notification PRIVATE PINGCORE_EXAMPLE_WINDOWS_LOCAL_NOTIFICATIONS=1)
    target_sources(example_local_notification PRIVATE windows/notification_template.cpp)
endif ()

if (IOS AND TARGET pingcore_static)
    # The extension links the SDK alone, without the app core's force-loaded Application declaration.
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/pingcore-ios/notification-extension-link.rsp"
            CONTENT "\"$<TARGET_FILE:pingcore_static>\"\n$<JOIN:$<TARGET_PROPERTY:pingcore_static,INTERFACE_LINK_LIBRARIES>,\n>\n-lc++\n-framework\nUserNotificationsUI\n")
endif ()
