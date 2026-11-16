# examples/web_hello/CMakeLists.txt

add_executable(web_hello
    main.cpp
)

target_link_libraries(web_hello PRIVATE
    PingDart::Web
    PingDart::Core
)
