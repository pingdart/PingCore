pingcore_add_example(
        example_buffer_reference
        "PingCore Buffer Reference"
        "com.pingcore.buffer-reference"
)

target_sources(example_buffer_reference PRIVATE frame_source.cpp)

if (ANDROID)
    target_sources(example_buffer_reference PRIVATE android_frame_source.cpp)
elseif (APPLE)
    target_sources(example_buffer_reference PRIVATE apple_frame_source.mm)
    target_compile_options(example_buffer_reference PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
elseif (NOT EMSCRIPTEN)
    target_sources(example_buffer_reference PRIVATE direct_frame_source.cpp)
endif ()
