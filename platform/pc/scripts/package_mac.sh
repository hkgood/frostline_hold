#!/usr/bin/env bash
# Builds the universal (Apple silicon + Intel) macOS app, signs it with a
# Developer ID, notarizes it, staples the ticket and zips it.
#   platform/pc/scripts/package_mac.sh              # itch.io
#   platform/pc/scripts/package_mac.sh --steam      # Steam (see steam_cmake_args)
#   platform/pc/scripts/package_mac.sh --unsigned   # local test, ad-hoc signature
#
# Signing takes a "Developer ID Application" certificate in the keychain
# (FROSTLINE_SIGN_IDENTITY picks one when there are several). Notarizing
# takes a notarytool profile, stored once with
#   xcrun notarytool store-credentials frostline --apple-id ... --team-id ...
# and named by FROSTLINE_NOTARY_PROFILE (default: frostline).
source "$(dirname "$0")/common.sh"

FLAVOUR=itch
SIGNED=1
EXTRA=()
for arg in "$@"; do
    case "$arg" in
        --steam) FLAVOUR=steam; read -r -a EXTRA <<< "$(steam_cmake_args)" ;;
        --unsigned) SIGNED=0 ;;
        *) echo "unknown option $arg" >&2; exit 2 ;;
    esac
done

IDENTITY="${FROSTLINE_SIGN_IDENTITY:-}"
if [[ "$SIGNED" == 1 && -z "$IDENTITY" ]]; then
    IDENTITY="$(security find-identity -v -p codesigning |
        sed -n 's/.*"\(Developer ID Application: [^"]*\)".*/\1/p' | head -1)"
    if [[ -z "$IDENTITY" ]]; then
        cat >&2 <<'EOF'
No "Developer ID Application" certificate in the keychain. Create one in
Xcode > Settings > Accounts > Manage Certificates (the Account Holder of the
team can), or pass --unsigned for a local test build.
EOF
        exit 1
    fi
fi

BUILD="$GAME_ROOT/build-pc/mac-$FLAVOUR"
cmake -S "$PC_DIR" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    ${EXTRA[@]+"${EXTRA[@]}"}
cmake --build "$BUILD" -j

OUT="$DIST_DIR/mac-$FLAVOUR"
APP="$OUT/Frostline Hold.app"
rm -rf "$OUT"
mkdir -p "$OUT"
ditto "$BUILD/Frostline Hold.app" "$APP"
# The development App ID file must not ship: Steam provides the ID.
rm -f "$APP/Contents/MacOS/steam_appid.txt"
copy_notices "$BUILD" "$APP/Contents/Resources/licenses"

if [[ "$SIGNED" == 0 ]]; then
    codesign --force --deep --sign - "$APP"
else
    # Inside out: the Steam library first, then the app around it.
    find "$APP/Contents/MacOS" -name '*.dylib' -print0 |
        xargs -0 -I{} codesign --force --timestamp --options runtime --sign "$IDENTITY" {}
    codesign --force --timestamp --options runtime --sign "$IDENTITY" "$APP"
    codesign --verify --strict --deep --verbose=2 "$APP"

    PROFILE="${FROSTLINE_NOTARY_PROFILE:-frostline}"
    SUBMISSION="$OUT/notarize.zip"
    ditto -c -k --keepParent "$APP" "$SUBMISSION"
    xcrun notarytool submit "$SUBMISSION" --keychain-profile "$PROFILE" --wait
    rm -f "$SUBMISSION"
    xcrun stapler staple "$APP"
    spctl --assess --type execute --verbose=2 "$APP"
fi

ZIP="$OUT/frostline-hold-$VERSION-mac-$FLAVOUR.zip"
ditto -c -k --keepParent "$APP" "$ZIP"
echo "mac ($FLAVOUR): $ZIP ($(du -h "$ZIP" | cut -f1))"
