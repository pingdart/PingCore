# examples/desktop_host/CMakeLists.txt

add_executable(desktop_host
    main.cpp
)

target_link_libraries(desktop_host PRIVATE
    PingDart::Core
    PingDart::PAL
)

if(WIN32)
    target_link_libraries(desktop_host PRIVATE pingdart_pal_windows)
endif()
