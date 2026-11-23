pingcore_add_example(
        example_custom_editor
        "PingCore Custom Editor"
        "com.pingcore.custom-editor"
)

pingcore_add_resources(example_custom_editor
        ROOT "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        NAMESPACE "custom_editor"
)
