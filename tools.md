# Project tools

This page lists the maintained scripts under `tools/`. Every runnable Python tool accepts the standard help spellings `-h`, `--h`, `-help`, and `--help`. For Aseprite subcommands, put the flag after the subcommand, for example `python3 tools/aseprite_sync.py export --h`.

On Windows/MSYS2, use whichever Python 3 launcher is installed (`python3`, `python`, or `py -3`). The Makefile auto-detects those names; if needed, override it with `make PYTHON='py -3' <target>`.

## Data generation

| Tool | Purpose | Run |
|---|---|---|
| `tools/gen_data.py` | Generates `source/data_gen.cpp` and `source/strings_gen.h` from the source JSON. The normal build runs it when inputs change. | `python3 tools/gen_data.py` or `make regen` |
| `tools/gen_catalog.py` | Regenerates the human-readable `CATALOG.md`. | `python3 tools/gen_catalog.py` |

## Art authoring and preview

| Tool | Purpose | Run |
|---|---|---|
| `tools/aseprite_sync.py` | Exports art JSON entries to native-size PNGs and imports edited PNG/Aseprite files back into `tools/data/art.json`, with a backup and generated C++ refresh. | `make aseprite-export`, edit, then `make aseprite-import` |
| `tools/preview_art.py` | Produces native-art PNGs and labeled contact sheets for visual review. Defaults to `tools/preview/`. | `python3 tools/preview_art.py`; use `--group`, `--name`, `--size`/`--scale`, `--columns`, and `--out` to customize |
| `tools/art_raster.py` | Shared rasterizer library used by the generator, previewer, tests and Aseprite bridge. Running it directly prints its help; it is not a separate renderer command. | `python3 tools/art_raster.py --h` |

The preview and Aseprite tools need Pillow. Install it with `python3 -m pip install pillow` or `py -3 -m pip install pillow`. Importing a same-name `.aseprite`/`.ase` project also needs the Aseprite CLI; set `ASEPRITE` to its executable path. PNG import needs no Aseprite CLI.

`make aseprite-export` creates `tools/aseprite_work/` and refuses to replace an existing manifest unless explicitly overridden through the script. `make aseprite-import` reads that manifest, accepts only the original canvas size (32×32 hats/faces, 16×16 enemies/icons) and fully opaque or fully transparent pixels, backs up `art.json`, and regenerates `source/data_gen.cpp` when pixels changed.

To deliberately replace an old workspace, use `python3 tools/aseprite_sync.py export --overwrite` (or `py -3` instead of `python3`); this replaces the PNGs, so do not use it while unimported edits are in progress.

## Regression tests

| Tool | Purpose | Run |
|---|---|---|
| `tools/test_art_raster.py` | Checks representative rasterization, paint order, inherited strokes, and empty no-hat art. | `python3 tools/test_art_raster.py` |
| `tools/test_aseprite_sync.py` | Tests PNG round-tripping and the Aseprite CLI handoff with a test stub. | `python3 tools/test_aseprite_sync.py` |

The Aseprite round-trip test requires Pillow. Tests accept the same help aliases as the other scripts.