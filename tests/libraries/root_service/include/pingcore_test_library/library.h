#pragma once

#include <pingcore/window.h>

namespace pingcore_test_library {

struct Service {
  int value = 0;
};

void Install(pingcore::WindowContext& root);

} // namespace pingcore_test_library
