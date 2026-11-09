#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/build/_deps/juce-src/modules/juce_audio_plugin_client/juce_audio_plugin_client_LV2.cpp"
PATCH="$ROOT/tools/pipedal-juce-lv2-boundedBlockLength.patch"
if [[ ! -f "$SRC" ]]; then
  echo "JUCE source not found at $SRC (configure cmake first)" >&2
  exit 1
fi
if [[ ! -f "$PATCH" ]]; then
  echo "Missing $PATCH" >&2
  exit 1
fi
patch -p1 -d "$ROOT/build/_deps/juce-src" < "$PATCH"
echo "Applied PipeDal LV2 patch to $SRC"
