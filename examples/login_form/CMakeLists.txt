# examples/login_form/CMakeLists.txt
add_executable(example_login_form main.cpp)
target_include_directories(example_login_form PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(example_login_form PRIVATE cxx_std_20)
