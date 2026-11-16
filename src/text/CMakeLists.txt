# src/text/CMakeLists.txt

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Freetype REQUIRED)
find_package(HarfBuzz REQUIRED)

add_library(pingdart_text STATIC
    unicode_analyzer.cpp
    font_manager.cpp
    text_shaper.cpp
    paragraph.cpp
    freetype/freetype_font_face.cpp
    atlas/skyline_packer.cpp
    atlas/glyph_atlas.cpp
    atlas/glyph_cache.cpp
    text_renderer.cpp
)

target_include_directories(pingdart_text
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${FREETYPE_INCLUDE_DIRS}
        ${HarfBuzz_INCLUDE_DIR}
)

target_link_libraries(pingdart_text
    PUBLIC
        PingDart::Core
        PingDart::RHI
        Freetype::Freetype
        HarfBuzz::HarfBuzz
)

add_library(PingDart::Text ALIAS pingdart_text)
