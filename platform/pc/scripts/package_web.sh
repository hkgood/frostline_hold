#!/usr/bin/env bash
# Builds the web version and zips it for itch.io (an HTML game: the zip has
# index.html at its root).
#   platform/pc/scripts/package_web.sh
source "$(dirname "$0")/common.sh"

BUILD="$GAME_ROOT/build-pc/web-release"
emcmake cmake -S "$PC_DIR" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" -j

STAGE="$DIST_DIR/web/frostline-hold-web"
rm -rf "$STAGE"
mkdir -p "$STAGE"
cp "$BUILD"/index.{html,js,wasm,data} "$STAGE/"
copy_notices "$BUILD" "$STAGE/licenses"

ZIP="$DIST_DIR/web/frostline-hold-$VERSION-web.zip"
rm -f "$ZIP"
(cd "$STAGE" && zip -qr "$ZIP" .)
echo "web: $ZIP ($(du -h "$ZIP" | cut -f1))"
