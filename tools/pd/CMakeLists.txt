add_executable(pd
    main.cpp
)

target_link_libraries(pd
    PRIVATE
        PingDart::CLI
        PingDart::PKG
        PingDart::Core
)
