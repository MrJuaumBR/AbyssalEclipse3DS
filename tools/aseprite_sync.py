#!/usr/bin/env python3
"""Round-trip the game's pixel art through Aseprite-compatible PNGs.

Export native-size PNGs from tools/data/art.json, edit them in Aseprite, save
with the same name as PNG or Aseprite project, then import them back into the
JSON fragments consumed by tools/gen_data.py.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:  # pragma: no cover - depends on optional authoring tool
    Image = None
    PIL_IMPORT_ERROR = exc
else:
    PIL_IMPORT_ERROR = None

from art_raster import rects

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_JSON = ROOT / "tools" / "data" / "art.json"
DEFAULT_WORK = ROOT / "tools" / "aseprite_work"
GROUPS = ("ENEMY", "HAT", "FACE", "ICO")
MANIFEST_NAME = "aseprite_manifest.json"
FORMAT_VERSION = 1


def canvas_for(group: str) -> int:
    return 32 if group in ("HAT", "FACE") else 16


def safe_name(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", value).strip("_") or "art"


def load_data(path: Path) -> dict:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise SystemExit(f"Cannot read art JSON {path}: {exc}") from exc
    for group in GROUPS:
        if group not in data or not isinstance(data[group], dict):
            raise SystemExit(f"Art JSON is missing the {group} group")
    return data


def render_fragment(fragment: str, size: int) -> Image.Image:
    """Render the project's opaque pixel-runs into a transparent RGBA image."""
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    pixels = image.load()
    for x, y, w, h, color in rects(fragment, size=size):
        rgba = ((color >> 16) & 255, (color >> 8) & 255, color & 255, 255)
        for yy in range(y, min(size, y + h)):
            for xx in range(x, min(size, x + w)):
                pixels[xx, yy] = rgba
    return image


def png_to_fragment(image: Image.Image) -> str:
    """Encode an opaque/transparency pixel image as disjoint SVG-like rects."""
    rgba = image.convert("RGBA")
    px = rgba.load()
    width, height = rgba.size
    rectangles = []
    previous = {}
    for y in range(height):
        current = {}
        x = 0
        while x < width:
            r, g, b, a = px[x, y]
            if a not in (0, 255):
                raise ValueError(f"semi-transparent pixel at ({x},{y}); use fully opaque colors or full transparency")
            if a == 0:
                x += 1
                continue
            color = (r, g, b)
            x0 = x
            x += 1
            while x < width and px[x, y] == (r, g, b, 255):
                x += 1
            key = (x0, x - x0, color)
            rect = previous.get(key)
            if rect is not None and rect["y"] + rect["height"] == y:
                rect["height"] += 1
            else:
                rect = {"x": x0, "y": y, "width": x - x0, "height": 1, "color": color}
                rectangles.append(rect)
            current[key] = rect
        previous = current

    rectangles.sort(key=lambda r: (r["y"], r["x"], r["color"]))
    return "".join(
        f'<rect x="{r["x"]}" y="{r["y"]}" width="{r["width"]}" height="{r["height"]}" '
        f'fill="#{r["color"][0]:02X}{r["color"][1]:02X}{r["color"][2]:02X}"/>'
        for r in rectangles
    )


def safe_output_path(root: Path, relative: str) -> Path:
    rel = Path(relative)
    if rel.is_absolute() or ".." in rel.parts:
        raise SystemExit(f"Unsafe path in {MANIFEST_NAME}: {relative}")
    return root / rel


