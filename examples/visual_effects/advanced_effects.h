#pragma once

#include <pingcore/pingcore.h>

namespace visual_effects {

pingcore::View GradientGlowSurface(pingcore::View content);
pingcore::View ShimmerSurface(pingcore::View content);
pingcore::View ParticleFieldSurface(pingcore::View content);
pingcore::View OrbitLoader();
pingcore::View LiquidAuroraSurface(pingcore::View content);

} // namespace visual_effects
