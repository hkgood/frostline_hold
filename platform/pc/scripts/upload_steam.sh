#!/usr/bin/env bash
# Uploads the Steam packages (package_windows.sh --steam, package_mac.sh
# --steam) with steamcmd. The build lands on no branch unless
# FROSTLINE_STEAM_BRANCH names one; set it live from the Steamworks site.
#
#   FROSTLINE_STEAM_APPID=...      the App ID
#   FROSTLINE_STEAM_DEPOT_WIN=...  the Windows depot (usually App ID + 1)
#   FROSTLINE_STEAM_DEPOT_MAC=...  the macOS depot
#   STEAM_BUILD_USER=...           a Steam account with upload rights
#   platform/pc/scripts/upload_steam.sh "build description"
source "$(dirname "$0")/common.sh"

: "${FROSTLINE_STEAM_APPID:?}" "${FROSTLINE_STEAM_DEPOT_WIN:?}" "${FROSTLINE_STEAM_DEPOT_MAC:?}"
: "${STEAM_BUILD_USER:?}"
DESCRIPTION="${1:-Frostline Hold $VERSION}"
WIN_ROOT="$DIST_DIR/windows-steam/Frostline Hold"
MAC_ROOT="$DIST_DIR/mac-steam"
for root in "$WIN_ROOT" "$MAC_ROOT/Frostline Hold.app"; do
    [[ -e "$root" ]] || { echo "missing $root: run the --steam package scripts first" >&2; exit 1; }
done

VDF="$DIST_DIR/steam"
mkdir -p "$VDF/output"
cat > "$VDF/depot_windows.vdf" <<EOF
"DepotBuild"
{
    "DepotID" "$FROSTLINE_STEAM_DEPOT_WIN"
    "ContentRoot" "$WIN_ROOT"
    "FileMapping" { "LocalPath" "*" "DepotPath" "." "Recursive" "1" }
}
EOF
cat > "$VDF/depot_mac.vdf" <<EOF
"DepotBuild"
{
    "DepotID" "$FROSTLINE_STEAM_DEPOT_MAC"
    "ContentRoot" "$MAC_ROOT"
    "FileMapping" { "LocalPath" "Frostline Hold.app/*" "DepotPath" "Frostline Hold.app" "Recursive" "1" }
}
EOF
cat > "$VDF/app_build.vdf" <<EOF
"AppBuild"
{
    "AppID" "$FROSTLINE_STEAM_APPID"
    "Desc" "$DESCRIPTION"
    "SetLive" "${FROSTLINE_STEAM_BRANCH:-}"
    "BuildOutput" "$VDF/output"
    "Depots"
    {
        "$FROSTLINE_STEAM_DEPOT_WIN" "$VDF/depot_windows.vdf"
        "$FROSTLINE_STEAM_DEPOT_MAC" "$VDF/depot_mac.vdf"
    }
}
EOF

steamcmd +login "$STEAM_BUILD_USER" +run_app_build "$VDF/app_build.vdf" +quit
