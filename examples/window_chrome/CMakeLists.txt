pingcore_add_example(
        example_window_chrome
        "PingCore Window Chrome"
        "com.pingcore.window_chrome"
)

pingcore_add_resources(example_window_chrome
        ROOT "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        NAMESPACE "window_chrome"
)
