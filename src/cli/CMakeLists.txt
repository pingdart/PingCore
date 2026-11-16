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
    PRIVATE
        ${FREETYPE_INCLUDE_DIRS}
        ${HarfBuzz_INCLUDE_DIR}
)

target_link_libraries(pingdart_cli
    PUBLIC
        PingDart::Core
        Freetype::Freetype
        HarfBuzz::HarfBuzz
)

add_library(PingDart::CLI ALIAS pingdart_cli)
