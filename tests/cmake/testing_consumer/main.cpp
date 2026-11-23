#include <pingcore/testing/ui_test.h>

namespace {
pingcore::View Content() {
  using namespace pingcore;
  auto count = UseState(0);
  return Button(std::to_string(count.Get())).OnClick([count]() mutable { count = count.Get() + 1; });
}
}

int main() {
  using namespace pingcore::testing;
  pingcore::Application application(Content, {.show_debug_overlay = false});
  UiTest ui(application);
  ui.Find(UiSelector::Text("0")).Tap();
  return ui.Find(UiSelector::Text("1")).Exists() ? 0 : 1;
}
