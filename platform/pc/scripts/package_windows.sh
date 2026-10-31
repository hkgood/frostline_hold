#!/usr/bin/env bash
# Cross-builds the Windows version with MinGW-w64 and zips it.
#   platform/pc/scripts/package_windows.sh           # itch.io
#   platform/pc/scripts/package_windows.sh --steam   # Steam (see steam_cmake_args)
source "$(dirname "$0")/common.sh"

FLAVOUR=itch
EXTRA=()
if [[ "${1:-}" == "--steam" ]]; then
    FLAVOUR=steam
    read -r -a EXTRA <<< "$(steam_cmake_args)"
fi

BUILD="$GAME_ROOT/build-pc/windows-$FLAVOUR"
cmake -S "$PC_DIR" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
    --toolchain "$PC_DIR/cmake/mingw-w64-x86_64.cmake" ${EXTRA[@]+"${EXTRA[@]}"}
cmake --build "$BUILD" -j

STAGE="$DIST_DIR/windows-$FLAVOUR/Frostline Hold"
rm -rf "$STAGE"
mkdir -p "$STAGE"
cp "$BUILD/Frostline Hold.exe" "$STAGE/"
x86_64-w64-mingw32-strip "$STAGE/Frostline Hold.exe"
cp -R "$BUILD/assets" "$STAGE/assets"
if [[ "$FLAVOUR" == steam ]]; then
    cp "$BUILD/steam_api64.dll" "$STAGE/"
fi
copy_notices "$BUILD" "$STAGE/licenses"

ZIP="$DIST_DIR/windows-$FLAVOUR/frostline-hold-$VERSION-windows-$FLAVOUR.zip"
rm -f "$ZIP"
(cd "$(dirname "$STAGE")" && zip -qr "$ZIP" "Frostline Hold")
echo "windows ($FLAVOUR): $ZIP ($(du -h "$ZIP" | cut -f1))"
