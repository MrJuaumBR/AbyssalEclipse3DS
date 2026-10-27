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
- D01 OK 137 spells (22/90/25 verified by test)  D02 OK 25 enemies  D03 OK 4 weapons  D04 OK en + pt-BR + fallback  D05 OK index-based (MIRAGE x2)

## Spell engine
- S01-S08 OK (see APPROX list in README for 9 simplified spells)

## Enemies
- E01-E14 APPROX  behaviours are my interpretation of the special names; verify each against the design doc text
- E15 APPROX  five bosses implemented with 2-3 attacks each

## Seasons
- Z01 OK Forest brambles (block shots, slow everyone)  Z02 APPROX Desert heat pulse  Z03 APPROX Snow frost patches
- Z04 APPROX Mountain rockfall  Z05 APPROX Eclipse darkness (design-doc gimmicks for 2-5 were not specified; these are mine)

## Screens
- U01-U12 OK (pause, debug, run summary included). Level names for seasons 2-5 are PLACEHOLDERS (marked on screen)

## Systems
- Y01 OK procedural atlas, 256x256 (fits; test asserts)  Y02 OK save/load/CRC/corruption fallback
- Y03 OK mixer verified silent-vs-audible in test; sound QUALITY is UNVERIFIED (never listened)
- Y04 OK adaptive 60/30 logic  Y05 UNVERIFIED suspend hook  Y06 OK live language switch
- Y07 OK   soak test + ASan/UBSan clean  Y08 OK   syntax check vs MY OWN stubs only  Y09 UNVERIFIED Makefile (not run with devkitARM)

## 0.1 release additions
- R01 OK   Ordinary cosmetics are always usable; only Batman, Joker, Terraria, Mario, and Iron Man parody outfits are achievement-gated and skipped until unlocked
- R02 OK   Four catalog tabs: spells, bestiary, clothes, and music
- R03 OK   Completed-season music tracks unlock in the Music catalog and can be previewed with A
- R04 OK   Active gameplay timer counts wave time only; menu, pause, allocation, and spell-choice time are excluded
- R05 OK   Stat points can be deferred with a maximum of three store actions per run and can be reclaimed during allocation
- R06 OK   Global XP gain increased by 5% before Luck's existing modifier
