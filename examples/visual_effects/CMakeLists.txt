pingcore_add_example(
        example_visual_effects
        "PingCore Visual Effects"
        "com.pingcore.visual-effects"
)

target_sources(example_visual_effects PRIVATE advanced_effects.cpp spotlight_hover.cpp)

pingcore_add_resources(example_visual_effects
        ROOT "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        NAMESPACE "visual_effects"
)
