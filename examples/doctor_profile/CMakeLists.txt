# examples/doctor_profile/CMakeLists.txt
add_executable(example_doctor_profile main.cpp)
target_include_directories(example_doctor_profile PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(example_doctor_profile PRIVATE cxx_std_20)
