pingcore_add_library(pingcore_test_library
        SOURCES
            src/library.cpp
        RESOURCES
            resources
        RESOURCE_NAMESPACE
            pingcore_test_library
)

target_include_directories(pingcore_test_library PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
add_library(PingCore::test_library ALIAS pingcore_test_library)
