# Art pipeline

The game keeps the compact enemy, icon, spell, weapon-icon, and projectile artwork on a 16×16 canvas. Cosmetic artwork is now authored and baked on a dedicated 32×32 canvas:

- `tools/data/art.json` contains the SVG-like fragments.
- `tools/art_raster.py` rasterizes rectangles, circles, ellipses, lines, inherited group strokes, and paint order. `rects(svg, size=32)` is used for `HAT` and `FACE` entries.
- `tools/gen_data.py` emits `source/data_gen.cpp`; the normal Makefile build regenerates it automatically when the generator or JSON inputs change. Run `make regen` to force regeneration.
- `source/gfx.cpp` composes the knight in a 32×32 atlas cell. Every character/costume draw uses half the former scale, preserving the original 16×16 gameplay footprint (and prior menu-preview size), while each cosmetic pixel has independent space for highlights, outlines, and facial details.
- The shotgun, pistol, katana, and machine-gun entries in the `ICO` group use consistent silhouettes, outlines, material colors, and highlights.

When adding a new cosmetic, use coordinates from `0` through `31`. Keep geometry inside the 32×32 cell. For a hat-only outfit (`hat >= 5`), the art fragment is drawn as the complete 32×32 outfit; for hats `0` through `4`, the fragment is layered over the 32×32 knight body. The `none` entry (index 4) is deliberately empty/transparent so it does not paint over the face.

## Edit art in Aseprite

The optional `tools/aseprite_sync.py` tool converts the art fragments into native-size transparent PNGs, then converts edited pixels back into the SVG-like rectangles in `tools/data/art.json`.

```sh
python3 -m pip install pillow   # or: py -3 -m pip install pillow
make aseprite-export
```

Open and edit the PNGs under `tools/aseprite_work/` in Aseprite. Keep the canvas size unchanged: hats/faces are 32×32; enemies/icons are 16×16. Use fully opaque pixels or full transparency (no partial alpha). Save over the same PNG, or save a same-name `.aseprite`/`.ase` file. The importer rejects resized canvases and semi-transparent pixels instead of silently distorting game art.

```sh
make aseprite-import
```

Import backs up `art.json`, updates only assets whose pixels changed, and regenerates `source/data_gen.cpp`. Saving a native Aseprite project requires its command-line executable; pass `--aseprite PATH` to the script or set `ASEPRITE` to the executable path. Saving/exporting as PNG needs no Aseprite CLI. For example, in Windows/MSYS2:

```sh
ASEPRITE='C:/Program Files (x86)/Steam/steamapps/common/Aseprite/Aseprite.exe' \
  make aseprite-import
```

The exporter refuses to replace an existing work manifest by default. To start a fresh export in the same folder, first move/delete that work folder, or run the script with `export --overwrite` only when you are willing to replace its PNGs.

Run the regression checks with:

```sh
python3 tools/test_art_raster.py
python3 tools/test_aseprite_sync.py
python3 tools/gen_data.py
```

Use `py -3` instead of `python3` for the direct test/generator commands when that is your Windows Python 3 launcher. The Aseprite sync test requires Pillow; the normal game build does not.

To inspect the art visually, generate a labeled contact sheet and individual
PNG files with:

```sh
python3 tools/preview_art.py
```

The default output is `tools/preview/`, and each standalone PNG is 128×128
pixels by default. Use `--group HAT`, `--group FACE`, `--name arcwarden`,
`--scale 12`, `--size 256`, or `--out /path/to/previews` to narrow or
customize the output. The preview uses the same rasterizer and canvas sizes as
the game compiler.

The 3DS renderer uses exact subtexture texel boundaries with nearest-neighbor filtering; do not inset glyph UVs by half a texel, as that crops edge pixels from the tiny font.

The C++ atlas allocator now validates every allocation and makes `gfx::init`
fail cleanly if a future art expansion exceeds the 256×256 atlas, instead of
silently returning invalid sprite coordinates.

## Azahar startup log

The 3DS build appends startup diagnostics to:

```text
sdmc:/3ds/AbyssalEclipse/boot.log
```

On Windows, open the SD-card folder used by Azahar, then open
`3ds/AbyssalEclipse/boot.log`. Copy the complete file after a failed launch.
It records platform graphics initialization, audio initialization, atlas
texture allocation/upload, and the game initialization stage where startup
stopped. The file is deliberately append-only so repeated launches preserve
the previous failure history.

## Hidden debug unlock

SELECT no longer opens the debug panel by itself. During a run, hold **L + R + X + Y** and press SELECT once to arm the debug unlock; SELECT can then toggle the panel normally. The unlock is held in the exported volatile symbol `g_debugUnlock`. A Luma cheat can write the magic value `0x0AE13D3B` to that RAM word, then pressing SELECT will open the debug panel without using the controller combo.
