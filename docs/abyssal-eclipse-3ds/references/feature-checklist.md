# Feature checklist (status after build 1)
Legend: OK = implemented and exercised by the host soak test | APPROX = implemented, simplified (reason given) | UNVERIFIED = needs real devkitPro / hardware | TODO

## Core loop
- F01 OK   Auto-attack nearest enemy; weapon is slot 1 of 12
- F02 OK   Movement D-pad + Circle Pad, bounds
- F03 OK   Wave director, timed waves, boss waves, 48 enemy cap
- F04 OK   XP gems, magnet, curve 10+(l-1)*5, 1 point/level
- F05 OK   PLAY / CLEARED / ALLOC / SPELL / DEAD / VICTORY (+ forced spell phase from debug)
- F06 OK   STR/RES/SPD/LUCK/INT per-point effects (base = zero point)
- F07 OK   3 offers 60/28/10/2 (+LUCK), 12 slots, upgrade offers at 12/12 (3 levels, +20%)
- F08 OK   Death resets transient state only; Phoenix Down revive
- F09 OK   Campaign 10/15/20 waves, unlock chain; Endless with best wave and tier scaling

## Data
- D01 OK 137 spells (22/90/25 verified by test)  D02 OK 25 enemies  D03 OK 9 weapons  D04 OK en + pt-BR + fallback  D05 OK index-based (MIRAGE x2)

## Spell engine
- S01-S08 OK (see APPROX list in README for 9 simplified spells)

## Enemies
- E01-E14 APPROX  behaviours are my interpretation of the special names; verify each against the design doc text
- E15 APPROX  five bosses implemented with 2-3 attacks each

## Seasons
- Z01 OK Forest brambles (block shots, slow everyone)  Z02 APPROX Desert heat pulse  Z03 APPROX Snow frost patches
- Z04 APPROX Mountain rockfall  Z05 APPROX Eclipse darkness (design-doc gimmicks for 2-5 were not specified; these are mine)

## Screens
- U01-U12 OK (pause, debug, run summary included). U13 OK distinct level names for all five seasons

## Systems
- Y01 OK procedural atlas, 256x256 (fits; test asserts)  Y02 OK save/load/CRC/corruption fallback
- Y03 OK mixer verified silent-vs-audible in test; sound QUALITY is UNVERIFIED (never listened)
- Y04 OK adaptive 60/30 logic  Y05 UNVERIFIED suspend hook  Y06 OK live language switch
- Y07 OK   soak test + ASan/UBSan clean  Y08 OK   syntax check vs MY OWN stubs only  Y09 UNVERIFIED Makefile (not run with devkitARM)

## 0.1 release additions
- R01 OK   Ordinary cosmetics are always usable; only Batman, Joker, Terraria, Mario, and Iron Man parody outfits are achievement-gated and skipped until unlocked
- R02 OK   Five visible, directly selectable catalog tabs: spells, bestiary, clothes, music, and weapons
- R03 OK   Completed-season music tracks unlock in the Music catalog and can be previewed with A
- R04 OK   Active gameplay timer counts wave time only; menu, pause, allocation, and spell-choice time are excluded
- R05 OK   Stat points can be deferred with a maximum of three store actions per run and can be reclaimed during allocation
- R06 OK   Global XP gain increased by 5% before Luck's existing modifier
- R07 OK   Flush the 3DS atlas staging cache before cosmetic partial uploads to prevent stale Old 3DS sprites

## Follow-up UI and combat fixes
- F10 OK   Selected spell detail shows description, current rank, cumulative numeric scaling, and Rank-1 stat values in a readable block; attack counters and cooldown indicators remain on the slot card
- F11 OK   Equipped damage, pull, weakening, miss, and aura-heal effects show a persistent player-centered range ring
- F12 OK   All nine weapon IDs have nonzero combat stats; Katana melee damage and Machine Gun projectile mapping have host regression coverage
- A01 OK   Katana icon is a simplified horizontal 16×16 sword silhouette to avoid noisy diagonal strips
- F13 OK   Katana hits create a short-sweeping violet/magenta crescent slash with spark accents and a dedicated procedural swish cue
- F14 OK   Catalog exposes all five categories in a visible tab strip; Weapons lists all nine weapons with stats and lock state
- F15 OK   Resistance point-up text explicitly says +6 HP and -2% damage taken; allocation UI is drawn in English and Portuguese tests
- F16 APPROX   Aura rings pulse; lightning arcs to nearby enemies and Vacuum sends white particles outward (visual intensity needs emulator/3DS review)
- F17 OK   Katana target range is reduced to 56 px and cleave radius to 46 px; host regression checks both balance values
- F18 OK   Orbit/summon spells cap at Rank 5, Time-Stop at Rank 9, and other spells at Rank 10; host tests check every adjacent effective rank, offer eligibility, cap enforcement, and bilingual rank/scaling summaries
