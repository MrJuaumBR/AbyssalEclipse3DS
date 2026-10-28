#!/usr/bin/env python3
"""Round-trip tests for the optional Aseprite art authoring helper."""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOOL = ROOT / "tools" / "aseprite_sync.py"


def main():
    parser = argparse.ArgumentParser(description=__doc__, add_help=False)
    parser.add_argument("-h", "--h", "-help", "--help", action="help", help="show this help message and exit")
    parser.parse_args()
    try:
        from PIL import Image
    except ImportError as exc:
        raise SystemExit("This test needs Pillow. Install with `python3 -m pip install pillow` or `py -3 -m pip install pillow`.") from exc
    with tempfile.TemporaryDirectory(prefix="ae_art_sync_test_") as tmp:
        temp = Path(tmp)
        art_json = temp / "art.json"
        work = temp / "work"
        data = {
            "ENEMY": {},
            "HAT": {
                "test_hat": '<rect x="2" y="3" width="5" height="4" fill="#AA33CC"/>',
                "none": "",
            },
            "FACE": {},
            "ICO": {},
        }
        art_json.write_text(json.dumps(data), encoding="utf-8")

        subprocess.run([
            sys.executable, str(TOOL), "export", "--json", str(art_json),
            "--out", str(work), "--group", "HAT", "--name", "test_hat",
        ], check=True, stdout=subprocess.PIPE, text=True)
        png = work / "HAT" / "test_hat.png"
        with Image.open(png) as opened:
            assert opened.size == (32, 32)
            edited = opened.convert("RGBA")
        edited.putpixel((10, 10), (12, 220, 80, 255))
        edited.putpixel((3, 4), (0, 0, 0, 0))
        edited.save(png)

        subprocess.run([
            sys.executable, str(TOOL), "import", "--json", str(art_json),
            "--input", str(work), "--group", "HAT", "--name", "test_hat", "--no-regen",
        ], check=True, stdout=subprocess.PIPE, text=True)
        updated = json.loads(art_json.read_text(encoding="utf-8"))
        with Image.open(png) as opened:
            expected = opened.convert("RGBA").tobytes()
        from art_raster import rects
        actual = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
        pixels = actual.load()
        for x, y, w, h, color in rects(updated["HAT"]["test_hat"], size=32):
            rgba = ((color >> 16) & 255, (color >> 8) & 255, color & 255, 255)
            for yy in range(y, y + h):
                for xx in range(x, x + w):
                    pixels[xx, yy] = rgba
        assert actual.tobytes() == expected, "imported art pixels differ from the edited PNG"
        assert updated["HAT"]["none"] == "", "unselected entries must remain unchanged"
        assert len(list(temp.glob("art.json.*.bak"))) == 1, "import should back up the source JSON"

        # Simulate Aseprite's `-b project.aseprite --save-as output.png` CLI contract.
        project = png.with_suffix(".aseprite")
        project.write_bytes(b"test project marker")
        marker = temp / "aseprite_cli_was_called"
        fake_cli = temp / "fake_aseprite.py"
        fake_cli.write_text(
            "#!/usr/bin/env python3\nimport os, shutil, sys\nfrom pathlib import Path\n"
            "a=sys.argv[1:]; shutil.copyfile(Path(a[1]).with_suffix('.png'), a[3]); "
            "Path(os.environ['ASEPRITE_TEST_MARKER']).write_text('yes')\n",
            encoding="utf-8",
        )
        fake_cli.chmod(0o755)
        env = os.environ.copy()
        env["ASEPRITE_TEST_MARKER"] = str(marker)
        subprocess.run([
            sys.executable, str(TOOL), "import", "--json", str(art_json),
            "--input", str(work), "--group", "HAT", "--name", "test_hat",
            "--aseprite", str(fake_cli), "--no-regen",
        ], check=True, stdout=subprocess.PIPE, text=True, env=env)
        assert marker.is_file(), "Aseprite CLI conversion was not invoked"
        print("Aseprite sync: OK (PNG/native CLI import, exact dimensions, colors and transparency)")


if __name__ == "__main__":
    main()
