add_library(shared_login_lib STATIC
    login_app.cpp
    login_app.h
)

target_include_directories(shared_login_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(shared_login_lib PUBLIC
    PingDart::Core
    PingDart::RHI
    PingDart::UI
    PingDart::Text
)

add_executable(shared_login
    main.cpp
)

target_link_libraries(shared_login PRIVATE
    shared_login_lib
    PingDart::PAL
)

if(APPLE AND NOT IOS)
    target_link_libraries(shared_login PRIVATE
        pingdart_pal_macos
        "-framework Cocoa"
        "-framework Metal"
        "-framework QuartzCore"
    )
endif()
