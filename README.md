# Abyssal Eclipse 3DS

| Main Menu | Campaigns | Catalog | Achievements | In-Game |
| --- | --- | --- | --- | --- |
| <img src="./data/main-menu.png" width="260" alt="Main Menu"> | <img src="./data/2026-10-26_22-20-54.281_0004000000BAB300_cmb.bmp" width="260" alt="Campaigns"> | <img src="./data/catalog.png" width="260" alt="Catalog"> | <img src="./data/achievments.png" width="260" alt="Achievements"> | <img src="./data/in-game.png" width="260" alt="In Game"> |

**Survive the abyss. Master the elements. Break the cycle.**

**Status: v0.1 RELEASE.** Abyssal Eclipse is a 2D pixel-art arena-survivor roguelite built natively for the original Nintendo 3DS. It is written in C++17 with devkitARM, libctru, citro3d, and citro2d.

Sprites, bitmap text, sound effects, and music are generated procedurally; the game does not require external runtime art files. A 256×256 RGBA8 texture atlas is built in RAM at startup.

## Game and current features

You control a lone knight in a looping nightmare. Weapons attack automatically; move, dodge, collect experience, spend stat points, and build a spell loadout through timed waves and boss fights.

### Spell ranks and caps

Spell upgrades advance one rank at a time. Rank 5 is **not** the general cap: most spells can reach Rank 10. Where applicable, generic numeric effects gain 20% per rank above Rank 1; custom timing, trigger frequency, and discrete counts follow their spell-specific formulas. Capped spells stop appearing as upgrade choices.

These six orbiting or summoned-entity spells are intentionally capped at **Rank 5** to prevent runaway entity counts: **Flame Wheel, Ball Lightning, Knife Circle, Sentinel, Sentinel Drone, and Fairy Companion**. **Time-Stop** caps at Rank 9 because its interval, duration, and cooldown reach their tuned limits by then. Most other spells can reach Rank 10. Avatar Cycle triggers periodic elemental attacks rather than adding a persistent summon, so it retains the higher cap.

The selected-spell detail panel shows its description, current rank/cap, applicable numeric scaling or a custom-effect label, and Rank-1 stat values. The spell catalog also lists each spell’s authored description, stats, effect payload, scope, and cap in [CATALOG.md](CATALOG.md).

### World, enemies, and progression

- **147 spells** across six elements and five seasons, including global, element-linked, and season-exclusive spells.

- **25 enemy types**, including seasonal enemies, bosses, and distinct attack patterns.

- **Five seasons:** Forest, Desert, Snow, Mountain, and Eclipse, with a unique name for each campaign level.

- **All-Seasons Map:** unlocks after clearing all 15 campaign levels across the five seasons; the scrolling 800×600 arena mixes enemies from every season.

- **Twenty-three achievements:** season completion, lifetime kill milestones, catalog discovery, spell-use challenges, play-time milestones, and cosmetic unlocks.

- **Lifetime statistics:** games, wins, losses, enemies killed, spells acquired and activated, spell upgrades, weapon attacks, and play time appear on the achievements screen.

- **Play-time achievements:** Little Dummy at 1 hour, Abyss Veteran at 10 hours, and Go Touch Grass at 50 hours; several special outfits have separate achievement rewards.

- **Achievement rewards:** ordinary hats, faces, colors, and the Dwarf Beard are available immediately. Five parody outfits and the Avatar air-nomad outfit are achievement-gated.

- **Stat-point bank:** unused level-up points can be stored for later in a run, with a hard limit of three store actions per session.

- **Active play-time counter:** lifetime time counts only active wave gameplay—not menus, allocation, spell selection, or paused time.

### Weapons, spells, and presentation

- **Nine weapons:** Dagger, Bow, Throwing Axe, Mage Staff, Shotgun, Pistol, retrievable Trident, Katana, and Machine Gun. Machine Gun unlocks after clearing every campaign level in all five seasons. The Catalog’s Weapons page lists descriptions, damage, attack interval, range, and lock status.

