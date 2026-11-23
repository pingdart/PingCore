#include <gauge/gauge.h>

#include <string>

namespace gauge {

pingcore::View Meter(int level) {
  using namespace pingcore;
  const int clamped = Clamp(level);
  return Row {
    Text("Level"),
    Text(std::to_string(clamped) + " / " + std::to_string(max_level)).With(FontSize(20.0F)),
  }.With(Spacing(8.0F), CrossAlign(CrossAxisAlignment::Center));
}

} // namespace gauge
