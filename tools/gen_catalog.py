#!/usr/bin/env python3
"""Generate the comprehensive user/developer content reference CATALOG.md."""
from __future__ import annotations
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import gen_data as gd  # noqa: E402 - provides canonical localization/effect tables

D = gd.D
LANGS = D["LANGS"]
EXTRA = gd.EXTRA
LEVEL_FIVE_EFFECTS = gd.LEVEL_FIVE_EFFECTS


def cell(value) -> str:
    if value is None:
        return "—"
    return str(value).replace("|", "\\|").replace("\r", "").replace("\n", "<br>").strip() or "—"


def tr(key: str, lang: str) -> str:
    block = LANGS[lang]
    if key in block and not isinstance(block[key], (dict, list)):
        return str(block[key])
    item = EXTRA.get(key)
    if item:
        return item[0 if lang == "en" else 1]
    return key


def local_spell(name: str, lang: str, field: str):
    if lang == "en":
        return None
    entry = LANGS[lang].get("spell", {}).get(name)
    return entry.get(field) if entry else None


def local_enemy(enemy: dict, lang: str, field: str):
    if lang == "en":
        return enemy.get({"n": "name", "d": "desc", "sp": "sp"}[field])
    entry = LANGS[lang].get("enemy", {}).get(enemy["key"], {})
    return entry.get(field, enemy.get({"n": "name", "d": "desc", "sp": "sp"}[field]))


def lore(kind: str, key: str, lang: str) -> str:
    return D.get("LORE", {}).get(lang, {}).get(kind, {}).get(key, "") or "—"


def stats_text(value) -> str:
    if isinstance(value, list):
        return "; ".join(str(x) for x in value if x)
    return str(value or "—")


def fmt_num(value) -> str:
    if isinstance(value, (float, int)):
        return f"{value:g}"
    return str(value)


def get_array(source: str, name: str) -> list[str]:
    match = re.search(rf"\b{name}\s*\[[^]]*\]\s*=\s*\{{(.*?)\}};", source, re.S)
    if not match:
        raise RuntimeError(f"Could not parse {name} from source")
    return re.findall(r"S_[A-Za-z0-9_]+", match.group(1))


def parse_int_array(source: str, name: str) -> list[int]:
    match = re.search(rf"\b{name}\s*\[[^]]*\]\s*=\s*\{{(.*?)\}};", source, re.S)
    if not match:
        raise RuntimeError(f"Could not parse {name} from source")
    return [int(x) for x in re.findall(r"-?\d+", match.group(1))]


def spell_cap(spell: dict) -> int:
    effects = gd.M[(spell["name"], spell["season"])]
    if any(e[0] in LEVEL_FIVE_EFFECTS for e in effects):
        return 5
    return gd.LEVEL_CAP_OVERRIDES.get(spell["name"], gd.ABSOLUTE_MAX_SPELL_LEVEL)


