# src/text/CMakeLists.txt

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Freetype QUIET)
find_package(HarfBuzz QUIET)
set_property(GLOBAL PROPERTY PINGCORE_HAS_FREETYPE "${Freetype_FOUND}")
set_property(GLOBAL PROPERTY PINGCORE_HAS_HARFBUZZ "${HarfBuzz_FOUND}")

set(TEXT_SOURCES
    unicode_analyzer.cpp
    font_manager.cpp
    text_shaper.cpp
    paragraph.cpp
    atlas/skyline_packer.cpp
    atlas/glyph_atlas.cpp
    atlas/glyph_cache.cpp
)

if(Freetype_FOUND AND HarfBuzz_FOUND)
    list(APPEND TEXT_SOURCES freetype/freetype_font_face.cpp)
else()
    list(APPEND TEXT_SOURCES font_face_stub.cpp)
endif()

if(HarfBuzz_FOUND)
    list(APPEND TEXT_SOURCES text_renderer.cpp)
endif()

add_library(pingdart_text STATIC ${TEXT_SOURCES})

target_include_directories(pingdart_text
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pingdart_text
    PUBLIC
        PingDart::Core
        PingDart::RHI
)

if(Freetype_FOUND)
    target_compile_definitions(pingdart_text PRIVATE PINGDART_HAS_FREETYPE=1)
    target_include_directories(pingdart_text PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(pingdart_text PUBLIC Freetype::Freetype)
endif()

if(HarfBuzz_FOUND)
    target_compile_definitions(pingdart_text PRIVATE PINGDART_HAS_HARFBUZZ=1)
    target_include_directories(pingdart_text PRIVATE ${HarfBuzz_INCLUDE_DIR})
    target_link_libraries(pingdart_text PUBLIC HarfBuzz::HarfBuzz)
endif()

add_library(PingDart::Text ALIAS pingdart_text)