def export_assets(args) -> int:
    data = load_data(args.json)
    if (args.out / MANIFEST_NAME).exists() and not args.overwrite:
        raise SystemExit(f"Refusing to replace existing {args.out / MANIFEST_NAME}; use --overwrite or choose a new --out folder")
    groups = [args.group] if args.group else list(GROUPS)
    assets = []
    seen_files = set()
    for group in groups:
        entries = data[group]
        names = [args.name] if args.name else list(entries)
        for name in names:
            if name not in entries:
                raise SystemExit(f"Unknown {group} art entry: {name}")
            relative = Path(group) / f"{safe_name(name)}.png"
            if relative.as_posix() in seen_files:
                raise SystemExit(f"Art names collide as an output file: {relative}")
            seen_files.add(relative.as_posix())
            output = args.out / relative
            if output.exists() and not args.overwrite:
                raise SystemExit(f"Refusing to overwrite {output}; use --overwrite or choose a new --out folder")
            output.parent.mkdir(parents=True, exist_ok=True)
            size = canvas_for(group)
            render_fragment(entries[name], size).save(output)
            assets.append({"group": group, "name": name, "size": size, "file": relative.as_posix()})

    args.out.mkdir(parents=True, exist_ok=True)
    manifest = {"format_version": FORMAT_VERSION, "source_json": str(args.json), "assets": assets}
    (args.out / MANIFEST_NAME).write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Exported {len(assets)} native-size PNGs to {args.out}")
    print(f"Open/edit the PNGs in Aseprite; keep the canvas size and save with the same filename.")
    return 0


def find_aseprite(explicit: str | None) -> str | None:
    if explicit:
        return explicit.strip().strip('"')
    env = os.environ.get("ASEPRITE", "").strip().strip('"')
    if env:
        return env
    for name in ("aseprite", "aseprite.exe", "Aseprite.exe"):
        found = shutil.which(name)
        if found:
            return found
    return None


def latest_art_file(base: Path) -> Path | None:
    candidates = [base.parent / (base.name + ext) for ext in (".png", ".aseprite", ".ase")]
    existing = [p for p in candidates if p.is_file()]
    if not existing:
        return None
    # Prefer whichever counterpart was saved most recently (the user's latest edit).
    return max(existing, key=lambda p: p.stat().st_mtime_ns)


def read_edited_image(path: Path, aseprite: str | None, tempdir: Path) -> Image.Image:
    if path.suffix.lower() == ".png":
        try:
            with Image.open(path) as opened:
                return opened.convert("RGBA").copy()
        except Exception as exc:
            raise SystemExit(f"Cannot open PNG {path}: {exc}") from exc
    if path.suffix.lower() not in (".ase", ".aseprite"):
        raise SystemExit(f"Unsupported art file extension: {path.suffix}")
    if not aseprite:
        raise SystemExit(
            f"{path} is an Aseprite project. Install/locate Aseprite CLI, then pass --aseprite PATH "
            "or set ASEPRITE; alternatively save/export it as a PNG."
        )
    png = tempdir / (path.stem + "_aseprite_export.png")
    try:
        subprocess.run([aseprite, "-b", str(path), "--save-as", str(png)], check=True)
    except (OSError, subprocess.CalledProcessError) as exc:
        raise SystemExit(f"Aseprite CLI export failed for {path}: {exc}") from exc
    if not png.is_file():
        raise SystemExit(f"Aseprite CLI did not create the expected PNG: {png}")
    with Image.open(png) as opened:
        return opened.convert("RGBA").copy()