def spell_sections(lines: list[str]) -> None:
    spells = D["SPELLS"]
    lines += ["## Spells", "", f"There are **{len(spells)} spell entries**. The table keeps the source-data index because **MIRAGE appears twice** (a global air spell and a Desert-exclusive spell); their effect payloads, scopes, and rank limits distinguish them.", "",
              "### Rank limits and progression", "",
              f"Every spell can be upgraded one rank at a time, up to its own cap. The absolute maximum is **{gd.ABSOLUTE_MAX_SPELL_LEVEL}**. Effects using generic numeric scaling gain **+20% per rank above Rank 1**; timing, trigger frequency, entity counts, and other custom parameters follow their spell-specific formulas.", "",
              "These six orbiting/summoned entity spells deliberately stop at **Rank 5** to prevent runaway entity counts: **FLAME WHEEL**, **BALL LIGHTNING**, **KNIFE CIRCLE**, **SENTINEL**, **SENTINEL DRONE**, and **FAIRY COMPANION**. **TIME-STOP** caps at Rank 9 because its interval, duration, and cooldown reach their tuned limits by then. Other spells are capped at Rank 10. AVATAR CYCLE is a periodic elemental attack, not a persistent summoned entity, so it retains the higher cap.", "",
              "Owned spells are offered as upgrades only when the 12-slot loadout is full; upgrade offers exclude spells already at their individual cap.", "",
              "### Full spell reference", "",
              "Effect payloads use internal effect names and source parameters, useful for distinguishing same-named entries and understanding the implementation. Lore is included when present in the source; `—` means no separate lore text is authored. PT-BR text falls back to English for any untranslated field.", "",
              "| # | Spell (EN / PT-BR) | Element | Type / activation | Rarity | Scope / season | Max rank | Effect payload | Base stats (EN / PT-BR) | Description (EN / PT-BR) | Lore (EN / PT-BR) |",
              "|---:|---|---|---|---|---|---:|---|---|---|---|"]
    for i, spell in enumerate(spells):
        name = spell["name"]
        pt_name = local_spell(name, "pt-br", "n") or name
        pt_desc = local_spell(name, "pt-br", "d") or spell["desc"]
        pt_stats = local_spell(name, "pt-br", "s")
        pt_stats = stats_text(pt_stats if pt_stats is not None else spell.get("stats"))
        en_stats = stats_text(spell.get("stats"))
        effects = gd.M[(name, spell["season"])]
        payload = "; ".join(f"{e[0]}({', '.join(fmt_num(x) for x in e[1:])})" for e in effects)
        scope = spell["scope"].upper()
        if spell.get("season"):
            scope += " / " + spell["season"]
        elif spell["scope"] == "element":
            scope += " / " + spell["element"].upper()
        en_lore = lore("spells", name, "en")
        pt_lore = lore("spells", name, "pt-br")
        type_act = f"{tr('ty_' + spell['type'], 'en')} / {tr('act_' + spell['activation'], 'en')}"
        lines.append("| {} | **{}** / {} | {} / {} | {} | {} | {} | {} | `{}` | {} / {} | {} / {} | {} / {} |".format(
            i, cell(name), cell(pt_name), cell(tr("el_" + spell["element"], "en")), cell(tr("el_" + spell["element"], "pt-br")),
            cell(type_act), cell(tr("r_" + spell["rarity"], "en")), cell(scope), spell_cap(spell), cell(payload),
            cell(en_stats), cell(pt_stats), cell(spell["desc"]), cell(pt_desc), cell(en_lore), cell(pt_lore)))
    lines.append("")


def weapon_section(lines: list[str]) -> None:
    source = (ROOT / "source" / "data_static.cpp").read_text(encoding="utf-8")
    balance = (ROOT / "source" / "balance.h").read_text(encoding="utf-8")
    name_keys = [x[2:].lower() for x in get_array(source, "WEAPON_NAME")]
    desc_keys = [x[2:].lower() for x in get_array(source, "WEAPON_FIRE")]
    stat_block = re.search(r"const WeaponStat WEAPON\[W_COUNT\]\s*=\s*\{(.*?)\};", balance, re.S)
    if not stat_block:
        raise RuntimeError("Could not parse weapon combat stats")
    rows = re.findall(r"\{([^{}]+)\}", stat_block.group(1))
    if not (len(name_keys) == len(desc_keys) == len(rows) == 9):
        raise RuntimeError(f"Weapon table mismatch: {len(name_keys)} names, {len(desc_keys)} descriptions, {len(rows)} stat rows")
    lines += ["## Weapons", "", "All nine selectable weapons have explicit combat stats. Only Machine Gun is completion-locked; the other eight are available from the weapon selector. Base values are before player stats and spell modifiers.", "",
              "| # | Weapon | Description (EN / PT-BR) | Damage | Attack interval | Range | Projectile speed | Pierce | Hit radius | Unlock / special behavior |",
              "|---:|---|---|---:|---:|---:|---:|---:|---:|---|"]
    for i, (name_key, desc_key, row) in enumerate(zip(name_keys, desc_keys, rows)):
        values = [float(x.strip().rstrip("f")) for x in row.split(",")]
        damage, interval, range_, speed, pierce, radius = values
        en_name, pt_name = tr(name_key, "en"), tr(name_key, "pt-br")
        en_desc, pt_desc = tr(desc_key, "en"), tr(desc_key, "pt-br")
        special = "Available from start."
        if name_key == "w_trident":
            special = "Thrown weapon; retrieve it by moving within 32 px."
        elif name_key == "w_katana":
            special = "Automatic close-range cleave; 46 px hit range; may cause bleed; distinct swing VFX/SFX."
        elif name_key == "w_machine_gun":
            special = "Unlock after clearing all three campaign levels in all five seasons; movement speed multiplier 0.70."
        lines.append(f"| {i} | **{cell(en_name)}** / {cell(pt_name)} | {cell(en_desc)} / {cell(pt_desc)} | {damage:g} | {interval:g} s | {range_:g} px | {speed:g} px/s | {pierce:g} | {radius:g} px | {cell(special)} |")
    lines.append("")


