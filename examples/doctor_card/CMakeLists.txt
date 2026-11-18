# examples/doctor_card/CMakeLists.txt
add_executable(example_doctor_card main.cpp)
target_include_directories(example_doctor_card PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(example_doctor_card PRIVATE cxx_std_20)
