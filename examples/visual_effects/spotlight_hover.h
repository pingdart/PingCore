#pragma once

#include <pingcore/pingcore.h>

namespace visual_effects {

struct SpotlightHover {
  class Extension;

  float radius = 90.0F;
  float corner_radius = 14.0F;
  double fade_in_duration = 0.4;
  double fade_out_duration = 0.18;
  pingcore::Color hover_tint = pingcore::Color::Rgb(88, 115, 168, 0.16F);

  bool operator==(const SpotlightHover&) const = default;
};

pingcore::View SpotlightButton(pingcore::StringVariant label);

} // namespace visual_effects
