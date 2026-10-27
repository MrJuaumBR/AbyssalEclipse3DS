#!/usr/bin/env python3
"""Regression checks for the small vector-to-pixel-art converter."""
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from art_raster import rects


def pixels(shapes):
    out = [[0] * 16 for _ in range(16)]
    for x, y, w, h, color in shapes:
        for yy in range(y, min(16, y + h)):
            for xx in range(x, min(16, x + w)):
                out[yy][xx] = color
    return out


def main():
    data = json.load(open(os.path.join(ROOT, 'tools', 'data', 'art.json'), encoding='utf-8'))
    eclipse = pixels(rects(data['ICO']['ecl']))
    assert eclipse[3][8] == 0xE8B54B, 'crescent should retain its gold sun arc'
    assert eclipse[8][4] == 0x0B0A12, 'dark disc must paint over the sun in source order'
    assert eclipse[3][4] == 0, 'circle corners must remain transparent, not square-filled'
    assert eclipse[8][0] == 0xB83A8A, 'outer icon rays must remain behind the discs'

    clock = pixels(rects(data['ICO']['clock']))
    assert clock[4][7] == 0x0B0A12, 'clock hand must paint over the clock face'
    assert clock[2][2] == 0, 'clock face corners must remain transparent'

    snow = pixels(rects(data['ICO']['snow']))
    assert snow[1][8] == 0xCFE8F5, 'snowflake lines must inherit the group stroke'
    assert snow[0][0] == 0, 'snowflake must not fill unrelated pixels'
    print('art rasterizer: OK (circle masks, paint order, group strokes)')


if __name__ == '__main__':
    main()
