pingcore_add_example(
        example_ui_gallery
        "PingCore UI Gallery"
        "com.pingcore.ui-gallery"
)

pingcore_add_resources(example_ui_gallery
        ROOT "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        NAMESPACE "ui_gallery"
)
