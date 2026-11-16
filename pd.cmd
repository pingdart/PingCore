@echo off
setlocal
set "DIR=%~dp0"
set "BIN=%DIR%build\tools\pd\pd.exe"

if not exist "%BIN%" (
    if exist "%DIR%build\tools\pd\Debug\pd.exe" set "BIN=%DIR%build\tools\pd\Debug\pd.exe"
    if exist "%DIR%build\tools\pd\Release\pd.exe" set "BIN=%DIR%build\tools\pd\Release\pd.exe"
)

if not exist "%BIN%" (
    echo [pd] Building PingDart CLI...
    cmake --build "%DIR%build" --target pd 2>nul
)

if exist "%BIN%" (
    "%BIN%" %*
) else (
    echo [pd] PingDart CLI wrapper
    echo [pd] To build natively: cmake -B build -G Ninja ^&^& ninja -C build pd
)
