#include <windows.h>
#include <pingcore/pingcore.h>

#include <memory>
#include <type_traits>

namespace {

class WindowsFirstRuntime : public pingcore::Runtime {
  std::unique_ptr<pingcore::UiWindow> CreateUiWindow(pingcore::ViewFactory, const pingcore::WindowOptions&) override {
    return {};
  }
};

static_assert(std::is_same_v<
    decltype(CreateWindow(nullptr, nullptr, 0, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr)), HWND>);

}
