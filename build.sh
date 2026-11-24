#!/usr/bin/env bash
# Builds Psaltikon as an Audio Unit, a VST3 and a standalone app, installs the
# plug-ins, and validates the Audio Unit for Logic.
# Requires: Xcode + command line tools, CMake (brew install cmake), git.
set -euo pipefail
cd "$(dirname "$0")"

JUCE_PATH="${JUCE_PATH:-$HOME/JUCE}"
if [ ! -f "$JUCE_PATH/CMakeLists.txt" ]; then
  echo "JUCE not found at $JUCE_PATH -- cloning JUCE 8..."
  git clone --depth 1 --branch 8.0.6 https://github.com/juce-framework/JUCE.git "$JUCE_PATH"
fi

cmake -B build -S . -DJUCE_PATH="$JUCE_PATH" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Psaltikon_AU Psaltikon_VST3 Psaltikon_Standalone -j

echo
echo "Installed:"
echo "  AU:   ~/Library/Audio/Plug-Ins/Components/Psaltikon.component"
echo "  VST3: ~/Library/Audio/Plug-Ins/VST3/Psaltikon.vst3"
echo "  App:  build/Psaltikon_artefacts/Release/Standalone/Psaltikon.app"
echo "Validating with auval (this is the check Logic does)..."
killall -9 AudioComponentRegistrar 2>/dev/null || true
auval -v aumu Psk1 Pslt
echo
echo "Done. Restart your DAW; in Logic, Settings > Plug-in Manager > Reset & Rescan Selection if needed."