def music_and_sfx(lines: list[str]) -> None:
    audio = (ROOT / "source" / "audio.cpp").read_text(encoding="utf-8")
    names = ["FOREST", "DESERT", "SNOW", "MOUNTAIN", "ECLIPSE"]
    keys = ["music_forest", "music_desert", "music_snow", "music_mountain", "music_eclipse"]
    roots = parse_int_array(audio, "ROOT")[:5]
    scale_of = parse_int_array(audio, "SCALE_OF")[:5]
    scale_match = re.search(r"const int SCALE\[4\]\[7\]\s*=\s*\{(.*?)\};", audio, re.S)
    scales = [[int(x) for x in re.findall(r"-?\d+", row)] for row in re.findall(r"\{([^{}]+)\}", scale_match.group(1))]
    pattern = re.search(r"const s8 PAT\[6\]\[16\]\s*=\s*\{(.*?)\};", audio, re.S)
    patterns = [[int(x) for x in re.findall(r"-?\d+", row)] for row in re.findall(r"\{([^{}]+)\}", pattern.group(1))]
    sfx_names = re.search(r"enum Sfx\s*\{(.*?)\};", (ROOT / "source" / "audio.h").read_text(encoding="utf-8"), re.S).group(1)
    sfx_names = [x.strip().split("=")[0].strip().replace("SFX_", "") for x in sfx_names.split(",") if x.strip() and "SFX_COUNT" not in x]
    sfx_rows = re.search(r"const SfxDef SD\[SFX_COUNT\]\s*=\s*\{(.*?)\};", audio, re.S).group(1)
    defs = re.findall(r"\{([^{}]+)\}", sfx_rows)
    lines += ["## Music and sound effects", "", "Music is synthesized in real time by a compact 22.05 kHz mono software mixer. Each season has a procedural 16-step theme; the menu uses a sixth internal pattern that is not a catalog-unlock track. Tracks unlock by completing all three levels of their season.", "",
              "| Season | Track title (EN / PT-BR) | Root MIDI note | Scale intervals (semitones) | Pattern degrees (16 steps) | Unlock |",
              "|---|---|---:|---|---|---|"]
    for i, (season, key) in enumerate(zip(names, keys)):
        title = f"{tr(key, 'en')} / {tr(key, 'pt-br')}"
        pattern_text = ", ".join("rest" if n < 0 else str(n) for n in patterns[i])
        intervals = ", ".join(str(x) for x in scales[scale_of[i]])
        lines.append(f"| {season} | {cell(title)} | {roots[i]} | {intervals} | {pattern_text} | Clear all 3 {season.title()} campaign levels. |")
    lines += ["", "### Procedural sound effects", "", "SFX are generated oscillator/noise sweeps; `noise=0` is tonal, `noise=1` is noise, and `noise=2` mixes both. Values are source synthesis parameters (start/end frequency in Hz, duration in seconds, linear amplitude, noise mode).", "",
              "| SFX | Start Hz | End Hz | Duration | Amplitude | Sound source |", "|---|---:|---:|---:|---:|---|"]
    noise_types = {"0": "tonal", "1": "noise", "2": "mixed noise + tone"}
    if len(sfx_names) != len(defs):
        raise RuntimeError(f"SFX table mismatch: {len(sfx_names)} names vs {len(defs)} synthesis rows")
    for name, row in zip(sfx_names, defs):
        vals = [x.strip().rstrip("f") for x in row.split(",")]
        lines.append(f"| {name} | {vals[0]} | {vals[1]} | {vals[2]} s | {vals[3]} | {noise_types.get(vals[4], vals[4])} |")
    lines.append("")


