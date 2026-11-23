set -eu

PINGCORE_RESOURCE_SOURCE="$PINGCORE_CORE_BUILD_DIR/pingcore-ios/@TARGET_NAME@/resources/package"
PINGCORE_RESOURCE_DESTINATION="$TARGET_BUILD_DIR/$UNLOCALIZED_RESOURCES_FOLDER_PATH/PingCore"
if [ ! -f "$PINGCORE_RESOURCE_SOURCE/pingcore/resources.bin" ]; then
  echo "error: PingCore resource package is missing: $PINGCORE_RESOURCE_SOURCE" >&2
  exit 1
fi

"${PINGCORE_CMAKE_EXECUTABLE:-cmake}" -E remove_directory "$PINGCORE_RESOURCE_DESTINATION"
"${PINGCORE_CMAKE_EXECUTABLE:-cmake}" -E make_directory "$PINGCORE_RESOURCE_DESTINATION"
"${PINGCORE_CMAKE_EXECUTABLE:-cmake}" -E copy_directory "$PINGCORE_RESOURCE_SOURCE" "$PINGCORE_RESOURCE_DESTINATION"

if [ -n "${PINGCORE_INTEGRATION_PLAN:-}" ]; then
  mkdir -p "$(dirname "$PINGCORE_INTEGRATION_PLAN")"
  {
    printf '{\n'
    printf '  "schema": 1,\n'
    printf '  "target": "%s",\n' "@TARGET_NAME@"
    printf '  "artifact": "%s",\n' "$TARGET_BUILD_DIR/$EXECUTABLE_PATH"
    printf '  "bundle": "%s",\n' "$TARGET_BUILD_DIR/$WRAPPER_NAME"
    printf '  "bundleIdentifier": "%s"\n' "$PRODUCT_BUNDLE_IDENTIFIER"
    printf '}\n'
  } > "$PINGCORE_INTEGRATION_PLAN"
fi
