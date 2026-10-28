# Abyssal Eclipse 0.2 BETA — change tracking

## Changes made in this workspace

- Removed the discontinued mining cosmetic completely: its achievement, localization strings, art JSON entry, preview PNG, Aseprite asset, generated C++ art, and manifest entry are gone.
- Replaced protected-character references with original project names: Nightwarden, Grinmask, Redcap, Arc Warden, and Elementalist. The generated source, localization data, documentation, and filenames were updated together.
- Added [`tools/translation_editor.html`](tools/translation_editor.html), a browser editor that loads the existing `LANGS` strings from `mock_data.json`, supports adding languages and editing translations, and exports a replacement `mock_data.json`.
- Replaced `preview.html` with a searchable, category-filtered art gallery that supports drag-and-drop loading and displays the current version.
- Added `tools/data/project.json` as the single editable version source. The generator injects it into both language tables; the project is now `v0.2 BETA`.
- Lowered `bal::BASE_HP` from 60 to 40.
- Rebalanced Sentinel Drone pressure with a lower damage coefficient and slower firing interval. The tuning values are now named constants in `source/balance.h`.
- Moved the in-game life readout into the top health bar, centered at a smaller scale so it no longer overflows below the menu.
- Reduced the cosmetic collection from 11 to 10 entries and kept save validation/indexing aligned with the removal.
- Extended the host regression run window to account for the lower-health balance while preserving the existing gameplay coverage.

## GitHub 0.1 baseline comparison

The supplied GitHub repository contains the tagged `0.1` stable commit (`18c9801`) and a later `main` commit (`978333d`, “Update 0.1 - QoL, Fixes and Improvments”). Relative to the `0.1` tag, that later repository update added or changed the following areas:

- **Content/reference data:** added `CATALOG.md`, expanded `mock_data.json` and `art.json`, regenerated spell/enemy/art tables, and added the catalog generator.
- **Gameplay and balance:** added or expanded spell rank/cap behavior, summon and Sentinel/Drone systems, aura effects, weapon additions and tuning, XP tuning, enemy behaviors, and simulation coverage.
- **UI and presentation:** added catalog and achievement screens, additional in-game HUD rendering, weapons navigation, localization strings, UI fitting, procedural visual effects, and screenshots.
- **Progression and saves:** added lifetime counters, spell-use history, additional achievement logic, catalog progress, save migration support, and v5 round-trip coverage.
- **Tooling and verification:** added/expanded host tests, art rasterization tests, Aseprite synchronization support, and the catalog-generation path.

The comparison used the repository’s actual `0.1` tag rather than assuming the current `main` state was the original 0.1 release. The current workspace changes above are separate from that historical comparison and are intended to be the 0.2 BETA tracking baseline.