def bestiary(lines: list[str]) -> None:
    enemies = D["EN"]
    beh = gd.BEH
    lines += ["## Bestiary", "", f"There are **{len(enemies)} enemy types**. Values are base data before wave scaling. `Behavior` combines the source special label with the implemented behavior flags.", "",
              "| # | Enemy (EN / PT-BR) | Season | HP | Contact/projectile damage | Speed | Attack rate | Range | Description (EN / PT-BR) | Special / behavior (EN / PT-BR) | Lore (EN / PT-BR) |",
              "|---:|---|---|---:|---:|---|---|---|---|---|---|"]
    for i, enemy in enumerate(enemies):
        key = enemy["key"]
        pt_name = local_enemy(enemy, "pt-br", "n")
        pt_desc = local_enemy(enemy, "pt-br", "d")
        pt_special = local_enemy(enemy, "pt-br", "sp")
        en_beh = enemy["sp"]
        flags = ", ".join(beh.get(key, []))
        behavior = en_beh + (f"; {flags}" if flags else "")
        pt_lore = lore("enemies", key, "pt-br")
        lines.append(f"| {i} | **{cell(enemy['name'])}** / {cell(pt_name)} | {cell(enemy['biome'])} | {enemy['hp']} | {enemy['dmg']} | {cell(enemy['spd'])} | {cell(enemy['atk'])} | {cell(enemy['range'])} | {cell(enemy['desc'])} / {cell(pt_desc)} | {cell(behavior)} / {cell(pt_special)} | {cell(lore('enemies', key, 'en'))} / {cell(pt_lore)} |")
    lines.append("")


def seasons(lines: list[str]) -> None:
    envs = D["ENVS"]
    lines += ["## Seasons, campaigns, and arenas", "", "Campaign seasons each contain three levels with wave quotas of 10, 15, and 20. Completing the prior season unlocks the next; completing the prior level unlocks the next level in that season.", "",
              "| Season | Elements | Gimmick (EN / PT-BR) | Level 1 | Level 2 | Level 3 | All-Seasons availability |", "|---|---|---|---|---|---|---|"]
    for sea_i, env in enumerate(envs):
        season = env["n"].lower()
        elems = " / ".join(f"{tr('el_' + e, 'en')} ({tr('el_' + e, 'pt-br')})" for e in env["el"])
        gim_key = env.get("gim", "gim_" + season)
        gim_en, gim_pt = tr(gim_key, "en"), tr(gim_key, "pt-br")
        lvls=[]
        for lv in range(1, 4):
            key=f"lvl_{sea_i+1}_{lv}"
            lvls.append(f"{tr(key,'en')} / {tr(key,'pt-br')}")
        lines.append(f"| {tr('s_' + season, 'en')} / {tr('s_' + season, 'pt-br')} | {cell(elems)} | {cell(gim_en)} / {cell(gim_pt)} | {cell(lvls[0])} (10 waves) | {cell(lvls[1])} (15 waves) | {cell(lvls[2])} (20 waves) | All-Seasons arena: clear all 15 campaign levels across the five seasons. |")
    lines.append("")


