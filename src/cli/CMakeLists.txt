add_library(pingdart_cli STATIC
    toolchain.cpp
    builder.cpp
    font_bundler.cpp
    packager.cpp
    verifier.cpp
)

target_include_directories(pingdart_cli
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pingdart_cli
    PUBLIC
        PingDart::Core
)

if(TARGET Freetype::Freetype)
    target_compile_definitions(pingdart_cli PRIVATE PINGDART_HAS_FREETYPE=1)
    target_include_directories(pingdart_cli PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(pingdart_cli PRIVATE Freetype::Freetype)
endif()

if(TARGET HarfBuzz::HarfBuzz)
    target_compile_definitions(pingdart_cli PRIVATE PINGDART_HAS_HARFBUZZ=1)
    target_include_directories(pingdart_cli PRIVATE ${HarfBuzz_INCLUDE_DIR})
    target_link_libraries(pingdart_cli PRIVATE HarfBuzz::HarfBuzz)
endif()

add_library(PingDart::CLI ALIAS pingdart_cli)
