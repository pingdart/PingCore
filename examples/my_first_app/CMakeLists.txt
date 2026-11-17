add_executable(my_first_app
    main.cpp
)

target_include_directories(my_first_app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(my_first_app PRIVATE
    PingDart::Core
    PingDart::PAL
    PingDart::RHI
    PingDart::UI
    PingDart::Text
)

if(APPLE AND NOT IOS)
    target_link_libraries(my_first_app PRIVATE pingdart_pal_macos)
endif()
