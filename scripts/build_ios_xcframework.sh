#!/bin/sh

set -eu

fail() {
  printf 'PingCore iOS XCFramework: %s\n' "$1" >&2
  exit 1
}

if [ "$#" -lt 3 ] || [ "$#" -gt 5 ]; then
  fail "usage: build_ios_xcframework.sh <source-dir> <build-dir> <output> [Debug|Release] [jobs]"
fi

source_directory=$1
build_directory=$2
output_path=$3
configuration=${4:-Release}
jobs=${5:-$(sysctl -n hw.ncpu 2>/dev/null || printf '1')}

case "$configuration" in
Debug | Release) ;;
*) fail "configuration must be Debug or Release" ;;
esac
case "$jobs" in
'' | *[!0-9]* | 0) fail "jobs must be a positive integer" ;;
esac
case "$build_directory" in
'' | / | "$source_directory") fail "build directory must be a dedicated path" ;;
esac
case "$output_path" in
*.xcframework) ;;
*) fail "output must use the .xcframework extension" ;;
esac

for command_name in cmake lipo xcodebuild; do
  command -v "$command_name" >/dev/null 2>&1 || fail "'$command_name' is required on PATH"
done

device_build_directory="$build_directory/device"
simulator_build_directory="$build_directory/simulator"
device_library="$device_build_directory/lib/$configuration/libpingcore_static.a"
simulator_library="$simulator_build_directory/lib/$configuration/libpingcore_static.a"
device_testing_library="$device_build_directory/lib/$configuration/libpingcore_testing.a"
simulator_testing_library="$simulator_build_directory/lib/$configuration/libpingcore_testing.a"
testing_output_path="$(dirname "$output_path")/PingCoreTesting.xcframework"
headers_directory="$build_directory/headers"

cmake -S "$source_directory" -B "$device_build_directory" -G Xcode \
  -DPINGCORE_ENABLE_PROFILING=OFF \
  -DPINGCORE_OPTIMIZATION=default -DPINGCORE_ENABLE_LTO=OFF \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DPINGCORE_BUILD_SHARED=OFF \
  -DPINGCORE_BUILD_STATIC=ON \
  -DPINGCORE_BUILD_TESTING_LIBRARY=ON \
  -DPINGCORE_BUILD_CLI=OFF \
  -DPINGCORE_BUILD_EXAMPLES=OFF \
  -DPINGCORE_BUILD_TESTS=OFF
cmake --build "$device_build_directory" --config "$configuration" \
  --target pingcore_static pingcore_testing --parallel "$jobs"

cmake -S "$source_directory" -B "$simulator_build_directory" -G Xcode \
  -DPINGCORE_ENABLE_PROFILING=OFF \
  -DPINGCORE_OPTIMIZATION=default -DPINGCORE_ENABLE_LTO=OFF \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphonesimulator \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DPINGCORE_BUILD_SHARED=OFF \
  -DPINGCORE_BUILD_STATIC=ON \
  -DPINGCORE_BUILD_TESTING_LIBRARY=ON \
  -DPINGCORE_BUILD_CLI=OFF \
  -DPINGCORE_BUILD_EXAMPLES=OFF \
  -DPINGCORE_BUILD_TESTS=OFF
cmake --build "$simulator_build_directory" --config "$configuration" \
  --target pingcore_static pingcore_testing --parallel "$jobs"

[ -f "$device_library" ] || fail "device build did not produce libpingcore_static.a"
[ -f "$simulator_library" ] || fail "Simulator build did not produce libpingcore_static.a"
lipo "$device_library" -verify_arch arm64 || fail "device library does not contain arm64"
lipo "$simulator_library" -verify_arch arm64 || fail "Simulator library does not contain arm64"
lipo "$simulator_library" -verify_arch x86_64 || fail "Simulator library does not contain x86_64"

rm -rf -- "$headers_directory"
mkdir -p "$headers_directory"
cp -R "$source_directory/include/." "$headers_directory/"
cp "$source_directory/platform/ios/PingCorePlatform.modulemap" "$headers_directory/module.modulemap"

mkdir -p "$(dirname "$output_path")"
rm -rf -- "$output_path"
xcodebuild -create-xcframework \
  -library "$device_library" -headers "$headers_directory" \
  -library "$simulator_library" -headers "$headers_directory" \
  -output "$output_path"

lipo "$device_testing_library" -verify_arch arm64 || fail "testing device library does not contain arm64"
lipo "$simulator_testing_library" -verify_arch arm64 || fail "testing Simulator library does not contain arm64"
lipo "$simulator_testing_library" -verify_arch x86_64 || fail "testing Simulator library does not contain x86_64"
rm -rf -- "$testing_output_path"
xcodebuild -create-xcframework \
  -library "$device_testing_library" \
  -library "$simulator_testing_library" \
  -output "$testing_output_path"

for required_path in \
  Info.plist \
  ios-arm64/Headers/module.modulemap \
  ios-arm64/Headers/pingcore/pingcore.h \
  ios-arm64/libpingcore_static.a \
  ios-arm64_x86_64-simulator/Headers/module.modulemap \
  ios-arm64_x86_64-simulator/Headers/pingcore/pingcore.h \
  ios-arm64_x86_64-simulator/libpingcore_static.a; do
  [ -e "$output_path/$required_path" ] || fail "XCFramework is missing $required_path"
done

simulator_slice="$output_path/ios-arm64_x86_64-simulator"
xcrun --sdk iphonesimulator clang \
  -fsyntax-only \
  -fobjc-arc \
  -fmodules \
  -target arm64-apple-ios15.0-simulator \
  -I "$simulator_slice/Headers" \
  "$source_directory/tests/platform/ios/platform_registry.m"
xcrun --sdk iphonesimulator clang++ \
  -c \
  -fobjc-arc \
  -fmodules \
  -std=c++20 \
  -target arm64-apple-ios15.0-simulator \
  -I "$simulator_slice/Headers" \
  "$source_directory/tests/platform/ios/platform_registry.mm" \
  -o "$build_directory/pingcore_ios_direct_factory_test.o"
xcrun --sdk iphonesimulator swiftc \
  -target arm64-apple-ios15.0-simulator \
  -I "$simulator_slice/Headers" \
  "$source_directory/tests/platform/ios/platform_registry.swift" \
  -L "$simulator_slice" \
  -lpingcore_static \
  -lc++ \
  -framework AVFoundation \
  -framework BackgroundTasks \
  -framework Contacts \
  -framework CoreBluetooth \
  -framework CoreFoundation \
  -framework CoreGraphics \
  -framework CoreImage \
  -framework CoreLocation \
  -framework CoreText \
  -framework CoreVideo \
  -framework EventKit \
  -framework Foundation \
  -framework ImageIO \
  -framework Metal \
  -framework MetalPerformanceShaders \
  -framework MobileCoreServices \
  -framework Photos \
  -framework QuartzCore \
  -framework UIKit \
  -framework UserNotifications \
  -Xlinker -weak_framework \
  -Xlinker UniformTypeIdentifiers \
  -o "$build_directory/pingcore_ios_swift_link_test"
