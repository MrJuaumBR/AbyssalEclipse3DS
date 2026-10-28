# Building Abyssal Eclipse 3DS

This guide covers the `.3dsx` Homebrew build and the optional `.cia` installable package.

## Prerequisites

1. Install devkitPro and the 3DS development packages from the devkitPro MSYS2 terminal:
   ```bash
   pacman -S 3ds-dev
   ```
2. Ensure `DEVKITPRO` and `DEVKITARM` are set (the devkitPro terminal normally does this):
   ```bash
   export DEVKITPRO=/opt/devkitpro
   export DEVKITARM=$DEVKITPRO/devkitARM
   ```
3. Install Python 3. The Makefile automatically selects `python3`, `python`, or the Windows `py -3` launcher. If auto-detection does not find your Python installation, specify it explicitly, for example `make PYTHON='py -3'`.

## Build the `.3dsx`

From the project root:

```bash
make clean && make
```

The normal build checks `tools/gen_data.py`, `tools/art_raster.py`, `tools/data/mock_data.json`, and `tools/data/art.json`; if any input is newer, it regenerates `source/data_gen.cpp` before compiling. To force regeneration at any time, run:

```bash
make regen
```

The output is `AbyssalEclipse.3dsx` in the project root. Copy it to `sdmc:/3ds/` and launch it with the Homebrew Launcher.

## Build the `.cia`

A CIA build additionally requires `bannertool.exe`, `makerom.exe`, `icon.png`, `banner.png`, `silence.wav`, and `AbyssalEclipse.rsf` in the project root. Then run:

```bash
make cia
```

This compiles the ELF, creates the SMDH and banner with bannertool, and packages `AbyssalEclipse.cia` with makerom. The CIA can be installed on a modded 3DS with FBI.

## Clean build outputs

```bash
make clean
```
