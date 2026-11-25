# AI Changes — AbyssalEclipse stable v0.2

This file tracks changes authored or applied with AI assistance. Human-authored art and the existing data model were preserved.

## Implemented in this pass

- Added project-local agent skills covering art direction, save compatibility, Old 3DS performance, QoL/localization, balance, and the MrJuaumBR ecosystem contract.
- Added a GitHub Pages catalogue site with responsive theme controls, English/Brazilian Portuguese labels, game previews, full `CATALOG.md` loading, `romfs/note.txt` changelog loading, and this file.
- Added the missing main-menu Shop entry and moved the coin balance above the bottom hint line without changing the save format.
- Added a safe Shop screen placeholder that exposes the current balance and explains that purchases will be data-backed in a later update.

## Bugs & Improvments

- The static repository snapshot has no `previews/` directory even though the requested Pages site expects preview PNGs. The site now has a graceful empty state and the release adds curated copies of existing authored concept PNGs.
- Real devkitPro/devkitARM compilation and hardware smoke testing are unavailable in this environment; the host/stub checks remain the release gate here.
- `romfs/note.txt` is loaded as plain text by the Pages site, so future changelogs should avoid relying on terminal control characters or unsupported markup.
- The event campaigns are intentionally visible placeholders until gameplay/save IDs are authored; they must not be made selectable by guessing a legacy season ID.
- Shop purchases are intentionally not persisted yet. Adding currency deductions without a catalog, prices, and migration contract would risk corrupting progression.
- The Endless rules popup currently dismisses on any touch; a future touch polish pass should distinguish the popup body from its close action while retaining B/A dismissal.

## Verification notes

Host-side Python art tests and source-level checks should be run before packaging. A real 3DS build is still required for final release confidence.

## Recheck against updated prompt

- Rebuilt `index.html` from the new ZIP base with a real appearance dialog, persisted dark/light and palette state, reduced-motion preference, responsive ecosystem tokens, and a local Markdown renderer. The page explains the `file://` limitation and provides direct source links when fetch is unavailable.
- Added the supplied `data/` screenshots and `previews/` assets to the page.
- Kept Shop visible as a disabled/coming-soon menu entry; it does not open a screen or change progression.
- Reworked Endless selection into a 5-season × 4-level future-proof grid. The first three level cells are selectable; the fourth is explicitly locked for future content. Multiple seasons and levels can be toggled, and the Game Rules popup edits Random Spell and Lives.
- A second touch on a selected Campaigns hub card now enters that campaign group directly; the same action remains available through A after D-pad selection.

## Updated prompt verification

The new prompt was applied to the fresh ZIP rather than silently merging against the older working tree. The Shop is disabled, not a fake purchasable screen; the Endless grid is backed by explicit per-season level masks; and the Campaign hub touch path distinguishes selection from direct entry.
