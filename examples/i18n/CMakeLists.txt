pingcore_add_example(
        example_i18n
        "PingCore i18n"
        "com.pingcore.i18n"
)

pingcore_add_resources(example_i18n
        ROOT "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        NAMESPACE "i18n"
)