def import_assets(args) -> int:
    manifest_path = args.input / MANIFEST_NAME
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise SystemExit(f"Cannot read {manifest_path}; export the art first: {exc}") from exc
    if manifest.get("format_version") != FORMAT_VERSION or not isinstance(manifest.get("assets"), list):
        raise SystemExit(f"Unsupported or invalid {MANIFEST_NAME}")

    data = load_data(args.json)
    selected = []
    for asset in manifest["assets"]:
        group, name, size = asset.get("group"), asset.get("name"), asset.get("size")
        if group not in GROUPS or name not in data[group]:
            raise SystemExit(f"Invalid art asset entry in manifest: {asset}")
        if args.group and group != args.group:
            continue
        if args.name and name != args.name:
            continue
        expected = canvas_for(group)
        if size != expected:
            raise SystemExit(f"Manifest canvas mismatch for {group}/{name}: expected {expected}, got {size}")
        selected.append((asset, group, name, expected))
    if not selected:
        raise SystemExit("No manifest entries matched the requested group/name")

    aseprite = find_aseprite(args.aseprite)
    updates = {}
    missing = []
    with tempfile.TemporaryDirectory(prefix="ae_aseprite_") as temp:
        tempdir = Path(temp)
        for asset, group, name, size in selected:
            png_path = safe_output_path(args.input, asset["file"])
            base = png_path.with_name(png_path.name[:-len(png_path.suffix)])
            edited = latest_art_file(base)
            if edited is None:
                missing.append(f"{group}/{name}")
                continue
            image = read_edited_image(edited, aseprite, tempdir)
            if image.size != (size, size):
                raise SystemExit(f"{edited} is {image.width}x{image.height}; {group}/{name} must stay {size}x{size}")
            try:
                fragment = png_to_fragment(image)
            except ValueError as exc:
                raise SystemExit(f"Invalid pixels in {edited}: {exc}") from exc
            current = render_fragment(data[group][name], size)
            if image.tobytes() != current.tobytes():
                updates[(group, name)] = fragment

    if missing:
        print("Skipped missing files: " + ", ".join(missing))
    if not updates:
        print("No art changes detected; art.json and generated tables were left untouched.")
        return 0

    for (group, name), fragment in updates.items():
        data[group][name] = fragment
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    backup = args.json.with_name(args.json.name + f".{stamp}.bak")
    shutil.copy2(args.json, backup)
    args.json.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Updated {len(updates)} art entries in {args.json}; backup: {backup}")

    if args.regen and args.json.resolve() == DEFAULT_JSON.resolve():
        try:
            subprocess.run([sys.executable, str(ROOT / "tools" / "gen_data.py")], cwd=ROOT, check=True)
        except (OSError, subprocess.CalledProcessError) as exc:
            raise SystemExit(f"JSON was updated, but data generation failed: {exc}") from exc
        print("Regenerated source/data_gen.cpp")
    elif args.regen:
        print("Skipped C++ regeneration because --json is not the project's default art JSON.")
    return 0


def require_pillow() -> None:
    if Image is None:
        raise SystemExit("This optional tool needs Pillow. Install with `python3 -m pip install pillow` or `py -3 -m pip install pillow`.") from PIL_IMPORT_ERROR


def make_parser():
    parser = argparse.ArgumentParser(description=__doc__, add_help=False)
    parser.add_argument("-h", "--h", "-help", "--help", action="help", help="show this help message and exit")
    sub = parser.add_subparsers(dest="command", required=True)

    exp = sub.add_parser("export", help="export the current art JSON to native-size PNGs", add_help=False)
    exp.add_argument("-h", "--h", "-help", "--help", action="help", help="show this help message and exit")
    exp.add_argument("--json", type=Path, default=DEFAULT_JSON, help="source art JSON")
    exp.add_argument("--out", type=Path, default=DEFAULT_WORK, help="Aseprite work folder")
    exp.add_argument("--group", choices=GROUPS, help="export one art group")
    exp.add_argument("--name", help="export one named entry; requires --group")
    exp.add_argument("--overwrite", action="store_true", help="allow overwriting existing PNGs/manifest")

    imp = sub.add_parser("import", help="import edited PNG/Aseprite files into art.json", add_help=False)
    imp.add_argument("-h", "--h", "-help", "--help", action="help", help="show this help message and exit")
    imp.add_argument("--json", type=Path, default=DEFAULT_JSON, help="target art JSON")
    imp.add_argument("--input", type=Path, default=DEFAULT_WORK, help="work folder created by export")
    imp.add_argument("--group", choices=GROUPS, help="import only one art group")
    imp.add_argument("--name", help="import only one named entry")
    imp.add_argument("--aseprite", help="Aseprite executable path (or set ASEPRITE)")
    imp.add_argument("--no-regen", dest="regen", action="store_false", help="do not run tools/gen_data.py after import")
    imp.set_defaults(regen=True)
    return parser


def main() -> int:
    parser = make_parser()
    args = parser.parse_args()
    require_pillow()
    if args.command == "export" and args.name and not args.group:
        parser.error("--name requires --group")
    if args.command == "import" and args.name and not args.group:
        parser.error("--name requires --group")
    if args.command == "export":
        return export_assets(args)
    return import_assets(args)


if __name__ == "__main__":
    raise SystemExit(main())
