# NeoDelay

Touch-friendly guitar **chorus + dual delay + ethereal** for Linux — **LV2** and **VST3**, built with **JUCE 8**.

By **Andreas Neophitou**.

## Features

- **Chorus** with Rate / Depth
- **Delay 1** and **Delay 2** in series — each with On / Time / Feedback / Reverse
- **Ethereal** (shimmer wash) with On / Shimmer amount / Mix
- **Demo** tone for Standalone testing without an interface

Signal path: Chorus → Delay 1 → Delay 2 → Ethereal → dry/wet Mix

## Requirements

- Linux (tested on Raspberry Pi OS / aarch64 and Ubuntu)
- CMake ≥ 3.22
- C++17 compiler (`g++` or `clang++`)
- Ninja (recommended)
- Dev packages typically needed:

```bash
sudo apt install build-essential cmake ninja-build pkg-config \
  libasound2-dev libfreetype-dev libx11-dev libxcomposite-dev \
  libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev \
  libxrender-dev libgl1-mesa-dev libcurl4-openssl-dev \
  libwebkit2gtk-4.1-dev lv2-dev
```

JUCE is fetched automatically via CMake `FetchContent` (no manual install).

## Build

```bash
cd ~/neodelay
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
# On low-memory boards, build juceaide single-threaded first if configure OOMs:
#   ninja -C build/_deps/juce-build/tools -j1 juceaide
cmake --build build -j1
```

Artifacts land under `build/NeoDelay_artefacts/Release/` and are also copied into `Dist/`:

| Format | Dist path |
|--------|-----------|
| LV2 | `Dist/LV2/NeoDelay.lv2` |
| VST3 | `Dist/VST3/NeoDelay.vst3` |

### PipeDal (Raspberry Pi)

PipeDal does not advertise `bufs:boundedBlockLength`. After CMake configures JUCE, apply the included patch before building LV2:

```bash
./tools/apply-pipedal-juce-lv2-patch.sh
cmake --build build --target NeoDelay_LV2 -j1
sudo cp -a Dist/LV2/NeoDelay.lv2 /usr/local/lib/lv2/
sudo systemctl restart pipedald
```

LV2 URI: `https://aneophitou.audio/plugins/neodelay`

## License

All rights reserved unless otherwise noted.
