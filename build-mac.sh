#!/usr/bin/env bash
# Build CV Fuzz on macOS and install it for Ableton Live.
# Usage: ./build-mac.sh              build and install
#        ./build-mac.sh --installer  also produce dist/CV Fuzz <version>.pkg
set -euo pipefail
cd "$(dirname "$0")"

echo "==> Checking tools"
if ! xcode-select -p >/dev/null 2>&1; then
  echo "Xcode Command Line Tools are missing. Installing (re-run this script when it finishes)."
  xcode-select --install || true
  exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
  if command -v brew >/dev/null 2>&1; then
    brew install cmake
  else
    echo "CMake is missing. Install Homebrew from https://brew.sh, then run: brew install cmake"
    exit 1
  fi
fi

echo "==> Configuring (first run downloads JUCE, about a minute)"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

echo "==> Building AU, VST3 and Standalone"
cmake --build build --config Release --target CVFuzz_AU CVFuzz_VST3 CVFuzz_Standalone -j "$(sysctl -n hw.ncpu)"

ART="build/CVFuzz_artefacts/Release"
AU_DEST="$HOME/Library/Audio/Plug-Ins/Components"
VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$AU_DEST" "$VST3_DEST"

echo "==> Installing"
rm -rf "$AU_DEST/CV Fuzz.component" "$VST3_DEST/CV Fuzz.vst3"
cp -R "$ART/AU/CV Fuzz.component" "$AU_DEST/"
cp -R "$ART/VST3/CV Fuzz.vst3" "$VST3_DEST/"

# Ad-hoc signing so macOS will load the plugins on Apple Silicon.
codesign --force --deep --sign - "$AU_DEST/CV Fuzz.component"
codesign --force --deep --sign - "$VST3_DEST/CV Fuzz.vst3"

# Make macOS re-read the Audio Unit list.
killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true

echo "==> Validating the Audio Unit"
if auval -v aufx Cvf1 Chca | tail -3 | grep -q "AU VALIDATION SUCCEEDED"; then
  echo "AU validation passed."
else
  echo "AU validation did not pass. Run 'auval -v aufx Cvf1 Chca' to see details. The VST3 can still be used."
fi

if [ "${1:-}" = "--installer" ]; then
  echo "==> Building the installer package"
  bash "$(dirname "$0")/installer/make-installer.sh" "$ART"
fi

echo
echo "Installed:"
echo "  $AU_DEST/CV Fuzz.component"
echo "  $VST3_DEST/CV Fuzz.vst3"
echo "Standalone app: $ART/Standalone/CV Fuzz.app"
echo
echo "In Ableton: Settings > Plug-Ins > turn on Audio Units and VST3 system folders > Rescan."
