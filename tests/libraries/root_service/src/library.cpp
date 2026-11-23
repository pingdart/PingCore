#include <pingcore_test_library/library.h>

#include <memory>

#include <pingcore_test_library_resources.h>

namespace pingcore_test_library {

void Install(pingcore::WindowContext& root) {
  const int value = raw::library_value_txt.Domain() == "pingcore_test_library" ? 42 : 0;
  root.Provide(std::make_shared<Service>(Service{value}));
}

} // namespace pingcore_test_library