def achievements_and_cosmetics(lines: list[str]) -> None:
    game = (ROOT / "source" / "game.cpp").read_text(encoding="utf-8")
    data_static = (ROOT / "source" / "data_static.cpp").read_text(encoding="utf-8")
    name_keys = [x[2:].lower() for x in get_array(game, "ACH_NAME")]
    desc_keys = [x[2:].lower() for x in get_array(game, "ACH_DESC")]
    if len(name_keys) != len(desc_keys):
        raise RuntimeError("Achievement name/description arrays do not align")
    rewards = {
        "ach_batman": "NIGHT COWL (parody headwear)", "ach_joker": "GRINNING MASK (parody headwear)",
        "ach_terraria": "PIXEL MINER HELM (parody headwear)", "ach_mario": "RED PLUMBER CAP (parody headwear)",
        "ach_iron_man": "ARC HELMET (parody headwear)", "ach_avatar": "AIR NOMAD AVATAR outfit",
        "ach_season_forest": "Unlocks the Forest music track and next season",
        "ach_season_desert": "Unlocks the Desert music track and next season",
        "ach_season_snow": "Unlocks the Snow music track and next season",
        "ach_season_mountain": "Unlocks the Mountain music track and next season",
        "ach_season_eclipse": "Unlocks the Eclipse music track",
        "ach_full_catalog": "Master-catalog achievement; full discovery also satisfies the Iron Man outfit requirement",
    }
    lines += ["## Achievements", "", f"There are **{len(name_keys)} achievements**. Exact completion criteria are taken from the localized descriptions; rewards are listed where applicable.", "",
              "| # | Achievement (EN / PT-BR) | Requirement (EN / PT-BR) | Reward / note |", "|---:|---|---|---|"]
    for i, (name, desc) in enumerate(zip(name_keys, desc_keys)):
        en_req, pt_req = tr(desc, "en"), tr(desc, "pt-br")
        reward = rewards.get(name, "Achievement only; no cosmetic reward.")
        lines.append(f"| {i+1} | **{cell(tr(name, 'en'))}** / {cell(tr(name, 'pt-br'))} | {cell(en_req)} / {cell(pt_req)} | {cell(reward)} |")

    lines += ["", "## Customization", "", "Hats/outfits, faces, and colors are cosmetic only. There are 11 headwear/outfit options, six faces, and six colors. Ordinary options are available immediately; special parody outfits and Avatar require achievements.", ""]
    for title, array, noun in (("Headwear and outfits", "HAT_NAME", "headwear"), ("Faces", "FACE_NAME", "face"), ("Colors", "COLOR_NAME", "color")):
        keys=[x[2:].lower() for x in get_array(data_static, array)]
        lines += [f"### {title}", "", "| # | Option (EN / PT-BR) | Unlock |", "|---:|---|---|"]
        for i, key in enumerate(keys):
            if array == "HAT_NAME" and i < 5:
                unlock="Available immediately"
            elif array == "HAT_NAME" and i < 10:
                unlock="Achievement reward; see the matching achievement above"
            elif array == "HAT_NAME":
                unlock="Avatar achievement"
            else:
                unlock="Available immediately"
            lines.append(f"| {i} | {cell(tr(key,'en'))} / {cell(tr(key,'pt-br'))} | {cell(unlock)} |")
        lines.append("")


