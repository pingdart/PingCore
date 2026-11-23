set -eu

if [ -z "${PINGCORE_HOME:-}" ]; then
  echo "error: PINGCORE_HOME is not configured; set it in Config/Local.xcconfig" >&2
  exit 1
fi

# The integration that builds the application core: the one PINGCORE_BUILD_SYSTEM names, the project's only
# declaration, or CMake when the project declares both. Everything else in this shell is shared.
if [ -z "${PINGCORE_BUILD_SYSTEM:-}" ]; then
  if [ -f "$PINGCORE_PROJECT_ROOT/CMakeLists.txt" ]; then
    PINGCORE_BUILD_SYSTEM=cmake
  else
    PINGCORE_BUILD_SYSTEM=mcpp
  fi
fi

if [ "$PINGCORE_BUILD_SYSTEM" = mcpp ]; then
  # mcpp builds the core for arm64; its archive carries the C++ runtime the core was compiled against, and the
  # shell calls the core only through pingcore::RunApplication(), whose signature names no standard-library type.
  case "$PLATFORM_NAME" in
    iphonesimulator) PINGCORE_MCPP_TARGET=aarch64-ios-sim ;;
    iphoneos) PINGCORE_MCPP_TARGET=aarch64-ios ;;
    *) echo "error: the mcpp integration builds no core for $PLATFORM_NAME" >&2; exit 1 ;;
  esac
  for PINGCORE_ARCH in $ARCHS; do
    if [ "$PINGCORE_ARCH" != arm64 ]; then
      echo "error: the mcpp integration builds the iOS core for arm64 only, not $PINGCORE_ARCH" >&2
      exit 1
    fi
  done
  # The CLI passes the profile and features it resolved from its options; a build from Xcode itself takes the
  # configuration's own profile.
  if [ -z "${PINGCORE_MCPP_PROFILE:-}" ]; then
    PINGCORE_MCPP_PROFILE=dev
    if [ "$CONFIGURATION" = Release ]; then
      PINGCORE_MCPP_PROFILE=release
    fi
  fi
  set -- build --target "$PINGCORE_MCPP_TARGET" --profile "$PINGCORE_MCPP_PROFILE" --message-format json
  if [ -n "${PINGCORE_MCPP_FEATURES:-}" ]; then
    set -- "$@" --features "$PINGCORE_MCPP_FEATURES"
  fi
  mkdir -p "$PINGCORE_CORE_BUILD_DIR/pingcore-ios"
  PINGCORE_MCPP_REPORT="$PINGCORE_CORE_BUILD_DIR/mcpp-build.json"
  PINGCORE_MCPP_COMMAND=${PINGCORE_MCPP:-mcpp}
  # Xcode exports every build setting to this script, over 500 variables, and mcpp cannot start a program whose
  # environment holds more than 512 entries; it reports that as running out of memory. The settings describe this
  # shell's build, not the core's, and the iOS SDK and deployment target among them would also reach the compiler of
  # the build programs mcpp runs on this Mac. mcpp therefore gets the user's environment without them.
  (
    cd "$PINGCORE_PROJECT_ROOT"
    for PINGCORE_NAME in $(env | sed -n 's/^\([A-Za-z_][A-Za-z0-9_]*\)=.*/\1/p'); do
      case "$PINGCORE_NAME" in
        HOME | USER | LOGNAME | PATH | SHELL | TMPDIR | TERM | LANG | LC_* | DEVELOPER_DIR | MCPP_* | XLINGS_* | \
          *_proxy | *_PROXY | SSL_CERT_*) ;;
        *) unset "$PINGCORE_NAME" 2> /dev/null || true ;;
      esac
    done
    exec "$PINGCORE_MCPP_COMMAND" "$@"
  ) > "$PINGCORE_MCPP_REPORT"
  # The application's own artifact; dependencies such as the compiler runtime report library artifacts too.
  PINGCORE_MCPP_PROGRAM=$(grep '"event":"artifact"' "$PINGCORE_MCPP_REPORT" |
    grep -E '"kind":"(bin|app|shared)"' | sed -n 's/.*"path":"\([^"]*\)".*/\1/p' | head -n 1)
  if [ -z "$PINGCORE_MCPP_PROGRAM" ]; then
    echo "error: mcpp reported no application artifact; see $PINGCORE_MCPP_REPORT" >&2
    exit 1
  fi
  PINGCORE_MCPP_CORE=$(dirname "$PINGCORE_MCPP_PROGRAM")/pingcore-ios
  set -- "$PINGCORE_MCPP_CORE"/*/
  if [ "$#" -ne 1 ] || [ ! -f "$1/link.rsp" ]; then
    echo "error: mcpp produced no application core under $PINGCORE_MCPP_CORE" >&2
    exit 1
  fi
  # The core keeps the location mcpp gave it; the shell reads it where it reads a CMake core.
  rm -rf "$PINGCORE_CORE_BUILD_DIR/pingcore-ios/@TARGET_NAME@"
  ln -s "${1%/}" "$PINGCORE_CORE_BUILD_DIR/pingcore-ios/@TARGET_NAME@"
  exit 0
fi

PINGCORE_CMAKE_ARCHS=$(printf '%s' "$ARCHS" | tr ' ' ';')
"${PINGCORE_CMAKE_EXECUTABLE:-cmake}" -S "$PINGCORE_PROJECT_ROOT" -B "$PINGCORE_CORE_BUILD_DIR" \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT="$SDKROOT" \
  -DCMAKE_OSX_ARCHITECTURES="$PINGCORE_CMAKE_ARCHS" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$IPHONEOS_DEPLOYMENT_TARGET" \
  -DCMAKE_BUILD_TYPE="$CONFIGURATION" \
  -DPINGCORE_HOME="$PINGCORE_HOME" \
  -DPINGCORE_OPTIMIZATION="${PINGCORE_OPTIMIZATION:-default}" \
  -DPINGCORE_ENABLE_LTO="${PINGCORE_ENABLE_LTO:-OFF}" \
  -DPINGCORE_ENABLE_PROFILING="${PINGCORE_ENABLE_PROFILING:-OFF}" \
  -DPINGCORE_BUILD_SHARED=OFF \
  -DPINGCORE_BUILD_TESTS=OFF \
  -DPINGCORE_BUILD_EXAMPLES=OFF \
  -DPINGCORE_BUILD_CLI=OFF
# A bare --parallel lets Make start every compile at once, which exhausts memory on a small Mac.
"${PINGCORE_CMAKE_EXECUTABLE:-cmake}" --build "$PINGCORE_CORE_BUILD_DIR" --target @TARGET_NAME@_pingcore_ios_core --parallel "$(sysctl -n hw.logicalcpu)"
