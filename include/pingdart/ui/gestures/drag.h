// include/pingdart/ui/gestures/drag.h
// PingDartUI Pan & Drag-and-Drop Gestures (PKG-138)
#pragma once

#include <pingdart/ui/gestures/gesture.h>

namespace pingdart::ui {

  class PanEvent : public GestureEvent {
  public:
      float delta_x{0.0f};
      float delta_y{0.0f};
      float total_dx{0.0f};
      float total_dy{0.0f};
      bool is_start{false};
      bool is_end{false};

      PanEvent(float x, float y, float dx, float dy, float tdx, float tdy, bool start = false, bool end = false)
          : GestureEvent(EventType::Pan, x, y),
            delta_x(dx),
            delta_y(dy),
            total_dx(tdx),
            total_dy(tdy),
            is_start(start),
            is_end(end) {}
  };

  class DragDropEvent : public GestureEvent {
  public:
      std::string data{""};
      std::string format{"text/plain"};

      DragDropEvent(EventType type, float x, float y, std::string d, std::string f = "text/plain")
          : GestureEvent(type, x, y),
            data(std::move(d)),
            format(std::move(f)) {}
  };

} // namespace pingdart::ui
