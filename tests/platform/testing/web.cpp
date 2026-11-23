#include "smoke.h"

#include <emscripten/bind.h>

EMSCRIPTEN_BINDINGS(pingcore_ui_testing_smoke) {
  emscripten::function("runUiTestingSmoke", +[] { return RunUiTestingSmoke("/package"); });
}
