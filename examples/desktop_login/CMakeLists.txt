# examples/desktop_login/CMakeLists.txt

add_executable(desktop_login
    main.cpp
)

target_link_libraries(desktop_login PRIVATE
    PingDart::Core
    PingDart::PAL
    PingDart::RHI
    PingDart::Text
    PingDart::UI
)

if(APPLE AND NOT IOS)
    target_link_libraries(desktop_login PRIVATE pingdart_pal_macos)
endif()
