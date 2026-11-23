#pragma once

#include <emscripten/val.h>

#include <pingcore/web/external_texture.h>

namespace pingcore::detail {

void CloseWebVideoFrame(emscripten::val& frame) noexcept;

} // namespace pingcore::detail
