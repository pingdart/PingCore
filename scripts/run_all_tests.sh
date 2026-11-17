#!/usr/bin/env bash
# PingCore Test Suite Runner (Linux / macOS)
# Executes all registered CTest test suites with failure output

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

echo "=== PingCore Automated Test Suite Runner ==="
echo "Root:  ${ROOT_DIR}"
echo "Build: ${BUILD_DIR}"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "[!] Build directory does not exist. Configuring with CMake..."
    cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" -DBUILD_TESTING=ON
fi

echo "[*] Building tests..."
cmake --build "${BUILD_DIR}" --config Release -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo "[*] Running CTest suites..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure --schedule-random "$@"

echo "=== All PingCore Test Suites Passed Successfully ==="
