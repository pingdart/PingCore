pingcore_add_example(example_image "PingCore Image" "org.pingcore.example.image")

pingcore_add_resources(example_image
        ROOT "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        NAMESPACE "image_example"
)
