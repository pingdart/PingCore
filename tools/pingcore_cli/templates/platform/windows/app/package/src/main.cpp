#include <windows.h>

#include <pingcore/windows/installer.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, wchar_t*, int) {
  return pingcore::windows::RunInstallerApplication();
}
