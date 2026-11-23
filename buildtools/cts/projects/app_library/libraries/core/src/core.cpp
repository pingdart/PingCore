#include <ctskit/core.h>

#include <pingcore/pingcore.h>

#include <ctskit_core_resources.h>

namespace ctskit {

std::string UseCoreName() {
  return pingcore::UseString(ctskit_core::strings::name);
}

} // namespace ctskit
