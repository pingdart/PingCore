#include "color_stream.h"

namespace pingcore::example {

std::shared_ptr<ColorStreamService> UseColorStream() {
  return UseService<ColorStreamService>();
}

} // namespace pingcore::example
