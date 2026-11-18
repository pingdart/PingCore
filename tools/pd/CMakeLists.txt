add_executable(pd
    main.cpp
    web_dev_server.cpp
)

target_compile_definitions(pd PRIVATE
    PINGCORE_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    PINGCORE_BUILD_DIR="${PROJECT_BINARY_DIR}"
)

target_link_libraries(pd
    PRIVATE
        PingDart::CLI
        PingDart::PKG
        PingDart::Core
        PingDart::UI
)

if(TARGET PingDart::Text)
    target_link_libraries(pd PRIVATE PingDart::Text)
endif()

if(TARGET pingcore_server)
    target_link_libraries(pd PRIVATE pingcore_server)
endif()