def gameplay_reference(lines: list[str]) -> None:
    lines += ["## Player stats and progression", "", "Base player values and per-point benefits are shown below. A point bonus is additive to the applicable derived multiplier; internal safety clamps still apply.", "",
              "| Stat | Base value | Effect per allocated point |", "|---|---:|---|",
              "| Strength | 8 | +8% damage and +2% projectile size |",
              "| Resistance | 5 | +6 maximum HP and 2% less damage taken |",
              "| Speed | 4 | +4% movement speed and +4% attack speed |",
              "| Luck | 3 | +3 percentage points critical chance, +8% XP, +2% rare-spell bonus |",
              "| Intelligence | 5 | -4% cooldown and +6% area-of-effect size |", "",
              "Base HP is 60. The starting movement speed is 62 px/s. Attacks automatically target enemies; move and dodge, collect XP, spend stat points, and build a spell loadout. The maximum spell-slot count is 12, including the starting weapon slot.", "",
              "### Content and unlock summary", "",
              "- **Six elements:** Water, Fire, Earth, Air, Blood, and Lightning.",
              "- **Five campaign seasons / 15 named levels**, plus the All-Seasons arena unlocked by clearing Eclipse 1–3.",
              "- **Five procedural catalog music tracks**, unlocked by completing their corresponding season; a separate menu pattern is always internal.",
              "- **Nine weapons**, with Machine Gun gated behind full campaign completion across all five seasons.",
              "- **25 enemy types**, including bosses, seasonal behaviors, and special attack patterns.",
              "- **Achievements** and catalog discovery cover season clears, kill milestones, time played, spells/enemies discovered, and cosmetics.",
              "- **Save data** preserves unlocks and discovery; the current v5 format migrates older v1–v4 files.", "",
              "### Localization and scope notes", "",
              "The game supports English and Brazilian Portuguese. CATALOG.md lists source-defined names, descriptions, stat strings, lore, effect payloads, unlocks, and combat values. Portuguese fields without a distinct translation are shown with the English fallback used by the runtime.",
              "Some displayed spell names are not unique; use the numbered entry plus its scope/season to identify the exact spell. Source data is authoritative when a marketing summary and an implementation detail differ.", ""]


def main() -> None:
    lines = ["# Abyssal Eclipse — Complete Content Catalog", "",
             "> Generated from `tools/data/mock_data.json`, `tools/gen_data.py`, runtime balance tables, and localization sources. Regenerate with `python3 tools/gen_catalog.py`.", "",
             "This reference covers the current game content and documented mechanics: spells and rank caps, weapons, music and SFX, enemies, seasons and campaign levels, achievements, cosmetics, and player progression. Names and text are shown in English and Brazilian Portuguese where available.", "",
             "## At a glance", "",
             f"- **{len(D['SPELLS'])} spell entries**; six are capped at Rank 5 for orbital/summoned entities, Time-Stop caps at Rank 9, and other spells cap at Rank {gd.ABSOLUTE_MAX_SPELL_LEVEL}.",
             f"- **{len(D['EN'])} enemy types**, **9 weapons**, **5 seasons**, **15 campaign levels**, **5 catalog music tracks**, and **{len(get_array((ROOT / 'source' / 'game.cpp').read_text(encoding='utf-8'), 'ACH_NAME'))} achievements**.",
             "- Cosmetic selection: 11 headwear/outfit entries, six faces, and six colors.", "",
             "## Contents", "", "1. [Spells](#spells)", "2. [Weapons](#weapons)", "3. [Music and sound effects](#music-and-sound-effects)", "4. [Bestiary](#bestiary)", "5. [Seasons and campaigns](#seasons-campaigns-and-arenas)", "6. [Achievements](#achievements)", "7. [Customization](#customization)", "8. [Stats and progression](#player-stats-and-progression)", ""]
    spell_sections(lines)
    weapon_section(lines)
    music_and_sfx(lines)
    bestiary(lines)
    seasons(lines)
    achievements_and_cosmetics(lines)
    gameplay_reference(lines)
    target = ROOT / "CATALOG.md"
    target.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {target} ({len(lines)} lines, {len(D['SPELLS'])} spells, {len(D['EN'])} enemies)")


if __name__ == "__main__":
    main()
