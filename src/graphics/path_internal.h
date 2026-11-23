#pragma once

#include <array>

#include <pingcore/vector.h>

namespace pingcore::detail {

enum class PathVerb {
  MoveTo,
  LineTo,
  QuadraticTo,
  CubicTo,
  Close,
};

struct PathElement {
  PathVerb verb = PathVerb::MoveTo;
  // MoveTo and LineTo use points[0], QuadraticTo uses points[0..1], and CubicTo uses points[0..2].
  std::array<Point, 3> points{};

  bool operator==(const PathElement&) const = default;
};

[[nodiscard]] Path CreateBorderStrokePath(Rect rect, CornerRadii corner_radii, float width);

} // namespace pingcore::detail
