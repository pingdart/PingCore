# cmake/toolchains/emscripten.cmake
# PingDart Framework - Emscripten WebAssembly Toolchain Configuration

if(NOT DEFINED EMSCRIPTEN_ROOT_PATH)
    if(DEFINED ENV{EMSDK})
        set(EMSCRIPTEN_ROOT_PATH "$ENV{EMSDK}/upstream/emscripten")
    elseif(DEFINED ENV{EMSCRIPTEN})
        set(EMSCRIPTEN_ROOT_PATH "$ENV{EMSCRIPTEN}")
    endif()
endif()

set(CMAKE_SYSTEM_NAME Emscripten)
set(CMAKE_SYSTEM_VERSION 1)

if(EXISTS "${EMSCRIPTEN_ROOT_PATH}/cmake/Modules/Platform/Emscripten.cmake")
    include("${EMSCRIPTEN_ROOT_PATH}/cmake/Modules/Platform/Emscripten.cmake")
endif()

# C++20 standard flags for WebAssembly
set(PINGDART_WASM_FLAGS
    "-sWASM=1"
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sNO_DISABLE_EXCEPTION_CATCHING"
    "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','allocateUTF8','UTF8ToString']"
)

string(REPLACE ";" " " PINGDART_WASM_FLAGS_STR "${PINGDART_WASM_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PINGDART_WASM_FLAGS_STR}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PINGDART_WASM_FLAGS_STR}")
