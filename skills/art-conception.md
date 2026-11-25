# Skill: Art Conception

AbyssalEclipse uses authored 16×16 pixel art with hard edges, opaque or fully transparent pixels, limited high-contrast palettes, and clear silhouettes at Old 3DS resolution. The visual language is dark abyssal violet with season accents: forest green, desert gold, snow cyan, mountain slate, and eclipse magenta. Concepts in `art_concept/` are references, not runtime replacements.

Keep sprites readable at native size. Preserve canvas dimensions and export through `tools/aseprite_sync.py`; do not introduce anti-aliasing, semi-transparent fringe pixels, or runtime heap allocations. New event icons belong in the `ICO` group and spell/pickup sprites in `SPELL`.
