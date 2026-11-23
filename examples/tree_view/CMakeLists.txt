pingcore_add_example(
        example_tree_view
        "PingCore File Tree"
        "com.pingcore.file-tree"
)

pingcore_add_resources(example_tree_view
        ROOT "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        NAMESPACE "tree_view"
)