- **Katana swing:** close-range arc cleave with a curved slash trail and a dedicated procedural swish cue; its range was reduced.

- **Aura effects:** pulsing rings; electrical fields arc lightning to nearby enemies; Vacuum has a white field and outward-moving particles.

- **Spell HUD:** trigger counters, cooldown progress bars, and compact spell names on bottom-screen slot cards. The selected spell’s expanded information appears in the detail panel.

- **Chronos rarity:** Time-Stop is a legendary global spell that triggers every 12 attacks, freezes enemies and hostile projectiles for 1.2 seconds, and then requires a 14-second cooldown. Its higher ranks improve the effect modestly while preserving its emergency-use role.

- **Resistance clarity:** each Resistance point-up states its exact effect: +6 HP and 2% less damage taken.

- **UI fitting:** prominent button, header, menu, Catalog, and achievement labels are measured against bitmap-font widths and reduced when needed to avoid overflow.

- **Catalog and meta-progression:** five labeled tabs show spells, bestiary, clothes, music, and Weapons. Music tracks can be previewed after unlocking their season.

- **Music:** five procedural season tracks, plus a separate internal menu pattern.

- **Balance update:** XP gains receive a global +5% release-tuning bonus before Luck modifiers.

- **Localization:** English and Brazilian Portuguese.

### Technical notes

- **Old 3DS-conscious runtime:** fixed gameplay pools, no gameplay heap allocation, compact procedural atlas, and optional 30 FPS mode.

- **Old 3DS refresh fix:** cosmetic atlas updates flush the staging texture cache before upload, preventing stale character regions on hardware.

- **Circle-art rasterization:** procedural SVG fragments are rasterized in paint order with circle/ellipse geometry and inherited group strokes.

- **Save compatibility:** the save loader migrates legacy v1, v2, v3, and v4 files to the current format. Newly added counters start at zero; existing unlocks and Catalog progress are retained.

For the complete spell-by-spell and item-by-item reference—including descriptions, localized text, effect payloads, rank caps, weapon stats, music/SFX, enemies, seasons, achievements, cosmetics, and player stats—see [**CATALOG.md**](CATALOG.md).

## Controls

- **D-Pad / Circle Pad:** Move during a run; navigate menus.

- **A:** Confirm/select. Attacks are automatic.

- **B:** Go back / pause-menu back action.

- **L / R:** Switch campaign seasons; cycle in-game tabs and Catalog categories.

- **X / Y:** Select spell slots; scroll Catalog lore. Y also starts an All-Seasons run after unlocking it.

- **START:** Pause.

- **SELECT:** Open the developer debug menu.

- **Touch screen:** Select menu items, campaign levels, spell cards, and visible Catalog tabs.

## Build and verification

Install devkitPro with the 3DS development packages and Python 3. Set `DEVKITPRO` and `DEVKITARM`, then build from the repository root:

```
python3 tools/gen_data.py       # regenerate runtime data/localized strings
python3 tools/gen_catalog.py    # regenerate CATALOG.md
make
```

See [BUILD.md](BUILD.md) for `.3dsx` output and optional `.cia` packaging instructions.

Run host, art, and backend syntax checks with:

```
sh verify.sh
```

The checks cover simulation, spell upgrades through each spell’s cap, weapon behavior, audio, save migration, achievement and Catalog/UI flows, procedural-art rasterization, and 3DS backend syntax.

## Known limitations

- A real devkitARM build and physical-console test have not been run in this host-only environment.

- The Katana slash, animated aura particles/lightning, and simplified 16×16 weapon icon still need visual review on an emulator or 3DS. Host tests cannot judge on-device readability or sound quality.

- Spell balance above Rank 5 has simulation regression coverage but still needs hands-on gameplay tuning.

- The gallery keeps the five image paths from the supplied README. Three referenced screenshots (`catalog.png`, `achievments.png`, and `in-game-katana.png`) were not included in the uploaded project archive, so add those files under `data/` for the links to render when the project is opened elsewhere.

## Credits

- **Game design and programming:** MrJuaumBR

- **AI assistants:** Claude, DeepSeek, Manus