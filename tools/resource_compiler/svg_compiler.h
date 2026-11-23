#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace pingcore::resource_compiler {

struct CompiledSvg {
  std::vector<std::byte> payload;
  float intrinsic_width = 0.0F;
  float intrinsic_height = 0.0F;
};

CompiledSvg CompileSvg(const std::filesystem::path& path);

} // namespace pingcore::resource_compiler
