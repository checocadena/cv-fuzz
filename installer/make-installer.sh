#!/usr/bin/env bash
# Builds "CV Fuzz <version>.pkg" from the compiled plugins.
# Usage: installer/make-installer.sh [path/to/CVFuzz_artefacts/Release]
# Optional signing (needs an Apple Developer ID):
#   export DEV_ID_APP="Developer ID Application: Your Name (TEAMID)"
#   export DEV_ID_INSTALLER="Developer ID Installer: Your Name (TEAMID)"
# Optional notarization (after: xcrun notarytool store-credentials <profile> ...):
#   export NOTARY_PROFILE="cvfuzz-notary"
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
ART="${1:-$ROOT/build/CVFuzz_artefacts/Release}"
VERSION="$(grep -m1 -Eo 'project\(CVFuzz VERSION [0-9.]+' "$ROOT/CMakeLists.txt" | awk '{print $3}')"
STAGE="$ROOT/build/installer"
OUT="$ROOT/dist"
rm -rf "$STAGE" && mkdir -p "$STAGE"/{au,vst3,app,pkgs} "$OUT"

mkdir -p "$STAGE/au/Library/Audio/Plug-Ins/Components" "$STAGE/vst3/Library/Audio/Plug-Ins/VST3" "$STAGE/app/Applications"
cp -R "$ART/AU/CV Fuzz.component"  "$STAGE/au/Library/Audio/Plug-Ins/Components/"
cp -R "$ART/VST3/CV Fuzz.vst3"     "$STAGE/vst3/Library/Audio/Plug-Ins/VST3/"
cp -R "$ART/Standalone/CV Fuzz.app" "$STAGE/app/Applications/"

# Sign the binaries: Developer ID if available, otherwise ad-hoc so they load on Apple Silicon.
SIGN_ID="${DEV_ID_APP:--}"
for b in "$STAGE/au/Library/Audio/Plug-Ins/Components/CV Fuzz.component" \
         "$STAGE/vst3/Library/Audio/Plug-Ins/VST3/CV Fuzz.vst3" \
         "$STAGE/app/Applications/CV Fuzz.app"; do
  ENT=()
  case "$b" in *.app) ENT=(--entitlements "$HERE/app.entitlements") ;; esac
  if [ "$SIGN_ID" = "-" ]; then
    codesign --force --deep ${ENT[@]+"${ENT[@]}"} --sign - "$b"
  else
    codesign --force --deep --options runtime --timestamp ${ENT[@]+"${ENT[@]}"} --sign "$SIGN_ID" "$b"
  fi
done

pkgbuild --root "$STAGE/au"   --identifier com.checocadena.cvfuzz.au   --version "$VERSION" --install-location / --scripts "$HERE/scripts" "$STAGE/pkgs/au.pkg"
pkgbuild --root "$STAGE/vst3" --identifier com.checocadena.cvfuzz.vst3 --version "$VERSION" --install-location / "$STAGE/pkgs/vst3.pkg"
pkgbuild --root "$STAGE/app"  --identifier com.checocadena.cvfuzz.app  --version "$VERSION" --install-location / "$STAGE/pkgs/app.pkg"

PKG="$OUT/CV Fuzz $VERSION.pkg"
SIGN_ARGS=()
if [ -n "${DEV_ID_INSTALLER:-}" ]; then SIGN_ARGS=(--sign "$DEV_ID_INSTALLER"); fi
productbuild --distribution "$HERE/distribution.xml" --resources "$HERE/resources" \
             --package-path "$STAGE/pkgs" ${SIGN_ARGS[@]+"${SIGN_ARGS[@]}"} "$PKG"

echo "Built: $PKG"

if [ -n "${NOTARY_PROFILE:-}" ]; then
  if [ -z "${DEV_ID_INSTALLER:-}" ]; then echo "Notarization needs DEV_ID_INSTALLER to be set."; exit 1; fi
  echo "==> Submitting to Apple for notarization (usually a few minutes)"
  xcrun notarytool submit "$PKG" --keychain-profile "$NOTARY_PROFILE" --wait
  echo "==> Stapling the approval to the installer"
  xcrun stapler staple "$PKG"
  echo "==> Checking Gatekeeper accepts it"
  spctl --assess --type install -vv "$PKG"
  echo "Ready to send: $PKG"
fi

# Stable name for the website's download link (copied after stapling, so it keeps the approval).
cp "$PKG" "$OUT/CV-Fuzz-macOS.pkg"
echo "Upload this to the GitHub release: $OUT/CV-Fuzz-macOS.pkg"
