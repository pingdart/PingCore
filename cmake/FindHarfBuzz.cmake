find_path(HarfBuzz_INCLUDE_DIR
    NAMES hb.h
    PATH_SUFFIXES harfbuzz
    PATHS /opt/homebrew/include /usr/local/include /usr/include
)

find_library(HarfBuzz_LIBRARY
    NAMES harfbuzz
    PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HarfBuzz
    REQUIRED_VARS HarfBuzz_LIBRARY HarfBuzz_INCLUDE_DIR
)

if(HarfBuzz_FOUND AND NOT TARGET HarfBuzz::HarfBuzz)
    add_library(HarfBuzz::HarfBuzz UNKNOWN IMPORTED)
    set_target_properties(HarfBuzz::HarfBuzz PROPERTIES
        IMPORTED_LOCATION "${HarfBuzz_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${HarfBuzz_INCLUDE_DIR}"
    )
endif()
