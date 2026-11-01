#!/usr/bin/env bash
# Builds the universal (Apple silicon + Intel) macOS app and signs it.
#   platform/pc/scripts/package_mac.sh              # itch.io, GitHub: Developer ID, notarized
#   platform/pc/scripts/package_mac.sh --steam      # Steam (see steam_cmake_args)
#   platform/pc/scripts/package_mac.sh --mas        # Mac App Store: a signed .pkg
#   platform/pc/scripts/package_mac.sh --unsigned   # local test, ad-hoc signature
#
# Developer ID builds take a "Developer ID Application" certificate in the
# keychain (FROSTLINE_SIGN_IDENTITY picks one when there are several), and a
# notarytool profile, stored once with
#   xcrun notarytool store-credentials frostline --apple-id ... --team-id ...
# and named by FROSTLINE_NOTARY_PROFILE (default: frostline). The script
# signs, notarizes, staples and zips.
#
# The Mac App Store build takes an "Apple Distribution" and a "Mac Installer
# Distribution" certificate (FROSTLINE_SIGN_IDENTITY and
# FROSTLINE_INSTALLER_IDENTITY pick them), and the app's Mac App Store
# provisioning profile at FROSTLINE_MAS_PROFILE (default
# ~/.frostline/FrostlineHold_MAS.provisionprofile). It is sandboxed
# (mac/FrostlineHold.entitlements), leaves Steam out and comes out as a .pkg
# for Transporter. Every upload needs a larger CFBundleVersion:
# FROSTLINE_BUILD_NUMBER sets it, by default the UTC time as YYDDDHHMM.
source "$(dirname "$0")/common.sh"

FLAVOUR=itch
SIGNED=1
EXTRA=()
for arg in "$@"; do
    case "$arg" in
        --steam) FLAVOUR=steam; read -r -a EXTRA <<< "$(steam_cmake_args)" ;;
        --mas) FLAVOUR=mas ;;
        --unsigned) SIGNED=0 ;;
        *) echo "unknown option $arg" >&2; exit 2 ;;
    esac
done
if [[ "$FLAVOUR" == mas && "$SIGNED" == 0 ]]; then
    echo "--mas makes a store upload; it cannot be --unsigned" >&2
    exit 2
fi

# find_identity PATTERN [POLICY]: the first valid identity whose name matches.
find_identity() {
    security find-identity -v ${2:+-p "$2"} |
        sed -nE "s/.*\"(($1): [^\"]*)\".*/\1/p" | head -1
}

need_identity() {
    if [[ -z "$1" ]]; then
        cat >&2 <<EOF
No "$2" certificate in the keychain. Create one in Xcode > Settings >
Accounts > Manage Certificates (the Account Holder of the team can).
EOF
        [[ "$FLAVOUR" == mas ]] || echo "For a local test build, pass --unsigned." >&2
        exit 1
    fi
}

IDENTITY="${FROSTLINE_SIGN_IDENTITY:-}"
INSTALLER="${FROSTLINE_INSTALLER_IDENTITY:-}"
PROFILE="${FROSTLINE_MAS_PROFILE:-$HOME/.frostline/FrostlineHold_MAS.provisionprofile}"
if [[ "$FLAVOUR" == mas ]]; then
    [[ -n "$IDENTITY" ]] || IDENTITY="$(find_identity "Apple Distribution|3rd Party Mac Developer Application" codesigning)"
    need_identity "$IDENTITY" "Apple Distribution"
    # The installer certificate signs packages, not code: no codesigning policy.
    [[ -n "$INSTALLER" ]] || INSTALLER="$(find_identity "3rd Party Mac Developer Installer|Mac Installer Distribution")"
    need_identity "$INSTALLER" "Mac Installer Distribution"
    if [[ ! -f "$PROFILE" ]]; then
        cat >&2 <<EOF
