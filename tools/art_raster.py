"""Rasterize the project's small SVG-like art fragments into 16x16 PixRect runs.

The game stores only opaque colored rectangles in its atlas. This module converts
vector circles and strokes into pixel-center coverage while preserving source
paint order and simple inherited <g> stroke styles.
"""
import math
import re

_TAG = re.compile(r'<(/?)([A-Za-z]+)\b([^>]*)>')
_ATTR = re.compile(r'([A-Za-z_:][-A-Za-z0-9_:.]*)\s*=\s*"([^"]*)"')
_SIZE = 16


def _color(value):
    if not value or value == 'none':
        return None
    m = re.fullmatch(r'#([0-9A-Fa-f]{6})', value)
    return int(m.group(1), 16) if m else None


def _runs(mask, color, out):
    if color is None:
        return
    for y, row in enumerate(mask):
        x = 0
        while x < _SIZE:
            if not row[x]:
                x += 1
                continue
            x0 = x
            while x < _SIZE and row[x]:
                x += 1
            out.append((x0, y, x - x0, 1, color))


def _empty_mask():
    return [[False] * _SIZE for _ in range(_SIZE)]


def _segment_distance(px, py, x1, y1, x2, y2):
    dx, dy = x2 - x1, y2 - y1
    den = dx * dx + dy * dy
    t = 0.0 if den == 0 else max(0.0, min(1.0, ((px-x1)*dx + (py-y1)*dy) / den))
    return math.hypot(px - (x1 + t * dx), py - (y1 + t * dy))


def rects(svg):
    """Return (x,y,w,h,rgb) horizontal-run rectangles for a 16x16 SVG fragment."""
    out = []
    # Each group frame stores inherited stroke color and width.
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
        stroke = _color(attrs.get('stroke'))
        if stroke is None:
            stroke = groups[-1][0]
        stroke_width = float(attrs.get('stroke-width', groups[-1][1]))
        fill = _color(attrs.get('fill'))

        if tag == 'rect':
            x, y = float(attrs['x']), float(attrs['y'])
            w, h = float(attrs['width']), float(attrs['height'])
            if fill is not None and w > 0 and h > 0:
                mask = _empty_mask()
                for py in range(_SIZE):
                    for px in range(_SIZE):
                        cx, cy = px + 0.5, py + 0.5
                        mask[py][px] = x <= cx < x + w and y <= cy < y + h
                _runs(mask, fill, out)
            if stroke is not None and stroke_width > 0 and w > 0 and h > 0:
                mask = _empty_mask()
                half = stroke_width * 0.5
                for py in range(_SIZE):
                    for px in range(_SIZE):
                        cx, cy = px + 0.5, py + 0.5
                        inside = x <= cx < x + w and y <= cy < y + h
                        near = min(abs(cx-x), abs(cx-(x+w)), abs(cy-y), abs(cy-(y+h))) <= half
                        mask[py][px] = inside and near
                _runs(mask, stroke, out)

        elif tag in ('circle', 'ellipse'):
            cx, cy = float(attrs['cx']), float(attrs['cy'])
            rx = float(attrs.get('r', attrs.get('rx', 0)))
            ry = float(attrs.get('r', attrs.get('ry', 0)))
            if rx <= 0 or ry <= 0:
                continue
            fill_mask = _empty_mask()
            stroke_mask = _empty_mask()
            half = stroke_width * 0.5
            for py in range(_SIZE):
                for px in range(_SIZE):
                    nx = (px + 0.5 - cx) / rx
                    ny = (py + 0.5 - cy) / ry
                    radius = math.hypot(nx, ny)
                    if fill is not None and radius <= 1.0:
                        fill_mask[py][px] = True
                    # Approximate a vector stroke in normalized ellipse space.
                    # For circles this is exact; on ellipses it is a pixel-art-friendly outline.
                    if stroke is not None and stroke_width > 0 and abs(radius - 1.0) <= half / max(rx, ry):
                        stroke_mask[py][px] = True
            _runs(fill_mask, fill, out)
            _runs(stroke_mask, stroke, out)

        elif tag == 'line':
            color = stroke or 0xFFFFFF
            x1, y1 = float(attrs['x1']), float(attrs['y1'])
            x2, y2 = float(attrs['x2']), float(attrs['y2'])
            mask = _empty_mask()
            radius = max(0.5, stroke_width * 0.5)
            for py in range(_SIZE):
                for px in range(_SIZE):
                    if _segment_distance(px + 0.5, py + 0.5, x1, y1, x2, y2) <= radius:
                        mask[py][px] = True
            _runs(mask, color, out)
    return out
