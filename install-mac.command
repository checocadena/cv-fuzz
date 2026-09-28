#!/usr/bin/env bash
# Double-click to install CV Fuzz (the AU and VST3 next to this file) for Ableton Live.
cd "$(dirname "$0")"
AU_DEST="$HOME/Library/Audio/Plug-Ins/Components"
VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$AU_DEST" "$VST3_DEST"
rm -rf "$AU_DEST/CV Fuzz.component" "$VST3_DEST/CV Fuzz.vst3"
cp -R "CV Fuzz.component" "$AU_DEST/"
cp -R "CV Fuzz.vst3" "$VST3_DEST/"
# Downloaded files are quarantined by macOS; clear that and ad-hoc sign so they load.
xattr -dr com.apple.quarantine "$AU_DEST/CV Fuzz.component" "$VST3_DEST/CV Fuzz.vst3" 2>/dev/null
codesign --force --deep --sign - "$AU_DEST/CV Fuzz.component"
codesign --force --deep --sign - "$VST3_DEST/CV Fuzz.vst3"
killall -9 AudioComponentRegistrar >/dev/null 2>&1
echo
echo "CV Fuzz installed. In Ableton: Settings > Plug-Ins > Rescan."
read -n 1 -s -r -p "Press any key to close."
