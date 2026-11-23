if (TARGET PingCore3p::Catch2)
    return()
endif ()

add_library(pingcore_3p_catch2
        ${CMAKE_CURRENT_LIST_DIR}/src/catch_amalgamated.cpp
)

target_compile_features(pingcore_3p_catch2 PUBLIC cxx_std_14)
target_compile_definitions(pingcore_3p_catch2 PUBLIC CATCH_AMALGAMATED_CUSTOM_MAIN=ON)

target_include_directories(pingcore_3p_catch2
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include/catch2
)

add_library(PingCore3p::Catch2 ALIAS pingcore_3p_catch2)
