#pragma once

#include <windows.h>

#include <functional>

#include <pingcore/app.h>

namespace pingcore::detail {

using Win32WindowReady = std::function<void(UiThreadDispatcher, HWND)>;

int RunWin32PlatformApplication(const Application& application, Win32WindowReady on_window_ready = {});

} // namespace pingcore::detail
