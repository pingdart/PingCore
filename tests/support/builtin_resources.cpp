#include "runtime_test_support.h"
#include "resources/resource_internal.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace pingcore::test {

namespace {

class BuiltinResources final : public PlatformResources {
public:
  ResourceConfiguration Configuration() const override {
    return {};
  }

  std::optional<pingcore::InputStream> OpenRead(std::string_view package_path) override {
    const std::filesystem::path path =
        std::filesystem::path(PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE) / std::filesystem::path(std::string(package_path));
    return pingcore::detail::OpenPackageFile(path);
  }
};

} // namespace

PlatformResources* BuiltinTestResources() {
  static BuiltinResources resources;
  return &resources;
}

} // namespace pingcore::test
