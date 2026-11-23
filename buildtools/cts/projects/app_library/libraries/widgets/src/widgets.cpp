#include <ctskit/widgets.h>

#include <ctskit/core.h>
#include <ctskit_widgets_resources.h>

using namespace pingcore;

namespace ctskit {

[[pingcore::composable]]
View Caption() {
  return Text(ctskit_widgets::strings::caption);
}

std::string UseWidgetsCaption() {
  return UseString(ctskit_widgets::strings::caption);
}

std::string UseWidgetsCoreName() {
  return UseCoreName();
}

} // namespace ctskit
