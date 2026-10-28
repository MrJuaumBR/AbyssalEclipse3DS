"""Rasterize the project's SVG-like art fragments into horizontal pixel runs.

Most game art remains 16x16. Cosmetic art can opt into a 32x32 canvas so hats
and face details are authored with a real pixel of their own instead of being
forced into the old coarse cell.
"""
import math
import re

_TAG = re.compile(r'<(/?)([A-Za-z]+)\b([^>]*)>')
_ATTR = re.compile(r'([A-Za-z_:][-A-Za-z0-9_:.]*)\s*=\s*"([^"]*)"')


def _color(value):
    if not value or value == 'none':
        return None
    m = re.fullmatch(r'#([0-9A-Fa-f]{6})', value)
    return int(m.group(1), 16) if m else None


def _runs(mask, color, out, size):
    if color is None:
        return
    for y, row in enumerate(mask):
        x = 0
        while x < size:
            if not row[x]:
                x += 1
                continue
            x0 = x
            while x < size and row[x]:
                x += 1
            out.append((x0, y, x - x0, 1, color))


def _empty_mask(size):
    return [[False] * size for _ in range(size)]


def _segment_distance(px, py, x1, y1, x2, y2):
    dx, dy = x2 - x1, y2 - y1
    den = dx * dx + dy * dy
    t = 0.0 if den == 0 else max(0.0, min(1.0, ((px-x1)*dx + (py-y1)*dy) / den))
    return math.hypot(px - (x1 + t * dx), py - (y1 + t * dy))


def rects(svg, size=16):
    """Return (x,y,w,h,rgb) pixel runs for an SVG fragment.

    ``size`` is the raster canvas. Geometry is interpreted directly in that
    canvas, so a cosmetic can use 32x32 coordinates while legacy art stays
    unchanged at 16x16.
    """
    out = []
    groups = [(None, 1.0)]
    for match in _TAG.finditer(svg):
        closing, tag, raw = match.groups()
        tag = tag.lower()
        attrs = dict(_ATTR.findall(raw))
        if tag == 'g':
            if closing:
                if len(groups) > 1:
                    groups.pop()
            else:
                inherited_color, inherited_width = groups[-1]
                group_color = _color(attrs.get('stroke'))
                groups.append((inherited_color if group_color is None else group_color,
                               float(attrs.get('stroke-width', inherited_width))))
            continue
        if closing:
            continue
        stroke = _color(attrs.get('stroke')) or groups[-1][0]
        stroke_width = float(attrs.get('stroke-width', groups[-1][1]))
        fill = _color(attrs.get('fill'))

        if tag == 'rect':
            x, y = float(attrs['x']), float(attrs['y'])
            w, h = float(attrs['width']), float(attrs['height'])
            if fill is not None and w > 0 and h > 0:
                mask = _empty_mask(size)
                for py in range(size):
                    for px in range(size):
                        cx, cy = px + 0.5, py + 0.5
                        mask[py][px] = x <= cx < x + w and y <= cy < y + h
                _runs(mask, fill, out, size)
            if stroke is not None and stroke_width > 0 and w > 0 and h > 0:
                mask = _empty_mask(size)
                half = stroke_width * 0.5
                for py in range(size):
                    for px in range(size):
                        cx, cy = px + 0.5, py + 0.5
                        inside = x <= cx < x + w and y <= cy < y + h
                        near = min(abs(cx-x), abs(cx-(x+w)), abs(cy-y), abs(cy-(y+h))) <= half
                        mask[py][px] = inside and near
                _runs(mask, stroke, out, size)

        elif tag in ('circle', 'ellipse'):
            cx, cy = float(attrs['cx']), float(attrs['cy'])
            rx = float(attrs.get('r', attrs.get('rx', 0)))
            ry = float(attrs.get('r', attrs.get('ry', 0)))
            if rx <= 0 or ry <= 0:
                continue
            fill_mask = _empty_mask(size)
            stroke_mask = _empty_mask(size)
            half = stroke_width * 0.5
            for py in range(size):
                for px in range(size):
                    nx = (px + 0.5 - cx) / rx
                    ny = (py + 0.5 - cy) / ry
                    radius = math.hypot(nx, ny)
                    if fill is not None and radius <= 1.0:
                        fill_mask[py][px] = True
                    if stroke is not None and stroke_width > 0 and abs(radius - 1.0) <= half / max(rx, ry):
                        stroke_mask[py][px] = True
            _runs(fill_mask, fill, out, size)
            _runs(stroke_mask, stroke, out, size)

        elif tag == 'line':
            color = stroke or 0xFFFFFF
            x1, y1 = float(attrs['x1']), float(attrs['y1'])
            x2, y2 = float(attrs['x2']), float(attrs['y2'])
            mask = _empty_mask(size)
            radius = max(0.5, stroke_width * 0.5)
            for py in range(size):
                for px in range(size):
                    if _segment_distance(px + 0.5, py + 0.5, x1, y1, x2, y2) <= radius:
                        mask[py][px] = True
            _runs(mask, color, out, size)
    return out


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description=__doc__ + " This module is also imported by the art tools.", add_help=False)
    parser.add_argument("-h", "--h", "-help", "--help", action="help", help="show this help message and exit")
    parser.parse_args()
    parser.print_help()
