#!/usr/bin/env bash
# Installs CV Fuzz for the current user.
#   VST3        -> ~/.vst3/CV Fuzz.vst3
#   Standalone  -> ~/.local/bin/cv-fuzz
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p "$HOME/.vst3" "$HOME/.local/bin"
rm -rf "$HOME/.vst3/CV Fuzz.vst3"
cp -R "CV Fuzz.vst3" "$HOME/.vst3/"
install -m 755 "CV Fuzz" "$HOME/.local/bin/cv-fuzz"
echo "Installed CV Fuzz."
echo "  VST3:       $HOME/.vst3/CV Fuzz.vst3  (rescan plugins in your DAW)"
echo "  Standalone: run 'cv-fuzz'"
if ! ldconfig -p 2>/dev/null | grep -q libwebkit2gtk-4.1; then
  echo
  echo "The interface needs WebKitGTK. On Ubuntu or Debian:"
  echo "  sudo apt install libwebkit2gtk-4.1-0 libgtk-3-0"
fi
