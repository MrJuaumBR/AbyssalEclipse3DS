# Skill: Old 3DS Optimization

Target the original Nintendo 3DS first: fixed simulation pools, no gameplay heap allocation, compact procedural atlas, bounded strings, and predictable frame work. Keep 30 FPS as an option and avoid per-frame allocation, unbounded containers, expensive full-screen effects, and repeated file I/O.

Changes must be data-oriented and bounded by the existing constants in `balance.h`. Rendering effects are cosmetic; simulation correctness must not depend on them. Prefer integer coordinates and precomputed or low-cost math in hot loops. Verify with the host suite and a devkitARM syntax/build check when the toolchain is available.
