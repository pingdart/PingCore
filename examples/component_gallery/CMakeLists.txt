# examples/component_gallery/CMakeLists.txt
add_executable(example_component_gallery main.cpp)
target_include_directories(example_component_gallery PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(example_component_gallery PRIVATE cxx_std_20)
