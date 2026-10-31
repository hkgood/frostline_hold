# Shared by the package_*.sh scripts; source it, do not run it.
# shellcheck shell=bash
set -euo pipefail

PC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GAME_ROOT="$(cd "$PC_DIR/../.." && pwd)"
DIST_DIR="$GAME_ROOT/dist"
VERSION="$(sed -n 's/^project(frostline_hold_pc VERSION \([0-9.]*\).*/\1/p' "$PC_DIR/CMakeLists.txt")"

# The Engine checkout, found as CMakeLists.txt finds it.
find_engine() {
    if [[ -n "${MOSAICO_ROOT:-}" ]]; then
        echo "$MOSAICO_ROOT/submodule/raylib-lite-engine"
    elif [[ -d "$GAME_ROOT/../../submodule/raylib-lite-engine" ]]; then
        echo "$GAME_ROOT/../../submodule/raylib-lite-engine"
    else
        echo "$GAME_ROOT/../ESP-Mosaic/submodule/raylib-lite-engine"
    fi
}

# copy_notices BUILD_DIR DEST: the licences every package has to carry.
copy_notices() {
    local build="$1" dest="$2" engine
    engine="$(find_engine)"
    mkdir -p "$dest"
    cp "$GAME_ROOT/LICENSE" "$dest/LICENSE.txt"
    cp "$engine/LICENSE" "$dest/LICENSE-raylib-lite-engine.txt"
    cp "$build/_deps/sdl3-src/LICENSE.txt" "$dest/LICENSE-SDL3.txt"
}

# Steam options for cmake, from FROSTLINE_STEAM_APPID and STEAMWORKS_SDK.
steam_cmake_args() {
    if [[ -z "${STEAMWORKS_SDK:-}" || -z "${FROSTLINE_STEAM_APPID:-}" ]]; then
        echo "a Steam build needs STEAMWORKS_SDK and FROSTLINE_STEAM_APPID" >&2
        exit 1
    fi
    echo "-DFROSTLINE_STEAM=ON -DSTEAMWORKS_SDK=$STEAMWORKS_SDK -DFROSTLINE_STEAM_APPID=$FROSTLINE_STEAM_APPID"
}