No provisioning profile at $PROFILE. Create a "Mac App Store Connect"
profile for the app's identifier at developer.apple.com (Certificates,
Identifiers & Profiles > Profiles), download it there, or set
FROSTLINE_MAS_PROFILE.
EOF
        exit 1
    fi
    EXTRA+=("-DFROSTLINE_BUILD_NUMBER=${FROSTLINE_BUILD_NUMBER:-$(date -u +%y%j%H%M)}")
elif [[ "$SIGNED" == 1 && -z "$IDENTITY" ]]; then
    IDENTITY="$(find_identity "Developer ID Application" codesigning)"
    need_identity "$IDENTITY" "Developer ID Application"
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

if [[ "$FLAVOUR" == mas ]]; then
    # The sandbox entitlements plus the identifiers the profile grants, which
    # must name this bundle exactly (the store takes no wildcard profile).
    PLIST="$BUILD/profile.plist"
    ENTITLEMENTS="$BUILD/mas.entitlements"
    security cms -D -i "$PROFILE" > "$PLIST"
    APP_ID="$(/usr/libexec/PlistBuddy -c "Print :Entitlements:com.apple.application-identifier" "$PLIST")"
    TEAM_ID="$(/usr/libexec/PlistBuddy -c "Print :Entitlements:com.apple.developer.team-identifier" "$PLIST")"
    BUNDLE_ID="$(/usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "$APP/Contents/Info.plist")"
    if [[ "$APP_ID" != "$TEAM_ID.$BUNDLE_ID" ]]; then
        echo "$PROFILE is for $APP_ID, not $TEAM_ID.$BUNDLE_ID" >&2
        exit 1
    fi
    cp "$PC_DIR/mac/FrostlineHold.entitlements" "$ENTITLEMENTS"
    /usr/libexec/PlistBuddy -c "Add :com.apple.application-identifier string $APP_ID" \
        -c "Add :com.apple.developer.team-identifier string $TEAM_ID" "$ENTITLEMENTS"
    cp "$PROFILE" "$APP/Contents/embedded.provisionprofile"
    codesign --force --timestamp --options runtime --entitlements "$ENTITLEMENTS" \
        --sign "$IDENTITY" "$APP"
    codesign --verify --strict --deep --verbose=2 "$APP"

    BUILD_NUMBER="$(/usr/libexec/PlistBuddy -c "Print :CFBundleVersion" "$APP/Contents/Info.plist")"
    PKG="$OUT/frostline-hold-$VERSION-$BUILD_NUMBER-mas.pkg"
    productbuild --component "$APP" /Applications --sign "$INSTALLER" "$PKG"
    pkgutil --check-signature "$PKG"
    echo "mac (mas): $PKG ($(du -h "$PKG" | cut -f1)); upload it with Transporter"
    exit 0
fi

if [[ "$SIGNED" == 0 ]]; then
    codesign --force --deep --sign - "$APP"
else
    # Inside out: the Steam library first, then the app around it.
    find "$APP/Contents/MacOS" -name '*.dylib' -print0 |
        xargs -0 -I{} codesign --force --timestamp --options runtime --sign "$IDENTITY" {}
    codesign --force --timestamp --options runtime --sign "$IDENTITY" "$APP"
    codesign --verify --strict --deep --verbose=2 "$APP"

    NOTARY_PROFILE="${FROSTLINE_NOTARY_PROFILE:-frostline}"
    SUBMISSION="$OUT/notarize.zip"
    ditto -c -k --keepParent "$APP" "$SUBMISSION"
    xcrun notarytool submit "$SUBMISSION" --keychain-profile "$NOTARY_PROFILE" --wait
    rm -f "$SUBMISSION"
    xcrun stapler staple "$APP"
    spctl --assess --type execute --verbose=2 "$APP"
fi

ZIP="$OUT/frostline-hold-$VERSION-mac-$FLAVOUR.zip"
ditto -c -k --keepParent "$APP" "$ZIP"
echo "mac ($FLAVOUR): $ZIP ($(du -h "$ZIP" | cut -f1))"
