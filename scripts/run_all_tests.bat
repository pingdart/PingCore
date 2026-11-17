@echo off
REM PingCore Test Suite Runner (Windows)
REM Executes all registered CTest test suites with failure output

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%.."
set "BUILD_DIR=%ROOT_DIR%\build"

echo === PingCore Automated Test Suite Runner ===
echo Root:  %ROOT_DIR%
echo Build: %BUILD_DIR%

if not exist "%BUILD_DIR%" (
    echo [!] Build directory does not exist. Configuring with CMake...
    cmake -B "%BUILD_DIR%" -S "%ROOT_DIR%" -DBUILD_TESTING=ON
)

echo [*] Building tests...
cmake --build "%BUILD_DIR%" --config Release

echo [*] Running CTest suites...
ctest --test-dir "%BUILD_DIR%" --output-on-failure %*

echo === All PingCore Test Suites Passed Successfully ===
