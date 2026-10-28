#!/usr/bin/env python3
"""Render tools/data/art.json into inspectable PNG files.

Examples:
  python3 tools/preview_art.py
  python3 tools/preview_art.py --group HAT --scale 12
  python3 tools/preview_art.py --group FACE --name glasses --scale 20

The preview uses the same art_raster.py implementation as the game data
compiler, so it shows the actual opaque pixel runs that will be baked into the
3DS atlas. Cosmetic HAT/FACE entries are rendered at 32x32; legacy art is
rendered at 16x16.
"""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError as exc:  # pragma: no cover
    Image = ImageDraw = ImageFont = None
    PIL_IMPORT_ERROR = exc
else:
    PIL_IMPORT_ERROR = None

from art_raster import rects

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_JSON = ROOT / "tools" / "data" / "art.json"
DEFAULT_OUT = ROOT / "tools" / "preview"
GROUPS = ("ENEMY", "HAT", "FACE", "ICO")
LABEL_H = 26
PAD = 8
BG = (20, 18, 30, 255)
CELL_BG = (43, 38, 58, 255)
GRID = (82, 72, 104, 255)
TEXT = (240, 234, 214, 255)


def safe_name(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", value).strip("_") or "art"


def canvas_for(group: str) -> int:
    return 32 if group in ("HAT", "FACE") else 16


def raster_image(svg: str, size: int, scale: int) -> Image.Image:
    image = Image.new("RGBA", (size * scale, size * scale), (0, 0, 0, 0))
    px = image.load()
    for x, y, w, h, color in rects(svg, size=size):
        rgb = ((color >> 16) & 255, (color >> 8) & 255, color & 255, 255)
        for yy in range(y, min(size, y + h)):
            for xx in range(x, min(size, x + w)):
                for sy in range(scale):
                    for sx in range(scale):
                        px[xx * scale + sx, yy * scale + sy] = rgb
    return image


def font():
    try:
        return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12)
    except OSError:
        return ImageFont.load_default()


def draw_cell(sheet: Image.Image, x: int, y: int, name: str, art: Image.Image, size: int, scale: int, fnt):
    draw = ImageDraw.Draw(sheet)
    cell_w = size * scale + PAD * 2
    cell_h = size * scale + LABEL_H + PAD * 2
    draw.rectangle((x, y, x + cell_w - 1, y + cell_h - 1), fill=CELL_BG, outline=GRID)
    # A checker-like neutral backing makes transparent pixels visible without
    # changing the exported standalone PNGs.
    for yy in range(size * scale):
        for xx in range(size * scale):
            if (xx // scale + yy // scale) & 1:
                draw.point((x + PAD + xx, y + PAD + yy), fill=(52, 46, 68, 255))
    sheet.alpha_composite(art, (x + PAD, y + PAD))
    label = name[:28]
    draw.text((x + PAD, y + PAD + size * scale + 4), label, fill=TEXT, font=fnt)


def render_group(group: str, entries: dict, out: Path, scale: int, columns: int):
    size = canvas_for(group)
    cell_w = size * scale + PAD * 2
    cell_h = size * scale + LABEL_H + PAD * 2
    names = list(entries)
    rows = (len(names) + columns - 1) // columns
    sheet = Image.new("RGBA", (columns * cell_w + PAD, rows * cell_h + PAD), BG)
    fnt = font()
    for i, name in enumerate(names):
        art = raster_image(entries[name], size, scale)
        draw_cell(sheet, PAD + (i % columns) * cell_w, PAD + (i // columns) * cell_h,
                  name, art, size, scale, fnt)
        art.save(out / f"{group.lower()}_{safe_name(name)}.png")
    sheet.save(out / f"{group.lower()}_sheet.png")


def main():
    parser = argparse.ArgumentParser(description=__doc__, add_help=False)
    parser.add_argument("-h", "--h", "-help", "--help", action="help", help="show this help message and exit")
    parser.add_argument("--json", type=Path, default=DEFAULT_JSON, help="art JSON path")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help="output directory")
    parser.add_argument("--group", choices=GROUPS, help="render one group instead of all groups")
    parser.add_argument("--name", help="render only this entry; requires --group")
    parser.add_argument("--scale", type=int, help="nearest-neighbor pixels per art pixel; overrides --size")
    parser.add_argument("--size", type=int, default=128, help="default exported art size in pixels (default: 128)")
    parser.add_argument("--columns", type=int, default=4, help="contact-sheet columns (default: 4)")
    args = parser.parse_args()
    if Image is None:
        raise SystemExit("This tool requires Pillow. Install with `python3 -m pip install pillow` (or `py -3 -m pip install pillow`).") from PIL_IMPORT_ERROR
    if args.columns < 1 or args.size < 1 or (args.scale is not None and args.scale < 1):
        parser.error("--scale, --size, and --columns must be positive")
    if args.name and not args.group:
        parser.error("--name requires --group")

    data = json.loads(args.json.read_text(encoding="utf-8"))
    groups = [args.group] if args.group else [g for g in GROUPS if g in data]
    args.out.mkdir(parents=True, exist_ok=True)
    for group in groups:
        entries = data[group]
        if args.name:
            if args.name not in entries:
                parser.error(f"unknown {group} art entry: {args.name}")
            entries = {args.name: entries[args.name]}
        size = canvas_for(group)
        scale = args.scale if args.scale is not None else max(1, args.size // size)
        if args.scale is None and scale * size != args.size:
            parser.error(f"--size must be divisible by {size} for {group}; use --scale to override")
        render_group(group, entries, args.out, scale, args.columns)
        print(f"{group}: {len(entries)} entries -> {args.out}")


if __name__ == "__main__":
    main()
