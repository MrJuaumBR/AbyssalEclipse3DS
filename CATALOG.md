# Abyssal Eclipse — Complete Content Catalog

> Generated from `tools/data/mock_data.json`, `tools/gen_data.py`, runtime balance tables, and localization sources. Regenerate with `python3 tools/gen_catalog.py`.

This reference covers the current game content and documented mechanics: spells and rank caps, weapons, music and SFX, enemies, seasons and campaign levels, achievements, cosmetics, and player progression. Names and text are shown in English and Brazilian Portuguese where available.

## At a glance

- **147 spell entries**; six are capped at Rank 5 for orbital/summoned entities, Time-Stop caps at Rank 9, and other spells cap at Rank 10.
- **25 enemy types**, **9 weapons**, **5 seasons**, **15 campaign levels**, **5 catalog music tracks**, and **22 achievements**.
- Cosmetic selection: 11 headwear/outfit entries, six faces, and six colors.

## Contents

1. [Spells](#spells)
2. [Weapons](#weapons)
3. [Music and sound effects](#music-and-sound-effects)
4. [Bestiary](#bestiary)
5. [Seasons and campaigns](#seasons-campaigns-and-arenas)
6. [Achievements](#achievements)
7. [Customization](#customization)
8. [Stats and progression](#player-stats-and-progression)

## Spells

There are **147 spell entries**. The table keeps the source-data index because **MIRAGE appears twice** (a global air spell and a Desert-exclusive spell); their effect payloads, scopes, and rank limits distinguish them.

### Rank limits and progression

Every spell can be upgraded one rank at a time, up to its own cap. The absolute maximum is **10**. Effects using generic numeric scaling gain **+20% per rank above Rank 1**; timing, trigger frequency, entity counts, and other custom parameters follow their spell-specific formulas.

These six orbiting/summoned entity spells deliberately stop at **Rank 5** to prevent runaway entity counts: **FLAME WHEEL**, **BALL LIGHTNING**, **KNIFE CIRCLE**, **SENTINEL**, **SENTINEL DRONE**, and **FAIRY COMPANION**. **TIME-STOP** caps at Rank 9 because its interval, duration, and cooldown reach their tuned limits by then. Other spells are capped at Rank 10. ELEMENTALIST CYCLE is a periodic elemental attack, not a persistent summoned entity, so it retains the higher cap.

Owned spells are offered as upgrades only when the 12-slot loadout is full; upgrade offers exclude spells already at their individual cap.

### Full spell reference

Effect payloads use internal effect names and source parameters, useful for distinguishing same-named entries and understanding the implementation. Lore is included when present in the source; `—` means no separate lore text is authored. PT-BR text falls back to English for any untranslated field.

| # | Spell (EN / PT-BR) | Element | Type / activation | Rarity | Scope / season | Max rank | Effect payload | Base stats (EN / PT-BR) | Description (EN / PT-BR) | Lore (EN / PT-BR) |
|---:|---|---|---|---|---|---:|---|---|---|---|
| 0 | **IRON WILL** / VONTADE DE FERRO | EARTH / TERRA | STATUS / SELF | COMMON | GLOBAL | 10 | `MAXHP(12)` | +12 HP / +12 VIDA | +12 MAX HP. / +12 DE VIDA MÁXIMA. | A horseshoe nailed above the door of a house that never fell. / Uma ferradura pregada sobre a porta de uma casa que nunca caiu. |
| 1 | **QUICKSTEP** / PASSO LIGEIRO | AIR / AR | STATUS / SELF | COMMON | GLOBAL | 10 | `MOVE_PCT(8)` | +8% SPD / +8% VEL. | +8% MOVE SPEED. / +8% DE VELOCIDADE DE MOVIMENTO. | A note sewn into the sole: not responsible for where these take you. / Um bilhete costurado na sola: não nos responsabilizamos por aonde estas botas levam você. |
| 2 | **FOCUS STONE** / PEDRA DE FOCO | LIGHTNING / RAIO | STATUS / SELF | COMMON | GLOBAL | 10 | `CD_PCT(10)` | -10% CD / -10% REC. | -10% ATTACK COOLDOWN. / -10% DE RECARGA DO ATAQUE. | Smooth as a held breath, and just as patient. / Lisa como um fôlego contido, e igualmente paciente. |
| 3 | **LUCKY FANG** / PRESA DA SORTE | AIR / AR | DAMAGE / TOUCH | COMMON | GLOBAL | 10 | `CRIT(8)` | +8% CRIT / +8% CRÍT. | +8% CRIT CHANCE. / +8% DE CHANCE DE CRÍTICO. | Pulled from a wolf that outlived three hunters and one very confident bard. / Tirada de um lobo que sobreviveu a três caçadores e a um bardo muito confiante. |
| 4 | **FLINT SHARD** / LASCA DE SÍLEX | FIRE / FOGO | DAMAGE / TOUCH | COMMON | GLOBAL | 10 | `FLATDMG(2)` | +2 DMG / +2 DANO | +2 FLAT DAMAGE PER THROW. / +2 DE DANO FIXO POR ARREMESSO. | The first tool any hand ever learned to throw angrily. / A primeira ferramenta que uma mão aprendeu a arremessar com raiva. |
| 5 | **WIND CHIME** / SINO DE VENTO | AIR / AR | STATUS / AREA OF EFFECT | COMMON | GLOBAL | 10 | `AURA(0, 48, 10)` | -10% SPD; 48PX / -10% VEL.; 48PX | NEARBY ENEMIES SLOWED 10%. / INIMIGOS PRÓXIMOS FICAM 10% MAIS LENTOS. | Hung in a doorway that opens onto nowhere in particular. / Pendurado numa porta que abre para lugar nenhum em particular. |
| 6 | **SAP GLAND** / GLÂNDULA DE SEIVA | EARTH / TERRA | STATUS / SELF | COMMON | GLOBAL | 10 | `KILLHEAL(1, 2)` | +1 HP/KILL / +1 VIDA/MORTE | KILLS HEAL 1 HP. MAX 1 PER 2S. / MATAR CURA 1 DE VIDA. MÁX. 1 A CADA 2S. | Cut from a tree that was never quite sure it was a tree. / Cortada de uma árvore que nunca teve certeza de ser uma árvore. |
| 7 | **EMBER CORE** / NÚCLEO DE BRASA | FIRE / FOGO | DAMAGE / AREA OF EFFECT | COMMON | GLOBAL | 10 | `KILL_BURST(8, 40)` | 8 DMG; 40PX / 8 DANO; 40PX | ON KILL, A SMALL FIRE BURST. / AO MATAR, UMA PEQUENA EXPLOSÃO DE FOGO. | A cinder still warm from the heart of a fallen fire elemental. / Uma brasa ainda quente do coração de um elemental do fogo caído. |
| 8 | **SPARK TIP** / PONTA FAÍSCA | LIGHTNING / RAIO | DAMAGE / TOUCH | COMMON | GLOBAL | 10 | `HIT_BONUS(15, 3)` | 15%; +3 DMG / 15%; +3 DANO | 15% CHANCE ON HIT FOR +3 LIGHTNING DMG. / 15% DE CHANCE AO ACERTAR DE +3 DE DANO DE RAIO. | Struck by lightning exactly once, and has held a grudge ever since. / Levou um raio exatamente uma vez e guarda rancor desde então. |
| 9 | **TOXIC SPORE** / ESPORO TÓXICO | WATER / ÁGUA | STATUS / AREA OF EFFECT | COMMON | GLOBAL | 10 | `KILL_POISON(5, 3, 40)` | 5 DMG; 3S / 5 DANO; 3S | ON KILL, POISON CLOUD DEALS 5 DMG OVER 3S. / AO MATAR, NUVEM DE VENENO CAUSA 5 DE DANO EM 3S. | It blooms politely, right up until it doesn't. / Floresce com educação, até o momento em que deixa de florescer. |
| 10 | **THORN MAIL** / COTA DE ESPINHOS | EARTH / TERRA | DAMAGE / SELF | RARE | GLOBAL | 10 | `REFLECT(40)` | 40% REFLECT / 40% REFLEXO | REFLECT 40% CONTACT DAMAGE. / REFLETE 40% DO DANO DE CONTATO. | Each barb was a rose once, before it learned what roses are for. / Cada espinho já foi uma rosa, antes de aprender para que servem as rosas. |
| 11 | **HUNTER'S EYE** / OLHO DO CAÇADOR | AIR / AR | DAMAGE / SELF | RARE | GLOBAL | 10 | `CRIT(15); CRITDMG(25)` | +15% CRIT; +25% CD / +15% CRÍT.; +25% D.CRÍT. | +15% CRIT CHANCE, +25% CRIT DMG. / +15% DE CRÍTICO, +25% DE DANO CRÍTICO. | Glass from the eye of a hunter who never missed and never smiled. / Vidro do olho de um caçador que nunca errou e nunca sorriu. |
| 12 | **STATIC FIELD** / CAMPO ESTÁTICO | LIGHTNING / RAIO | DAMAGE / AREA OF EFFECT | RARE | GLOBAL | 10 | `AURA(2, 48, 0)` | 1/0.5S; 48PX / 1/0,5S; 48PX | AURA DEALS 1 DMG EVERY 0.5S. / AURA CAUSA 1 DE DANO A CADA 0,5S. | The air around it crackles like a wool blanket's long-nursed revenge. / O ar ao redor estala como a vingança antiga de um cobertor de lã. |
| 13 | **ECHO CHAMBER** / CÂMARA DE ECO | AIR / AR | WEAPON / AREA OF EFFECT | RARE | GLOBAL | 10 | `NTH_RING(5, 12, 0.25)` | 5TH; 360° / 5º; 360° | EVERY 5TH THROW FIRES A FULL RING. / A CADA 5º ARREMESSO, DISPARA UM ANEL COMPLETO. | A bell with no clapper, answering things nobody rang. / Um sino sem badalo, respondendo a coisas que ninguém tocou. |
| 14 | **FROST LENS** / LENTE GÉLIDA | WATER / ÁGUA | WEAPON / TOUCH | RARE | GLOBAL | 10 | `HIT_FREEZE(20, 1)` | 20%; 1S / 20%; 1S | 20% CHANCE ON HIT TO FREEZE 1S. / 20% DE CHANCE DE CONGELAR POR 1S AO ACERTAR. | Whatever you look at through it forgets to blink. / Tudo que se vê através dela esquece de piscar. |
| 15 | **RICOCHET** / RICOCHE | AIR / AR | WEAPON / TOUCH | RARE | GLOBAL | 10 | `BOUNCE_ENEMY(1, 50)` | 1 BOUNCE; 50% DMG / 1 QUIQUE; 50% DANO | THROWS BOUNCE TO A NEARBY ENEMY. / ARREMESSOS QUICAM PARA UM INIMIGO PRÓXIMO. | — / — |
| 16 | **TIDAL FORCE** / FORÇA DA MARÉ | WATER / ÁGUA | WEAPON / TOUCH | EPIC | GLOBAL | 10 | `PIERCE(3)` | PIERCE 3 / PERFURA 3 | THROWS PIERCE UP TO 3 ENEMIES. / ARREMESSOS ATRAVESSAM ATÉ 3 INIMIGOS. | A wave that forgot to come back down. / Uma onda que esqueceu de voltar para baixo. |
| 17 | **PACT OF IRON** / PACTO DE FERRO | EARTH / TERRA | STATUS / SELF | EPIC | GLOBAL | 10 | `RES(3); MAXHP(25); MOVE_PCT(-15)` | +3 RES; +25 HP; -15% SPD / +3 RES; +25 VIDA; -15% VEL. | +3 RES, +25 HP, -15% MOVE SPD. / +3 RES, +25 VIDA, -15% DE VELOCIDADE. | Signed with a gauntlet, so the handshake would last. / Assinado com uma manopla, para o aperto de mão durar. |
| 18 | **ELEMENTAL CORE** / NÚCLEO ELEMENTAL | AIR / AR | WEAPON / SELF | EPIC | GLOBAL | 10 | `DMG_PCT(15)` | +15% DMG; 8S / +15% DANO; 8S | WEAPON CYCLES ELEMENT EVERY 8S, +15% DMG. / A ARMA TROCA DE ELEMENTO A CADA 8S, +15% DE DANO. | Six elements share this one stone and take turns being the loudest. / Seis elementos dividem esta pedra e se revezam para ser o mais barulhento. |
| 19 | **GRAVITY WELL** / POÇO DE GRAVIDADE | EARTH / TERRA | STATUS / AREA OF EFFECT | EPIC | GLOBAL | 10 | `AURA_PULL(64)` | PULL; 64PX / PUXA; 64PX | NEARBY ENEMIES ARE PULLED TOWARD YOU. / INIMIGOS PRÓXIMOS SÃO PUXADOS ATÉ VOCÊ. | — / — |
| 20 | **ECLIPSE CROWN** / COROA DO ECLIPSE | LIGHTNING / RAIO | STATUS / SELF | LEGENDARY | GLOBAL | 10 | `ALLNUM_PCT(30)` | +30% ALL / +30% TUDO | ALL NUMERIC SPELL EFFECTS +30%. / TODOS OS EFEITOS NUMÉRICOS +30%. | Worn by something that ruled before the sky was sorted into days. / Usada por algo que governava antes do céu ser dividido em dias. |
| 21 | **WORLD ENDER** / ACABA-MUNDOS | BLOOD / SANGUE | DAMAGE / AREA OF EFFECT | LEGENDARY | GLOBAL | 10 | `KILL_CHAIN(10)` | 10% CHAIN / 10% CADEIA | KILLS HAVE 10% CHANCE TO CHAIN-KILL A NEARBY ENEMY. / MORTES TÊM 10% DE CHANCE DE MATAR EM CADEIA. | Forged on the last day of an earlier world, and never put back. / Forjado no último dia de um mundo anterior e nunca guardado de volta. |
| 22 | **AQUA VEIL** / VÉU AQUÁTICO | WATER / ÁGUA | STATUS / SELF | COMMON | ELEMENT / WATER | 10 | `RES(2)` | +2 RES / +2 RES | +2 RES. / +2 DE RESISTÊNCIA. | — / — |
| 23 | **BRINE TOUCH** / TOQUE DE SALMOURA | WATER / ÁGUA | DAMAGE / TOUCH | COMMON | ELEMENT / WATER | 10 | `VS_SLOWED(10)` | +10% DMG / +10% DANO | +10% DMG VS SLOWED ENEMIES. / +10% DE DANO CONTRA INIMIGOS LENTOS. | — / — |
| 24 | **BUBBLE BURST** / EXPLOSÃO DE BOLHAS | WATER / ÁGUA | DAMAGE / AREA OF EFFECT | COMMON | ELEMENT / WATER | 10 | `HIT_BURST(6, 32)` | 6 DMG; 32PX / 6 DANO; 32PX | ON HIT, A SMALL WATER BURST. / AO ACERTAR, UMA PEQUENA EXPLOSÃO DE ÁGUA. | — / — |
| 25 | **SPLASH GUARD** / GUARDA DE RESPINGO | WATER / ÁGUA | STATUS / SELF | COMMON | ELEMENT / WATER | 10 | `TAKEN_PCT(-10)` | -10% TAKEN / -10% RECEBIDO | -10% DAMAGE TAKEN. / -10% DE DANO RECEBIDO. | — / — |
| 26 | **TIDE RING** / ANEL DA MARÉ | WATER / ÁGUA | STATUS / AREA OF EFFECT | RARE | ELEMENT / WATER | 10 | `AURA(0, 64, 25)` | -25% SPD; 64PX / -25% VEL.; 64PX | NEARBY ENEMIES SLOWED 25%. / INIMIGOS PRÓXIMOS FICAM 25% MAIS LENTOS. | Salt still gathers on it, a hundred miles from any sea. / O sal ainda se acumula nele, a cem milhas de qualquer mar. |
| 27 | **RIPTIDE** / CORRENTEZA | WATER / ÁGUA | WEAPON / TOUCH | RARE | ELEMENT / WATER | 10 | `KNOCKBACK(1)` | KNOCKBACK / EMPURRÃO | THROWS KNOCK ENEMIES BACK. / ARREMESSOS EMPURRAM OS INIMIGOS PARA TRÁS. | — / — |
| 28 | **FROSTBITE** / CONGELAMENTO | WATER / ÁGUA | DAMAGE / TOUCH | RARE | ELEMENT / WATER | 10 | `HIT_SLOW(20, 2)` | -20% SPD; 2S / -20% VEL.; 2S | HITS SLOW 20% FOR 2S. / ACERTOS DEIXAM 20% MAIS LENTO POR 2S. | — / — |
| 29 | **TIDAL SHIELD** / ESCUDO DE MARÉ | WATER / ÁGUA | STATUS / SELF | RARE | ELEMENT / WATER | 10 | `SHIELD_WAVE(20)` | 20 SHIELD / 20 ESCUDO | A SHIELD ABSORBS 20 DAMAGE EACH WAVE. / UM ESCUDO ABSORVE 20 DE DANO A CADA ONDA. | — / — |
| 30 | **UNDERTOW** / RESSACA | WATER / ÁGUA | STATUS / AREA OF EFFECT | RARE | ELEMENT / WATER | 10 | `KILL_SLOWAOE(30, 2, 48)` | -30% SPD; 2S / -30% VEL.; 2S | DEFEATED ENEMIES SLOW NEARBY FOES. / INIMIGOS DERROTADOS DEIXAM OS PRÓXIMOS LENTOS. | — / — |
| 31 | **DEEP CURRENT** / CORRENTE PROFUNDA | WATER / ÁGUA | DAMAGE / AREA OF EFFECT | EPIC | ELEMENT / WATER | 10 | `AURA(2, 56, 15)` | 2 DPS; -15% SPD / 2 DPS; -15% VEL. | AURA DEALS 2 DMG/S AND SLOWS 15%. / AURA CAUSA 2 DE DANO/S E DEIXA 15% MAIS LENTO. | — / — |
| 32 | **MIRROR POOL** / POÇO ESPELHADO | WATER / ÁGUA | STATUS / SELF | EPIC | ELEMENT / WATER | 10 | `MIRROR(20)` | 20% REFLECT / 20% REFLEXO | 20% CHANCE TO REFLECT A PROJECTILE. / 20% DE CHANCE DE REFLETIR UM PROJÉTIL. | — / — |
| 33 | **WHIRLPOOL** / REDEMOINHO | WATER / ÁGUA | WEAPON / AREA OF EFFECT | EPIC | ELEMENT / WATER | 10 | `WHIRL(2, 3)` | 3S; 2 DPS / 3S; 2 DPS | THROWS LEAVE A SPINNING VORTEX. / ARREMESSOS DEIXAM UM VÓRTICE GIRATÓRIO. | — / — |
| 34 | **RAIN DANCE** / DANÇA DA CHUVA | WATER / ÁGUA | STATUS / SELF | EPIC | ELEMENT / WATER | 10 | `KILL_MOVE(2, 20)` | +2%/KILL; MAX 20% / +2%/MORTE; MÁX 20% | KILLS GRANT +2% MOVE SPEED. MAX +20%. / MORTES DÃO +2% DE VELOCIDADE. MÁX. +20%. | — / — |
| 35 | **PRESSURE POINT** / PONTO DE PRESSÃO | WATER / ÁGUA | DAMAGE / TOUCH | EPIC | ELEMENT / WATER | 10 | `CRITBONUS(40)` | +40% CRIT / +40% CRÍT. | CRITS DEAL +40% DAMAGE. / CRÍTICOS CAUSAM +40% DE DANO. | — / — |
| 36 | **ABYSSAL PACT** / PACTO ABISSAL | WATER / ÁGUA | STATUS / SELF | LEGENDARY | ELEMENT / WATER | 10 | `NTH_BLAST(10, 60, 110)` | 10TH; 60 DMG / 10º; 60 DANO | EVERY 10TH THROW IS A TIDAL WAVE. / A CADA 10º ARREMESSO, UMA ONDA GIGANTE. | The sea offered a hand. Something in the dark shook it. / — |
| 37 | **EMBER TRAIL** / RASTRO DE BRASA | FIRE / FOGO | DAMAGE / AREA OF EFFECT | COMMON | ELEMENT / FIRE | 10 | `TRAIL_FIRE(4, 1)` | 4 DPS; 1S / 4 DPS; 1S | YOU LEAVE BURNING FOOTPRINTS. / VOCÊ DEIXA PEGADAS EM CHAMAS. | — / — |
| 38 | **BLAZING FANG** / PRESA FLAMEJANTE | FIRE / FOGO | DAMAGE / TOUCH | COMMON | ELEMENT / FIRE | 10 | `VS_BURNING(12)` | +12% DMG / +12% DANO | +12% DMG VS BURNING ENEMIES. / +12% DE DANO CONTRA INIMIGOS EM CHAMAS. | — / — |
| 39 | **ASHEN WAKE** / ESTEIRA DE CINZAS | FIRE / FOGO | STATUS / SELF | COMMON | ELEMENT / FIRE | 10 | `KILL_CLOUD(3, 2, 40)` | 3 DPS; 2S / 3 DPS; 2S | KILLS LEAVE A BURNING CLOUD. / MORTES DEIXAM UMA NUVEM EM CHAMAS. | — / — |
| 40 | **SCORCH** / CHAMUSCAR | FIRE / FOGO | WEAPON / TOUCH | COMMON | ELEMENT / FIRE | 10 | `HIT_BURN(2, 2)` | 2 DPS; 2S / 2 DPS; 2S | THROWS BURN FOR 2S. / ARREMESSOS QUEIMAM POR 2S. | — / — |
| 41 | **PYRE** / PIRE | FIRE / FOGO | DAMAGE / AREA OF EFFECT | COMMON | ELEMENT / FIRE | 10 | `KILL_EXPLODE_BURNING(8, 40)` | 8 DMG; 40PX / 8 DANO; 40PX | DEFEATED BURNING ENEMIES EXPLODE. / INIMIGOS EM CHAMAS DERROTADOS EXPLODEM. | — / — |
| 42 | **CINDER CLOAK** / MANTO DE CINZAS | FIRE / FOGO | STATUS / SELF | RARE | ELEMENT / FIRE | 10 | `BURN_TOUCH(3, 3)` | 3 DPS; 3S / 3 DPS; 3S | ENEMIES THAT TOUCH YOU BURN 3S. / QUEM TE TOCA QUEIMA POR 3S. | — / — |
| 43 | **FLAME WHEEL** / RODA DE FOGO | FIRE / FOGO | WEAPON / AREA OF EFFECT | RARE | ELEMENT / FIRE | 5 | `ORBIT(5, 2)` | ORBIT; 5 DPS / ÓRBITA; 5 DPS | THROWS ORBIT AROUND YOU. / ARREMESSOS ORBITAM AO SEU REDOR. | — / — |
| 44 | **HEAT SHOCK** / ONDA DE CALOR | FIRE / FOGO | DAMAGE / AREA OF EFFECT | RARE | ELEMENT / FIRE | 10 | `KILL_BURST(6, 48)` | 6 DMG; 48PX / 6 DANO; 48PX | ON KILL, NEARBY ENEMIES TAKE 6 DMG. / AO MATAR, INIMIGOS PRÓXIMOS TOMAM 6 DE DANO. | — / — |
| 45 | **SUNSPOT** / MANCHA SOLAR | FIRE / FOGO | DAMAGE / AREA OF EFFECT | RARE | ELEMENT / FIRE | 10 | `AURA(1, 40, 0)` | 1 DPS; 40PX / 1 DPS; 40PX | AURA DEALS 1 DMG/S. / AURA CAUSA 1 DE DANO/S. | — / — |
| 46 | **WARMTH** / CALOR | FIRE / FOGO | STATUS / SELF | RARE | ELEMENT / FIRE | 10 | `WARMTH(15)` | -15% TAKEN / -15% RECEBIDO | TAKE 15% LESS DAMAGE WHILE BURNING. / TOME 15% MENOS DANO ENQUANTO QUEIMA. | — / — |
| 47 | **MOLTEN CORE** / NÚCLEO FUNDIDO | FIRE / FOGO | WEAPON / SELF | EPIC | ELEMENT / FIRE | 10 | `EXPLODE(10, 40)` | 10 DMG; 40PX / 10 DANO; 40PX | THROWS EXPLODE ON IMPACT. / ARREMESSOS EXPLODEM NO IMPACTO. | — / — |
| 48 | **INFERNO BRAND** / MARCA INFERNAL | FIRE / FOGO | DAMAGE / TOUCH | EPIC | ELEMENT / FIRE | 10 | `CRIT_BURN(4, 3)` | 4 DPS; 3S / 4 DPS; 3S | CRITS SET THE TARGET ABLAZE. / CRÍTICOS INCENDEIAM O ALVO. | — / — |
| 49 | **FIRESTORM** / TEMPESTADE DE FOGO | FIRE / FOGO | WEAPON / AREA OF EFFECT | EPIC | ELEMENT / FIRE | 10 | `NTH_BLAST(6, 8, 64)` | 8 DMG; 64PX / 8 DANO; 64PX | EVERY 6TH THROW CALLS A FIRE STORM. / A CADA 6º ARREMESSO, UMA TEMPESTADE. | — / — |
| 50 | **SOLAR FLARE** / ERUPÇÃO SOLAR | FIRE / FOGO | DAMAGE / AREA OF EFFECT | EPIC | ELEMENT / FIRE | 10 | `PERIODIC_FLARE(8, 10)` | 10 DMG; 8S / 10 DANO; 8S | EVERY 8S, A FLARE BURNS THE WHOLE SCREEN. / A CADA 8S, UMA EXPLOSÃO QUEIMA A TELA TODA. | — / — |
| 51 | **PHOENIX DOWN** / RENASCER DA FÊNIX | FIRE / FOGO | STATUS / SELF | LEGENDARY | ELEMENT / FIRE | 10 | `REVIVE(30)` | 1 REVIVE; 30% HP / 1 RENASCER; 30% VIDA | ONCE PER RUN, REVIVE WITH 30% HP. / UMA VEZ POR PARTIDA, RESSUSCITE COM 30% DE VIDA. | A single feather that refuses to accept the ending. / — |
| 52 | **STONE SKIN** / PELE DE PEDRA | EARTH / TERRA | STATUS / SELF | COMMON | ELEMENT / EARTH | 10 | `RES(2); MOVE_PCT(-5)` | +2 RES; -5% SPD / +2 RES; -5% VEL. | +2 RES, -5% MOVE SPEED. / +2 RES, -5% DE VELOCIDADE. | — / — |
| 53 | **ROOT SNARE** / RAÍZES PRENSORAS | EARTH / TERRA | WEAPON / TOUCH | COMMON | ELEMENT / EARTH | 10 | `HIT_ROOT(1)` | ROOT 1S / PRENDE 1S | HITS ROOT ENEMIES FOR 1S. / ACERTOS PRENDEM O INIMIGO POR 1S. | — / — |
| 54 | **DUST CLOUD** / NUVEM DE POEIRA | EARTH / TERRA | STATUS / AREA OF EFFECT | COMMON | ELEMENT / EARTH | 10 | `KILL_SLOWAOE(20, 2, 48)` | -20% ATK; 48PX / -20% ATQ; 48PX | ON KILL, DUST SLOWS NEARBY FOES. / AO MATAR, POEIRA DEIXA INIMIGOS MAIS LENTOS. | — / — |
| 55 | **MOSS CLOAK** / MANTO DE MUSGO | EARTH / TERRA | STATUS / SELF | COMMON | ELEMENT / EARTH | 10 | `REGEN(1, 8)` | +1/8S / +1/8S | REGEN 1 HP EVERY 8S. / REGENERA 1 DE VIDA A CADA 8S. | Damp, soft, and quietly proud of its own patience. / Úmido, macio e discretamente orgulhoso da própria paciência. |
| 56 | **BOULDER TOSS** / ARREMESSO DE ROCHA | EARTH / TERRA | WEAPON / TOUCH | RARE | ELEMENT / EARTH | 10 | `DMG_PCT(25); ATKSPD_PCT(-15)` | +25% DMG; -15% ATK / +25% DANO; -15% ATQ | +25% DMG, -15% ATTACK SPEED. / +25% DE DANO, -15% DE VELOCIDADE DE ATAQUE. | — / — |
| 57 | **ECHO STONE** / PEDRA DO ECO | EARTH / TERRA | WEAPON / TOUCH | RARE | ELEMENT / EARTH | 10 | `BOUNCE_WALL(2)` | 2 BOUNCES / 2 QUIQUES | THROWS BOUNCE OFF WALLS. / ARREMESSOS QUICAM NAS PAREDES. | — / — |
| 58 | **TREMBLE** / TREMOR | EARTH / TERRA | DAMAGE / AREA OF EFFECT | RARE | ELEMENT / EARTH | 10 | `NTH_STUN(5, 0.5, 48)` | STUN .5S; 48PX / ATORDOA 0,5S; 48PX | EVERY 5TH THROW SHAKES THE GROUND. / A CADA 5º ARREMESSO, O CHÃO TREME. | — / — |
| 59 | **STURDY FRAME** / ESTRUTURA ROBUSTA | EARTH / TERRA | STATUS / SELF | RARE | ELEMENT / EARTH | 10 | `DEBUFF_PCT(-30)` | -30% DEBUFF / -30% EFEITOS | SLOWS AND KNOCKBACK LAST 30% LESS. / LENTIDÃO E EMPURRÃO DURAM 30% MENOS. | — / — |
| 60 | **CLAY GUARD** / GUARDA DE ARGILA | EARTH / TERRA | STATUS / SELF | RARE | ELEMENT / EARTH | 10 | `BLOCK_WAVE(1)` | 1 BLOCK / 1 BLOQUEIO | A CLAY FAMILIAR BLOCKS 1 HIT PER WAVE. / UM FAMILIAR BLOQUEIA 1 GOLPE POR ONDA. | — / — |
| 61 | **GAIA'S GIFT** / PRESENTE DE GAIA | EARTH / TERRA | STATUS / SELF | EPIC | ELEMENT / EARTH | 10 | `MAXHP(40); MOVE_PCT(-10)` | +40 HP; -10% SPD / +40 VIDA; -10% VEL. | +40 MAX HP, -10% MOVE SPEED. / +40 DE VIDA MÁXIMA, -10% DE VELOCIDADE. | — / — |
| 62 | **SPLIT STONE** / PEDRA DIVIDIDA | EARTH / TERRA | WEAPON / TOUCH | EPIC | ELEMENT / EARTH | 10 | `SPLIT(3, 50)` | 3 SHARDS; 50% DMG / 3 ESTILHAÇOS; 50% DANO | THROWS SPLIT INTO 3 SHARDS. / ARREMESSOS SE DIVIDEM EM 3 ESTILHAÇOS. | — / — |
| 63 | **AVALANCHE** / AVALANCHE | EARTH / TERRA | WEAPON / AREA OF EFFECT | EPIC | ELEMENT / EARTH | 10 | `NTH_BLAST(8, 12, 56)` | 12 DMG; 56PX / 12 DANO; 56PX | EVERY 8TH THROW DROPS BOULDERS. / A CADA 8º ARREMESSO, PEDRAS CAEM DO CÉU. | — / — |
| 64 | **MOUNTAIN'S WILL** / VONTADE DA MONTANHA | EARTH / TERRA | STATUS / SELF | EPIC | ELEMENT / EARTH | 10 | `RES(3); MAXHP(20)` | +3 RES; +20 HP / +3 RES; +20 VIDA | +3 RES, +20 HP. / +3 RES, +20 DE VIDA. | — / — |
| 65 | **PETRIFY** / PETRIFICAR | EARTH / TERRA | WEAPON / TOUCH | LEGENDARY | ELEMENT / EARTH | 10 | `HIT_STUN(5, 2)` | 5%; 2S STUN / 5%; 2S | 5% CHANCE TO STUN A FOE FOR 2S. / 5% DE CHANCE DE ATORDOAR POR 2S. | Medusa's lesser-known cousin, sold by the ounce. / — |
| 66 | **TECTONIC** / TECTÔNICO | EARTH / TERRA | DAMAGE / AREA OF EFFECT | LEGENDARY | ELEMENT / EARTH | 10 | `NTH_BLAST(12, 20, 80)` | 20 DMG; 80PX / 20 DANO; 80PX | EVERY 12TH THROW SPLITS THE GROUND. / A CADA 12º ARREMESSO, O CHÃO SE ABRE. | A fragment of a fault line that still remembers the argument. / — |
| 67 | **GALE STEP** / PASSO VENTOSO | AIR / AR | STATUS / SELF | COMMON | ELEMENT / AIR | 10 | `MOVE_PCT(12)` | +12% SPD / +12% VEL. | +12% MOVE SPEED. / +12% DE VELOCIDADE DE MOVIMENTO. | — / — |
| 68 | **FEATHER FALL** / QUEDA DE PLUMA | AIR / AR | STATUS / SELF | COMMON | ELEMENT / AIR | 10 | `RANGED_TAKEN_PCT(-20)` | -20% RANGED / -20% DISTÂNCIA | -20% DAMAGE FROM RANGED ATTACKS. / -20% DE DANO DE ATAQUES À DISTÂNCIA. | — / — |
| 69 | **ZEPHYR BLADE** / LÂMINA ZÉFIR | AIR / AR | WEAPON / TOUCH | COMMON | ELEMENT / AIR | 10 | `THROWSPD_PCT(10)` | +10% SPD / +10% VEL. | +10% THROW SPEED. / +10% DE VELOCIDADE DE ARREMESSO. | — / — |
| 70 | **WHISPER** / SUSSURRO | AIR / AR | STATUS / SELF | COMMON | ELEMENT / AIR | 10 | `CRIT(5); MOVE_PCT(5)` | +5% CRIT; +5% SPD / +5% CRÍT.; +5% VEL. | +5% CRIT, +5% MOVE SPEED. / +5% DE CRÍTICO, +5% DE VELOCIDADE. | — / — |
| 71 | **CYCLONE** / CICLONE | AIR / AR | WEAPON / AREA OF EFFECT | RARE | ELEMENT / AIR | 10 | `HOMING(1)` | HOMING / GUIADO | THROWS CURVE TOWARD THE NEAREST FOE. / ARREMESSOS CURVAM ATÉ O INIMIGO MAIS PRÓXIMO. | — / — |
| 72 | **SONIC CRACK** / ESTOURO SÔNICO | AIR / AR | DAMAGE / TOUCH | RARE | ELEMENT / AIR | 10 | `HIT_STUN(10, 0.5)` | 10%; 0.5S / 10%; 0,5S | HITS HAVE 10% CHANCE TO STUN. / ACERTOS TÊM 10% DE CHANCE DE ATORDOAR. | — / — |
| 73 | **TAILWIND** / VENTO A FAVOR | AIR / AR | STATUS / SELF | RARE | ELEMENT / AIR | 10 | `ATKSPD_PCT(15)` | +15% ATK / +15% ATQ | +15% ATTACK SPEED. / +15% DE VELOCIDADE DE ATAQUE. | — / — |
| 74 | **VACUUM** / VÁCUO | AIR / AR | STATUS / AREA OF EFFECT | RARE | ELEMENT / AIR | 10 | `AURA_WEAKEN(15, 56)` | -15% DMG; 56PX / -15% DANO; 56PX | NEARBY ENEMIES DEAL 15% LESS DAMAGE. / INIMIGOS PRÓXIMOS CAUSAM 15% MENOS DANO. | — / — |
| 75 | **MIRAGE** / MIRAGEM | AIR / AR | STATUS / SELF | RARE | ELEMENT / AIR | 10 | `DODGE(10)` | 10% DODGE / 10% ESQUIVA | 10% CHANCE TO DODGE A HIT. / 10% DE CHANCE DE ESQUIVAR. | — / — |
| 76 | **UPDRAFT** / CORRENTE ASCENDENTE | AIR / AR | WEAPON / TOUCH | EPIC | ELEMENT / AIR | 10 | `PIERCE(2); THROWSPD_PCT(20)` | PIERCE 2; +20% SPD / PERFURA 2; +20% VEL. | THROWS PIERCE 2 ENEMIES AND SPEED UP. / ARREMESSOS PERFURAM 2 INIMIGOS E ACELERAM. | — / — |
| 77 | **SHATTER WIND** / VENTO QUEBRANTE | AIR / AR | DAMAGE / TOUCH | EPIC | ELEMENT / AIR | 10 | `VS_FROZEN(20)` | +20% DMG / +20% DANO | +20% DMG VS FROZEN OR STUNNED. / +20% DE DANO CONTRA CONGELADOS OU ATORDOADOS. | — / — |
| 78 | **STORMBREAK** / QUEBRA-TEMPESTADE | AIR / AR | DAMAGE / AREA OF EFFECT | EPIC | ELEMENT / AIR | 10 | `NTH_BLAST(7, 10, 64)` | 10 DMG; 64PX / 10 DANO; 64PX | EVERY 7TH THROW RELEASES A GUST RING. / A CADA 7º ARREMESSO, UM ANEL DE VENTO. | — / — |
| 79 | **SKY DANCE** / DANÇA DO CÉU | AIR / AR | WEAPON / SELF | EPIC | ELEMENT / AIR | 10 | `NTH_DOUBLE(3)` | 3RD; 2X / 3º; 2X | EVERY 3RD THROW FIRES TWICE. / A CADA 3º ARREMESSO, DISPARA DUAS VEZES. | — / — |
| 80 | **TEMPEST** / TEMPESTADE | AIR / AR | WEAPON / AREA OF EFFECT | LEGENDARY | ELEMENT / AIR | 10 | `HIT_BOLT(8, 2)` | +8 DMG; CHAIN 2 / +8 DANO; CADEIA 2 | THROWS SUMMON LIGHTNING ON HIT. / ARREMESSOS INVOCAM RAIO AO ACERTAR. | Bottled wind from a storm that never finished its sentence. / — |
| 81 | **EYE OF THE STORM** / OLHO DA TEMPESTADE | AIR / AR | STATUS / SELF | LEGENDARY | ELEMENT / AIR | 10 | `ATKSPD_PCT(25); MOVE_PCT(10)` | +25% ATK; +10% MOVE / +25% ATQ; +10% MOV. | +25% ATTACK SPEED, +10% MOVE SPEED. / +25% DE VELOCIDADE DE ATAQUE, +10% DE MOVIMENTO. | The calm at the centre, kept in a jar against better advice. / — |
| 82 | **BLOOD PEBBLE** / SEIXO DE SANGUE | BLOOD / SANGUE | STATUS / SELF | COMMON | ELEMENT / BLOOD | 10 | `LIFESTEAL(1)` | 1% STEAL / 1% ROUBO | 1% LIFESTEAL ON ALL DAMAGE. CAP 1 HP/S. / 1% DE ROUBO DE VIDA. LIMITE 1 DE VIDA/S. | A river stone that spent one very long night in a very red river. / Uma pedra de rio que passou uma noite muito longa num rio muito vermelho. |
| 83 | **THIRST** / SEDE | BLOOD / SANGUE | STATUS / SELF | COMMON | ELEMENT / BLOOD | 10 | `VS_WOUNDED(15)` | +15% DMG / +15% DANO | +15% DMG VS WOUNDED ENEMIES. / +15% DE DANO CONTRA INIMIGOS FERIDOS. | — / — |
| 84 | **BLOOD SIGIL** / SELO DE SANGUE | BLOOD / SANGUE | STATUS / SELF | RARE | ELEMENT / BLOOD | 10 | `DMG_PCT(30); MAXHP_PCT(-25)` | +30% DMG; -25% HP / +30% DANO; -25% VIDA | +30% DMG, -25% MAX HP. / +30% DE DANO, -25% DE VIDA MÁXIMA. | Signed in something redder than ink. / Assinado com algo mais vermelho que tinta. |
| 85 | **CRIMSON PACT** / PACTO CARMESIM | BLOOD / SANGUE | STATUS / SELF | RARE | ELEMENT / BLOOD | 10 | `DMG_PCT(40); HEAL_PCT(-50)` | +40% DMG; -50% HEAL / +40% DANO; -50% CURA | +40% DMG, ALL HEALING -50%. / +40% DE DANO, TODA CURA -50%. | Two signatures, one pen, and a paragraph nobody read. / Duas assinaturas, uma caneta e um parágrafo que ninguém leu. |
| 86 | **VAMPIRE TOUCH** / TOQUE VAMPÍRICO | BLOOD / SANGUE | STATUS / SELF | RARE | ELEMENT / BLOOD | 10 | `LIFESTEAL(3)` | 3% STEAL / 3% ROUBO | 3% LIFESTEAL. CAP 1 HP/S. / 3% DE ROUBO DE VIDA. LIMITE 1 DE VIDA/S. | A glove left on the doorstep of a manor with no mirrors. / Uma luva deixada na porta de uma mansão sem espelhos. |
| 87 | **RUPTURE** / RUPTURA | BLOOD / SANGUE | DAMAGE / TOUCH | RARE | ELEMENT / BLOOD | 10 | `HIT_BLEED(3, 3)` | 3 DPS; 3S / 3 DPS; 3S | HITS CAUSE BLEED: 3 DPS FOR 3S. / ACERTOS CAUSAM SANGRAMENTO: 3 DE DANO/S POR 3S. | — / — |
| 88 | **VEIN BURST** / EXPLOSÃO DE VEIA | BLOOD / SANGUE | DAMAGE / AREA OF EFFECT | RARE | ELEMENT / BLOOD | 10 | `KILL_BURST(8, 48)` | 8 DMG; 48PX / 8 DANO; 48PX | ON KILL, BLOOD BURSTS FOR 8 DMG. / AO MATAR, SANGUE EXPLODE CAUSANDO 8 DE DANO. | — / — |
| 89 | **MARTYR** / MÁRTIR | BLOOD / SANGUE | STATUS / SELF | EPIC | ELEMENT / BLOOD | 10 | `TAKEN_PCT(-15); DEALT_PCT(-10)` | -15% TAKEN; -10% DEALT / -15% RECEBIDO; -10% CAUSADO | TAKE 15% LESS DMG, DEAL 10% LESS DMG. / TOME 15% MENOS DANO, CAUSE 10% MENOS DANO. | — / — |
| 90 | **LAST BREATH** / ÚLTIMO SUSPIRO | BLOOD / SANGUE | WEAPON / SELF | EPIC | ELEMENT / BLOOD | 10 | `LOWHP_DMG(50, 30)` | +50% DMG / +50% DANO | +50% DMG BELOW 30% HP. / +50% DE DANO ABAIXO DE 30% DE VIDA. | Kept in a jar by something that wanted to remember how it felt. / Guardado num frasco por algo que queria lembrar como era. |
| 91 | **BLOOD MOON** / LUA DE SANGUE | BLOOD / SANGUE | DAMAGE / AREA OF EFFECT | EPIC | ELEMENT / BLOOD | 10 | `AURA(3, 56, 0); AURA_HEALKILL(1)` | 3 DPS; +1 HP/KILL / 3 DPS; +1 VIDA/MORTE | AURA DEALS 3 DPS. KILLS IN IT HEAL 1 HP. / AURA CAUSA 3 DE DANO/S. MORTES NELA CURAM 1 DE VIDA. | It rose red once, and has been collecting ever since. / Subiu vermelha uma vez e vem cobrando desde então. |
| 92 | **SANGUINE RITE** / RITO SANGUÍNEO | BLOOD / SANGUE | STATUS / SELF | EPIC | ELEMENT / BLOOD | 10 | `KILL_MAXHP(1, 10)` | +1 HP/10K / +1 VIDA/10 | EVERY 10 KILLS, GAIN +1 MAX HP. / A CADA 10 MORTES, GANHE +1 DE VIDA MÁXIMA. | — / — |
| 93 | **CRIMSON CHAIN** / CORRENTE CARMESIM | BLOOD / SANGUE | DAMAGE / TOUCH | EPIC | ELEMENT / BLOOD | 10 | `CRIT_HEAL(1)` | +1 HP/CRIT / +1 VIDA/CRÍT. | CRITS HEAL 1 HP. CAP 1 PER SECOND. / CRÍTICOS CURAM 1 DE VIDA. LIMITE 1 POR SEGUNDO. | — / — |
| 94 | **RED TIDE** / MARÉ VERMELHA | BLOOD / SANGUE | DAMAGE / AREA OF EFFECT | EPIC | ELEMENT / BLOOD | 10 | `AURA(2, 56, 0); KILL_HEALEVERY(1, 4)` | 2 DPS; 1HP/4K / 2 DPS; 1 VIDA/4 | AURA DEALS 2 DPS, HEALS 1 HP PER 4 KILLS. / AURA CAUSA 2 DE DANO/S, CURA 1 DE VIDA A CADA 4 MORTES. | — / — |
| 95 | **HEART OF GLASS** / CORAÇÃO DE VIDRO | BLOOD / SANGUE | STATUS / SELF | LEGENDARY | ELEMENT / BLOOD | 10 | `MAXHP_PCT(-40); DMG_PCT(60)` | -40% HP; +60% DMG / -40% VIDA; +60% DANO | -40% MAX HP, +60% DAMAGE. / -40% DE VIDA MÁXIMA, +60% DE DANO. | It beats. You can hear it. That is the problem. / — |
| 96 | **EXSANGUINATE** / EXSANGUINAR | BLOOD / SANGUE | WEAPON / TOUCH | LEGENDARY | ELEMENT / BLOOD | 10 | `EXSANG(8)` | 8%; 2X +1HP / 8%; 2X +1 VIDA | 8% CHANCE TO DEAL DOUBLE DMG AND HEAL 1 HP. / 8% DE CHANCE DE DANO DOBRADO E CURA 1 DE VIDA. | The last drop of a very persistent nobleman. / — |
| 97 | **JOLT** / PULSO | LIGHTNING / RAIO | DAMAGE / TOUCH | COMMON | ELEMENT / LIGHTNING | 10 | `ATKSPD_PCT(8)` | +8% ATK / +8% ATQ | +8% ATTACK SPEED. / +8% DE VELOCIDADE DE ATAQUE. | — / — |
| 98 | **STATIC CLING** / CARGA ESTÁTICA | LIGHTNING / RAIO | STATUS / TOUCH | COMMON | ELEMENT / LIGHTNING | 10 | `HIT_CHAIN(2, 1)` | +2 CHAIN / +2 CADEIA | HITS CHAIN 2 DMG TO A NEARBY FOE. / ACERTOS PASSAM 2 DE DANO A UM INIMIGO PRÓXIMO. | — / — |
| 99 | **STATIC BLOOM** / FLORAÇÃO ESTÁTICA | LIGHTNING / RAIO | DAMAGE / AREA OF EFFECT | COMMON | ELEMENT / LIGHTNING | 10 | `KILL_BURST(5, 40)` | 5 DMG; 40PX / 5 DANO; 40PX | ON KILL, A STATIC BURST FOR 5 DMG. / AO MATAR, UMA EXPLOSÃO ESTÁTICA DE 5 DE DANO. | — / — |
| 100 | **SHOCK THERAPY** / ELETROTERAPIA | LIGHTNING / RAIO | STATUS / SELF | COMMON | ELEMENT / LIGHTNING | 10 | `VS_FROZEN(25)` | +25% DMG / +25% DANO | STUNNED ENEMIES TAKE +25% DAMAGE. / INIMIGOS ATORDOADOS TOMAM +25% DE DANO. | — / — |
| 101 | **OVERCHARGE** / SOBRECARGA | LIGHTNING / RAIO | WEAPON / TOUCH | RARE | ELEMENT / LIGHTNING | 10 | `DMG_PCT(20); MAXHP_PCT(-10)` | +20% DMG; -10% HP / +20% DANO; -10% VIDA | +20% DMG, -10% MAX HP. / +20% DE DANO, -10% DE VIDA MÁXIMA. | — / — |
| 102 | **THUNDER STEP** / PASSO TROVÃO | LIGHTNING / RAIO | STATUS / SELF | RARE | ELEMENT / LIGHTNING | 10 | `MOVE_PCT(10); TRAIL_SPARK(2)` | +10% SPD; 2 DPS / +10% VEL.; 2 DPS | +10% MOVE SPEED, LEAVE SPARKS. / +10% DE VELOCIDADE E DEIXA FAÍSCAS. | — / — |
| 103 | **VOLT SHIELD** / ESCUDO VOLTAICO | LIGHTNING / RAIO | STATUS / SELF | RARE | ELEMENT / LIGHTNING | 10 | `ATTACKER_DMG(4)` | 4 DMG / 4 DANO | ATTACKERS TAKE 4 DAMAGE. / QUEM TE ATACA TOMA 4 DE DANO. | — / — |
| 104 | **ARC FLASH** / ARCO VOLTAICO | LIGHTNING / RAIO | DAMAGE / AREA OF EFFECT | RARE | ELEMENT / LIGHTNING | 10 | `NTH_STUN(6, 0.5, 56)` | 0.5S; 56PX / 0,5S; 56PX | EVERY 6TH THROW STUNS NEARBY FOES. / A CADA 6º ARREMESSO, ATORDOA INIMIGOS PRÓXIMOS. | — / — |
| 105 | **CHAIN LIGHTNING** / RAIO EM CADEIA | LIGHTNING / RAIO | WEAPON / TOUCH | EPIC | ELEMENT / LIGHTNING | 10 | `CRIT_CHAIN(2, 50)` | CHAIN 2; 50% DMG / CADEIA 2; 50% DANO | CRITS CHAIN TO 2 ENEMIES. / CRÍTICOS SALTAM PARA 2 INIMIGOS. | — / — |
| 106 | **CONDUCTOR** / CONDUTOR | LIGHTNING / RAIO | DAMAGE / AREA OF EFFECT | EPIC | ELEMENT / LIGHTNING | 10 | `AURA(2.5, 40, 0)` | 1/0.4S; 40PX / 1/0,4S; 40PX | AURA DEALS 1 DMG EVERY 0.4S. / AURA CAUSA 1 DE DANO A CADA 0,4S. | — / — |
| 107 | **STORMCALLER** / CHAMA-TEMPESTADE | LIGHTNING / RAIO | WEAPON / AREA OF EFFECT | EPIC | ELEMENT / LIGHTNING | 10 | `NTH_BOLT(1, 6, 30)` | +6 DMG; 30PX / +6 DANO; 30PX | EVERY THROW LANDS A LIGHTNING BOLT. / TODO ARREMESSO ATRAI UM RAIO. | A tuning fork the sky keeps trying to borrow. / Um diapasão que o céu vive tentando pedir emprestado. |
| 108 | **BALL LIGHTNING** / RAIO ESFÉRICO | LIGHTNING / RAIO | WEAPON / AREA OF EFFECT | EPIC | ELEMENT / LIGHTNING | 5 | `ORBIT(3, 2)` | ORBIT; 3 DPS / ÓRBITA; 3 DPS | THROWS ORBIT AND ZAP. / ARREMESSOS ORBITAM E CHOQUEIAM. | — / — |
| 109 | **THUNDERHEAD** / MENTE DO TROVÃO | LIGHTNING / RAIO | STATUS / SELF | EPIC | ELEMENT / LIGHTNING | 10 | `CHAINPLUS(1)` | +1 CHAIN / +1 CADEIA | +1 CHAIN ON ALL LIGHTNING EFFECTS. / +1 CADEIA EM TODOS OS EFEITOS DE RAIO. | — / — |
| 110 | **TEMPEST CROWN** / COROA DA TEMPESTADE | LIGHTNING / RAIO | STATUS / SELF | LEGENDARY | ELEMENT / LIGHTNING | 10 | `NTH_STRIKE3(5, 10, 3)` | 5TH; 3 TARGETS / 5º; 3 ALVOS | EVERY 5TH THROW STRIKES 3 FOES. / A CADA 5º ARREMESSO, ATINGE 3 INIMIGOS. | Three thrones, three kings, one very short argument. / — |
| 111 | **THUNDER GOD** / DEUS DO TROVÃO | LIGHTNING / RAIO | STATUS / SELF | LEGENDARY | ELEMENT / LIGHTNING | 10 | `LIGHTNING_PCT(30); ATKSPD_PCT(15)` | +30% DMG; +15% ATK / +30% DANO; +15% ATQ | +30% LIGHTNING DMG, +15% ATK SPEED. / +30% DE DANO DE RAIO, +15% DE VELOCIDADE DE ATAQUE. | A shard of a god's tooth. It still bites. / — |
| 112 | **THORNBACK** / DORSO DE ESPINHOS | EARTH / TERRA | DAMAGE / SELF | RARE | SEASON / FOREST | 10 | `REFLECT(60)` | 60% REFLECT / 60% REFLEXO | REFLECT 60% CONTACT DAMAGE. / REFLETE 60% DO DANO DE CONTATO. | — / — |
| 113 | **VERDANT GROWTH** / CRESCIMENTO VERDEJANTE | EARTH / TERRA | STATUS / SELF | EPIC | SEASON / FOREST | 10 | `HP_PER_WAVE(2)` | +2 HP/WAVE / +2 VIDA/ONDA | +2 MAX HP PER WAVE CLEARED. / +2 DE VIDA MÁXIMA POR ONDA CONCLUÍDA. | — / — |
| 114 | **CANOPY** / DOSSEL | AIR / AR | STATUS / SELF | RARE | SEASON / FOREST | 10 | `RANGED_TAKEN_PCT(-25)` | -25% RANGED / -25% DISTÂNCIA | -25% DAMAGE FROM RANGED ATTACKS. / -25% DE DANO DE ATAQUES À DISTÂNCIA. | — / — |
| 115 | **BRIAR TRAP** / ARMADILHA DE ESPINHOS | EARTH / TERRA | WEAPON / TOUCH | RARE | SEASON / FOREST | 10 | `HIT_ROOT(2)` | ROOT 2S / PRENDE 2S | HITS ROOT ENEMIES FOR 2S. / ACERTOS PRENDEM POR 2S. | — / — |
| 116 | **WILD HEART** / CORAÇÃO SELVAGEM | AIR / AR | STATUS / SELF | LEGENDARY | SEASON / FOREST | 10 | `DMG_PCT(20); MOVE_PCT(10)` | +20% DMG; +10% SPD / +20% DANO; +10% VEL. | +20% DMG AND +10% MOVE SPD THIS SEASON. / +20% DE DANO E +10% DE VELOCIDADE NESTA TEMPORADA. | A seed that grew around the axe instead of away from it. / — |
| 117 | **SUNBAKED** / TORRADO PELO SOL | FIRE / FOGO | STATUS / SELF | COMMON | SEASON / DESERT | 10 | `DMG_PCT(10)` | +10% DMG / +10% DANO | +10% DAMAGE THIS SEASON. / +10% DE DANO NESTA TEMPORADA. | — / — |
| 118 | **SCORCHING SANDS** / AREIAS ESCALDANTES | FIRE / FOGO | DAMAGE / AREA OF EFFECT | RARE | SEASON / DESERT | 10 | `AURA(2, 48, 0)` | 2 DPS; 48PX / 2 DPS; 48PX | AURA DEALS 2 DMG/S. / AURA CAUSA 2 DE DANO/S. | — / — |
| 119 | **MIRAGE** / MIRAGEM | FIRE / FOGO | STATUS / SELF | EPIC | SEASON / DESERT | 10 | `DODGE(15)` | 15% DODGE / 10% ESQUIVA | 15% CHANCE TO DODGE A HIT. / 10% DE CHANCE DE ESQUIVAR. | — / — |
| 120 | **SANDSTORM** / TEMPESTADE DE AREIA | EARTH / TERRA | DAMAGE / AREA OF EFFECT | EPIC | SEASON / DESERT | 10 | `NTH_BLAST(7, 6, 64)` | 6 DMG; 64PX / 6 DANO; 64PX | EVERY 7TH THROW BLINDS NEARBY FOES. / A CADA 7º ARREMESSO, CEGA INIMIGOS PRÓXIMOS. | — / — |
| 121 | **PHARAOH'S CURSE** / MALDIÇÃO DO FARAÓ | EARTH / TERRA | WEAPON / TOUCH | LEGENDARY | SEASON / DESERT | 10 | `KILL_CURSE(10, 48)` | 10 DMG; 48PX / 10 DANO; 48PX | KILLED FOES LEAVE A CURSED MARK THAT EXPLODES. / INIMIGOS MORTOS DEIXAM UMA MARCA QUE EXPLODE. | A grudge written in hieroglyphs, and it is a long one. / — |
| 122 | **ICE ARMOR** / ARMADURA DE GELO | WATER / ÁGUA | STATUS / SELF | RARE | SEASON / SNOW | 10 | `RES(3); MOVE_PCT(-5)` | +3 RES; -5% SPD / +3 RES; -5% VEL. | +3 RES, -5% MOVE SPEED. / +3 RES, -5% DE VELOCIDADE. | — / — |
| 123 | **WHITEOUT** / NEVASCA | AIR / AR | STATUS / AREA OF EFFECT | RARE | SEASON / SNOW | 10 | `AURA_MISS(20, 56)` | 20% MISS; 56PX / 20% ERRO; 56PX | NEARBY ENEMIES MISS 20% OF ATTACKS. / INIMIGOS PRÓXIMOS ERRAM 20% DOS ATAQUES. | — / — |
| 124 | **BLIZZARD** / NEVASCA FORTE | WATER / ÁGUA | DAMAGE / AREA OF EFFECT | EPIC | SEASON / SNOW | 10 | `AURA(1, 64, 30)` | -30% SPD; 1 DPS / -30% VEL.; 1 DPS | AURA SLOWS 30% AND DEALS 1 DPS. / AURA DEIXA 30% MAIS LENTO E CAUSA 1 DE DANO/S. | — / — |
| 125 | **PERMAFROST** / PERMAFROST | WATER / ÁGUA | WEAPON / TOUCH | EPIC | SEASON / SNOW | 10 | `HIT_FREEZE(40, 1.5)` | FREEZE 1.5S / CONGELA 1,5S | THROWS FREEZE FOR 1.5S. / ARREMESSOS CONGELAM POR 1,5S. | — / — |
| 126 | **FROZEN HEART** / CORAÇÃO CONGELADO | WATER / ÁGUA | STATUS / SELF | LEGENDARY | SEASON / SNOW | 10 | `IMMUNE_SLOW(1); DMG_PCT(20)` | IMMUNE SLOW; +20% DMG / IMUNE LENTO; +20% DANO | IMMUNE TO SLOWS, +20% DAMAGE. / IMUNE A LENTIDÃO, +20% DE DANO. | Somebody kept waiting. The waiting froze over. / — |
| 127 | **HIGH ALTITUDE** / ALTA ALTITUDE | AIR / AR | STATUS / SELF | RARE | SEASON / MOUNTAIN | 10 | `RANGE_PCT(15)` | +15% RANGE / +15% ALCANCE | +15% ATTACK RANGE. / +15% DE ALCANCE DE ATAQUE. | — / — |
| 128 | **ECHO PEAK** / PICO DO ECO | AIR / AR | DAMAGE / AREA OF EFFECT | RARE | SEASON / MOUNTAIN | 10 | `NTH_DOUBLE(4)` | 4TH; 2X / 4º; 2X | EVERY 4TH THROW FIRES TWICE. / A CADA 4º ARREMESSO, DISPARA DUAS VEZES. | — / — |
| 129 | **STONEHEART** / CORAÇÃO DE PEDRA | EARTH / TERRA | STATUS / SELF | EPIC | SEASON / MOUNTAIN | 10 | `RES(5); MOVE_PCT(-15)` | +5 RES; -15% SPD / +5 RES; -15% VEL. | +5 RES, -15% MOVE SPEED. / +5 RES, -15% DE VELOCIDADE. | — / — |
| 130 | **LANDSLIDE** / DESLIZAMENTO | EARTH / TERRA | WEAPON / AREA OF EFFECT | EPIC | SEASON / MOUNTAIN | 10 | `NTH_BLAST(8, 14, 56)` | 14 DMG; 56PX / 14 DANO; 56PX | EVERY 8TH THROW DROPS BOULDERS. / A CADA 8º ARREMESSO, PEDRAS CAEM. | — / — |
| 131 | **SUMMIT'S EDGE** / FIO DO CUME | EARTH / TERRA | WEAPON / TOUCH | LEGENDARY | SEASON / MOUNTAIN | 10 | `DMG_PCT(35); MAXHP_PCT(-20)` | +35% DMG; -20% HP / +35% DANO; -20% VIDA | +35% DAMAGE, -20% MAX HP. / +35% DE DANO, -20% DE VIDA MÁXIMA. | The piece of the mountain that argued with the sky. / — |
| 132 | **ECLIPSE BRAND** / MARCA DO ECLIPSE | LIGHTNING / RAIO | DAMAGE / TOUCH | RARE | SEASON / ECLIPSE | 10 | `MARK(20, 3)` | +20% DMG; 3S / +20% DANO; 3S | MARKED FOES TAKE +20% DAMAGE FOR 3S. / ALVOS MARCADOS TOMAM +20% DE DANO POR 3S. | — / — |
| 133 | **UMBRAL VEIL** / VÉU UMBRAL | BLOOD / SANGUE | STATUS / SELF | EPIC | SEASON / ECLIPSE | 10 | `DODGE(15)` | 15% DODGE / 15% ESQUIVA | 15% CHANCE TO DODGE A HIT. / 15% DE CHANCE DE ESQUIVAR. | — / — |
| 134 | **DARK RITE** / RITO SOMBRIO | BLOOD / SANGUE | STATUS / SELF | EPIC | SEASON / ECLIPSE | 10 | `KILLHEAL(1, 1)` | +1 HP/KILL / +1 VIDA/MORTE | KILLS HEAL 1 HP. CAP 1 PER SECOND. / MORTES CURAM 1 DE VIDA. LIMITE 1 POR SEGUNDO. | — / — |
| 135 | **VOID TOUCH** / TOQUE DO VAZIO | BLOOD / SANGUE | WEAPON / TOUCH | EPIC | SEASON / ECLIPSE | 10 | `ARMOR_IGNORE(1)` | IGNORE ARMOR / IGNORA ARMADURA | THROWS IGNORE ENEMY ARMOR. / ARREMESSOS IGNORAM A ARMADURA INIMIGA. | — / — |
| 136 | **TOTAL ECLIPSE** / ECLIPSE TOTAL | LIGHTNING / RAIO | DAMAGE / AREA OF EFFECT | LEGENDARY | SEASON / ECLIPSE | 10 | `NTH_BLAST(15, 40, 999)` | 40 DMG; FULL SCREEN / 40 DANO; TELA TODA | EVERY 15TH THROW DARKENS THE SKY. / A CADA 15º ARREMESSO, O CÉU ESCURECE. | — / — |
| 137 | **SENTINEL DRONE** / DRONE SENTINELA | EARTH / TERRA | WEAPON / SELF | RARE | SEASON / ECLIPSE | 5 | `DRONE_MINIGUN(1)` | 5 SHOTS/S; FAST / 5 DISPAROS/S; RAPIDO | A DRONE ORBITS YOU AND FIRES AT THE NEAREST FOE. / UM DRONE ORBITA E DISPARA NO INIMIGO MAIS PROXIMO. | — / — |
| 138 | **KNIFE CIRCLE** / CIRCULO DE FACAS | AIR / AR | WEAPON / AREA OF EFFECT | RARE | ELEMENT / AIR | 5 | `ORBIT(4, 1)` | 4 DPS; 1 KNIFE / 4 DPS; 1 FACA | SHARP KNIVES SPIN AROUND YOU, DEALING DAMAGE. / FACAS AFIADAS GIRAM AO SEU REDOR E CAUSAM DANO. | A wheel of sharpened steel, forever seeking a target. / Uma roda de aco afiado, sempre a procura de um alvo. |
| 139 | **OCCASIONAL BOMB** / BOMBA OCASIONAL | FIRE / FOGO | DAMAGE / AREA OF EFFECT | EPIC | ELEMENT / FIRE | 10 | `PERIODIC_BOMB(12, 40, 60)` | 12S INTERVAL; 40 DMG; 60PX / INTERVALO 12S; 40 DANO; 60PX | A BOMB APPEARS NEAR YOU AND EXPLODES AFTER 1 SECOND. / UMA BOMBA SURGE PERTO E EXPLODE APOS 1 SEGUNDO. | The fuse burns slowly. Its maker never came back for it. / O pavio queima devagar. Quem a criou nunca voltou. |
| 140 | **SENTINEL** / SENTINELA | EARTH / TERRA | WEAPON / SELF | LEGENDARY | ELEMENT / EARTH | 5 | `SENTINEL(1)` | 50 HP; 0.8S SHOTS / 50 VIDA; DISPARO 0,8S | A STONE SENTINEL GUARDS YOU AND FIRES AT NEARBY FOES. / UMA SENTINELA DE PEDRA PROTEGE E ATIRA EM INIMIGOS. | An ancient guardian that still recognizes the living. / Uma guardia antiga que ainda reconhece os vivos. |
| 141 | **GLACIAL BLOOM** / FLOR GLACIAL | WATER / ÁGUA | DAMAGE / AREA OF EFFECT | RARE | ELEMENT / WATER | 10 | `HIT_FREEZE(18, 1.2); KILL_BURST(6, 32)` | 18% FREEZE; 6 BURST DMG / 32 RADIUS / 18% CONGELAR; 6 DANO / RAIO 32 | FREEZE FOES ON HIT, THEN SHATTER THEM WITH A BURST WHEN THEY FALL. / CONGELE INIMIGOS AO ACERTAR E OS ESTILHACA EM UMA EXPLOSAO AO CAIR. | Old ice blooms again wherever a foe falls, turning every defeat into a brittle frostburst. / O gelo antigo floresce onde um inimigo cai, transformando cada derrota em uma explosao congelante. |
| 142 | **STARLANCE** / LANCA ESTELAR | LIGHTNING / RAIO | WEAPON / SELF | EPIC | ELEMENT / LIGHTNING | 10 | `NTH_BOLT(7, 14, 44)` | EVERY 7TH THROW; 14 DMG / 44 RADIUS / A CADA 7 DISPAROS; 14 DANO / RAIO 44 | EVERY FEW ATTACKS, A LIGHTNING LANCE STRIKES NEARBY FOES. / A CADA POUCOS ATAQUES, UMA LANCA ELETRICA ATINGE INIMIGOS PROXIMOS. | A needle of starlight waits between volleys, then crosses the battlefield in a single bright instant. / Uma agulha de luz estelar aguarda entre disparos e cruza o campo em um unico instante brilhante. |
| 143 | **SOUL HARVEST** / COLHEITA DE ALMAS | BLOOD / SANGUE | STATUS / SELF | RARE | ELEMENT / BLOOD | 10 | `KILL_HEALEVERY(1.5, 5); LIFESTEAL(1)` | HEAL EVERY 5 KILLS; 1.5 HP / 1 LIFESTEAL / CURA A CADA 5 ABATES; 1,5 VIDA / 1 DRENAGEM | KILLS FEED YOUR VITALITY; BLOODLUST RESTORES HEALTH ON EVERY HIT. / ABATES ALIMENTAM SUA VITALIDADE; GOLPES DE SANGUE RESTAURAM VIDA. | The harvest is never wasted: every fallen enemy and every stolen heartbeat returns a little strength. / Nada se perde na colheita: cada inimigo abatido e cada batimento roubado devolvem um pouco de vigor. |
| 144 | **ELEMENTALIST CYCLE** / CICLO DO ELEMENTALIST | AIR / AR | DAMAGE / SELF | LEGENDARY | GLOBAL | 10 | `ELEMENTALIST(8, 18, 42)` | EVERY 8TH ATTACK; RANDOM ELEMENTAL STRIKE / A CADA 8 ATAQUES; GOLPE ELEMENTAL ALEATORIO | EVERY FEW ATTACKS, RANDOMLY RELEASES AN EARTH SPIKE, WATER SLASH, AIR PUSH, OR FIREBALL. / A CADA POUCOS ATAQUES, LIBERA UM GOLPE ALEATORIO DE TERRA, ÁGUA, AR OU FOGO. | Four elements answer the same call, but never in the same order. / Quatro elementos respondem ao mesmo chamado, mas nunca na mesma ordem. |
| 145 | **FAIRY COMPANION** / COMPANHEIRA FADA | AIR / AR | STATUS / SELF | EPIC | GLOBAL | 5 | `FAIRY(1, 4, 0)` | SUMMONED COMPANION; HEALS EVERY 4 SECONDS / COMPANHEIRA INVOCADA; CURA A CADA 4 SEGUNDOS | A FAIRY FOLLOWS YOU AND PERIODICALLY RESTORES A SMALL AMOUNT OF LIFE. / UMA FADA SEGUE VOCE E RESTAURA VIDA PERIODICAMENTE. | A tiny light follows the survivor and refuses to let hope die. / Uma pequena luz segue o sobrevivente e se recusa a deixar a esperanca morrer. |
| 146 | **TIME-STOP** / PARADA TEMPORAL | LIGHTNING / RAIO | STATUS / SELF | LEGENDARY | GLOBAL | 9 | `TIME_STOP(12, 1.2, 14)` | EVERY 12TH ATTACK; 1.2S STOP / 14S COOLDOWN / A CADA 12 ATAQUES; 1,2S PARADO / 14S DE RECARGA | EVERY 12 ATTACKS, BRIEFLY STOPS ENEMIES AND THEIR PROJECTILES. RARE AND POWERFUL, WITH A LONG COOLDOWN. / A CADA 12 ATAQUES, PARA INIMIGOS E PROJETEIS POR UM INSTANTE. | Chronos grants no second chance: one stolen moment, then a long silence before time can be folded again. / Chronos nao concede uma segunda chance: um momento roubado, depois um longo silencio antes que o tempo se dobre outra vez. |

## Weapons

All nine selectable weapons have explicit combat stats. Only Machine Gun is completion-locked; the other eight are available from the weapon selector. Base values are before player stats and spell modifiers.

| # | Weapon | Description (EN / PT-BR) | Damage | Attack interval | Range | Projectile speed | Pierce | Hit radius | Unlock / special behavior |
|---:|---|---|---:|---:|---:|---:|---:|---:|---|
| 0 | **DAGGER** / ADAGA | RAPID KNIVES AT THE NEAREST FOE. / FACAS RÁPIDAS NO INIMIGO MAIS PRÓXIMO. | 3 | 0.28 s | 90 px | 250 px/s | 0 | 3 px | Available from start. |
| 1 | **BOW** / ARCO | A LONG ARROW IN A STRAIGHT LINE. / UMA FLECHA LONGA EM LINHA RETA. | 6 | 0.65 s | 210 px | 310 px/s | 0 | 3 px | Available from start. |
| 2 | **THROWING AXE** / MACHADO DE ARREMESSO | A HEAVY AXE THAT ARCS OVER ITS TARGET. / UM MACHADO PESADO QUE FAZ ARCO SOBRE O ALVO. | 11 | 1.15 s | 140 px | 150 px/s | 0 | 22 px | Available from start. |
| 3 | **MAGE STAFF** / CAJADO DE MAGO | A SLOW ORB THAT HITS VERY HARD. / UMA ESFERA LENTA QUE BATE MUITO FORTE. | 24 | 1.9 s | 160 px | 90 px/s | 1 | 8 px | Available from start. |
| 4 | **SHOTGUN** / ESPINGARDA | A FIVE-PELLET BLAST WITH A TIGHT SPREAD. / UMA RAJADA DE CINCO PROJETEIS COM DISPERSAO CURTA. | 3.5 | 0.95 s | 115 px | 240 px/s | 0 | 2.5 px | Available from start. |
| 5 | **PISTOL** / PISTOLA | A FAST, ACCURATE SHOT WITH MEDIUM RANGE. / UM TIRO RAPIDO E PRECISO DE ALCANCE MEDIO. | 7.5 | 0.48 s | 185 px | 360 px/s | 0 | 2.5 px | Available from start. |
| 6 | **TRIDENT** / TRIDENTE | THROW IT, THEN MOVE CLOSE TO RECLAIM IT. / ARREMESSE E CHEGUE PERTO PARA RECUPERAR. | 18 | 1.35 s | 220 px | 170 px/s | 0 | 5 px | Thrown weapon; retrieve it by moving within 32 px. |
| 7 | **KATANA** / KATANA | A FAST CLOSE-RANGE SWING THAT HITS IN AN ARC AND MAY BLEED. / UM GOLPE RAPIDO DE CURTO ALCANCE QUE ATINGE EM ARCO E PODE CAUSAR SANGRAMENTO. | 9 | 0.48 s | 56 px | 250 px/s | 0 | 3 px | Automatic close-range cleave; 46 px hit range; may cause bleed; distinct swing VFX/SFX. |
| 8 | **MACHINE GUN** / METRALHADORA | A RAPID MEDIUM-RANGE STREAM; HEAVY TO CARRY, SO YOU MOVE SLOWER. / UMA RAJADA RÁPIDA DE ALCANCE MEDIO; PESADA, ENTÃO VOCÊ SE MOVE MAIS DEVAGAR. | 4 | 0.14 s | 240 px | 420 px/s | 0 | 2.5 px | Unlock after clearing all three campaign levels in all five seasons; movement speed multiplier 0.70. |

## Music and sound effects

Music is synthesized in real time by a compact 22.05 kHz mono software mixer. Each season has a procedural 16-step theme; the menu uses a sixth internal pattern that is not a catalog-unlock track. Tracks unlock by completing all three levels of their season.

| Season | Track title (EN / PT-BR) | Root MIDI note | Scale intervals (semitones) | Pattern degrees (16 steps) | Unlock |
|---|---|---:|---|---|---|
| FOREST | FOREST - ROOTS OF DUSK / FLORESTA - RAIZES DO CREPUSCULO | 57 | 0, 2, 3, 5, 7, 8, 10 | 0, rest, 2, 4, rest, 2, 0, rest, 3, rest, 4, 6, 4, rest, 2, rest | Clear all 3 Forest campaign levels. |
| DESERT | DESERT - SUNKEN HORIZON / DESERTO - HORIZONTE SUBMERSO | 52 | 0, 1, 4, 5, 7, 8, 10 | 0, 1, rest, 4, 3, rest, 1, 0, 4, rest, 3, 1, rest, 0, 1, rest | Clear all 3 Desert campaign levels. |
| SNOW | SNOW - WHITE SILENCE / NEVE - SILENCIO BRANCO | 59 | 0, 2, 3, 5, 7, 8, 11 | 4, rest, 2, rest, 0, 2, 4, rest, 5, 4, rest, 2, 0, rest, 2, rest | Clear all 3 Snow campaign levels. |
| MOUNTAIN | MOUNTAIN - STONE ABOVE / MONTANHA - PEDRA ACIMA | 50 | 0, 2, 3, 5, 7, 8, 10 | 0, rest, rest, 3, 0, rest, 4, rest, 0, 3, rest, 2, 0, rest, rest, 1 | Clear all 3 Mountain campaign levels. |
| ECLIPSE | ECLIPSE - THE LAST LIGHT / ECLIPSE - A ULTIMA LUZ | 49 | 0, 1, 3, 4, 6, 7, 9 | 0, 1, 3, rest, 1, 3, 4, rest, 6, 4, 3, rest, 1, 0, rest, 1 | Clear all 3 Eclipse campaign levels. |

### Procedural sound effects

SFX are generated oscillator/noise sweeps; `noise=0` is tonal, `noise=1` is noise, and `noise=2` mixes both. Values are source synthesis parameters (start/end frequency in Hz, duration in seconds, linear amplitude, noise mode).

| SFX | Start Hz | End Hz | Duration | Amplitude | Sound source |
|---|---:|---:|---:|---:|---|
| SHOOT | 900 | 500 | 0.06 s | 0.10 | tonal |
| HIT | 300 | 200 | 0.05 s | 0.16 | noise |
| KILL | 320 | 110 | 0.12 s | 0.18 | tonal |
| HURT | 220 | 70 | 0.18 s | 0.26 | noise |
| PICKUP | 1200 | 1900 | 0.05 s | 0.10 | tonal |
| LEVEL | 500 | 1100 | 0.30 s | 0.16 | tonal |
| UI | 800 | 800 | 0.04 s | 0.10 | tonal |
| WAVE | 400 | 700 | 0.20 s | 0.14 | tonal |
| BOSS | 140 | 60 | 0.50 s | 0.26 | noise |
| DEATH | 400 | 45 | 0.80 s | 0.26 | tonal |
| BLAST | 260 | 60 | 0.25 s | 0.24 | noise |
| SWING | 1500 | 240 | 0.12 s | 0.14 | mixed noise + tone |

## Bestiary

There are **25 enemy types**. Values are base data before wave scaling. `Behavior` combines the source special label with the implemented behavior flags.

| # | Enemy (EN / PT-BR) | Season | HP | Contact/projectile damage | Speed | Attack rate | Range | Description (EN / PT-BR) | Special / behavior (EN / PT-BR) | Lore (EN / PT-BR) |
|---:|---|---|---:|---:|---|---|---|---|---|---|
| 0 | **GRUB** / LARVA | FOREST | 6 | 1 | SLOW | NORMAL | TOUCH | A FAT LARVA THAT CRAWLS STRAIGHT AT YOU. / UMA LARVA GORDA QUE RASTEJA DIRETO ATÉ VOCÊ. | NONE / NENHUM | It has no eyes, but it always finds you. / Não tem olhos, mas sempre encontra você. |
| 1 | **STINGER WASP** / VESPA FERROADORA | FOREST | 4 | 2 | FAST | NORMAL | TOUCH | FAST AND BRITTLE, DIVES AT YOUR HEAD. / RÁPIDA E FRÁGIL, MERGULHA NA SUA CABEÇA. | DASHES AFTER A SHORT WINDUP; DASH / INVESTE APÓS UM CURTO PREPARO | It apologizes after the sting, but only out of habit. / Pede desculpas depois da ferroada, mas só por hábito. |
| 2 | **BARK BEETLE** / BESOURO-CASCA | FOREST | 18 | 2 | VERY SLOW | SLOW | TOUCH | AN ARMORED WALL OF BARK AND LEGS. / UMA MURALHA BLINDADA DE CASCA E PATAS. | ARMORED -1 DMG; ARMOR / BLINDADO: -1 DE DANO | Wears the forest floor like a coat of old doors. / Veste o chão da floresta como um casaco de portas velhas. |
| 3 | **WEB SPITTER** / COSPE-TEIAS | FOREST | 8 | 1 | NORMAL | SLOW | HIGH | LURKS IN CORNERS AND SPITS STICKY WEBS. / ESPREITA NOS CANTOS E COSPE TEIAS PEGAJOSAS. | WEBS SLOW YOU; WEB / AS TEIAS DEIXAM VOCÊ LENTO | Every web is a letter, and most of them are about you. / Cada teia é uma carta, e a maioria é sobre você. |
| 4 | **BROOD MOTHER (BOSS)** / MÃE DA NINHADA (CHEFE) | FOREST | 240 | 6 | NORMAL | NORMAL | HIGH | THE FOREST'S HEART, BIRTHING ENDLESS WASPS. / O CORAÇÃO DA FLORESTA, GERANDO VESPAS SEM FIM. | SUMMONS WASPS, ENRAGES AT 50%; BOSS / INVOCA VESPAS, ENFURECE COM 50% | She does not fight. She only makes more. / Ela não luta. Ela só faz mais. |
| 5 | **SCARAB** / ESCARAVELHO | DESERT | 8 | 1 | NORMAL | NORMAL | TOUCH | A SCUTTLING DUNE BEETLE. / UM BESOURO DAS DUNAS QUE ANDA DE LADO. | NONE / NENHUM | Pushes a small sun across the sand and resents every witness. / Empurra um solzinho pela areia e se ressente de cada testemunha. |
| 6 | **BANDAGED THRALL** / ESCRAVO ENFAIXADO | DESERT | 14 | 3 | VERY SLOW | SLOW | TOUCH | A WRAPPED CORPSE THAT RISES AGAIN. / UM CADÁVER ENFAIXADO QUE LEVANTA DE NOVO. | REVIVES ONCE AT 50% HP; REVIVE / REVIVE UMA VEZ COM 50% DE VIDA | Still wearing the linen it was promised would be temporary. / Ainda usa o linho que lhe prometeram ser temporário. |
| 7 | **ANUBIS HOUND** / CÃO DE ANÚBIS | DESERT | 12 | 3 | VERY FAST | NORMAL | TOUCH | A JACKAL-FACED HUNTER THAT LUNGES IN A LINE. / UM CAÇADOR COM CARA DE CHACAL QUE DÁ BOTES EM LINHA. | STRAIGHT-LINE LUNGE; LUNGE / BOTE EM LINHA RETA | Its smile is the only part of it that holds still. / O sorriso é a única parte dele que fica parada. |
| 8 | **TOMB GUARDIAN** / GUARDIÃO DA TUMBA | DESERT | 24 | 4 | VERY SLOW | SLOW | TOUCH | A SHIELDED STATUE THAT MUST BE FLANKED. / UMA ESTÁTUA COM ESCUDO QUE PRECISA SER FLANQUEADA. | SHIELD: HIT FROM BEHIND; SHIELD / ESCUDO: ATAQUE PELAS COSTAS | Sworn to a door that fell down three kingdoms ago. / Jurou proteger uma porta que caiu três reinos atrás. |
| 9 | **SUN PHARAOH (BOSS)** / FARAÓ SOLAR (CHEFE) | DESERT | 260 | 7 | LOW | NORMAL | HIGH | THE SUN-CROWNED RULER OF THE DUNES. / O GOVERNANTE DAS DUNAS COROADO PELO SOL. | SUMMONS REVIVING THRALLS, SOLAR BEAM SWEEP, ENRAGES AT 30%; BOSS / INVOCA ESCRAVOS QUE REVIVEM, VARREDURA DE RAIO SOLAR, ENFURECE COM 30% | His crown is a sunrise he refuses to let finish. / A coroa dele é um nascer do sol que ele se recusa a deixar terminar. |
| 10 | **FROST BAT** / MORCEGO GÉLIDO | SNOW | 6 | 2 | VERY FAST | FAST | TOUCH | A SHRIEKING FLIER, ERRATIC AND FAST. / UM VOADOR ESTRIDENTE, ERRÁTICO E RÁPIDO. | ERRATIC FLIGHT; ERRATIC / VOO ERRÁTICO | Screams in a key the snow has never learned to ignore. / Grita numa nota que a neve nunca aprendeu a ignorar. |
| 11 | **ICE ELEMENTAL** / ELEMENTAL DE GELO | SNOW | 16 | 3 | SLOW | NORMAL | TOUCH | A FROZEN CORE THAT SPLITS WHEN BROKEN. / UM NÚCLEO CONGELADO QUE SE DIVIDE AO QUEBRAR. | SPLITS INTO 2 ON DEATH; SPLIT_DEATH / DIVIDE-SE EM 2 AO MORRER | Somewhere inside, a very small summer is still trying to get out. / Lá dentro, um verão minúsculo ainda tenta escapar. |
| 12 | **FROST ARCHER** / ARQUEIRO GÉLIDO | SNOW | 10 | 3 | LOW | SLOW | VERY HIGH | A PATIENT SHOOTER IN A FROSTED HOOD. / UM ATIRADOR PACIENTE COM CAPUZ COBERTO DE GELO. | HITS SLOW YOU 25%; SLOWHIT / ACERTOS DEIXAM VOCÊ 25% MAIS LENTO | Frost on the string, and nothing in the quiver but winters. / Geada na corda e nada na aljava além de invernos. |
| 13 | **SNOW STALKER** / ESPREITADOR DA NEVE | SNOW | 14 | 5 | FAST | NORMAL | TOUCH | A WHITE AMBUSHER THAT HIDES IN THE DRIFTS. / UM EMBOSCADOR BRANCO QUE SE ESCONDE NOS MONTES DE NEVE. | INVISIBLE UNTIL CLOSE; INVIS / INVISÍVEL ATÉ CHEGAR PERTO | The drift behind you is a little too neat. / O monte de neve atrás de você está arrumado demais. |
| 14 | **THE FROZEN PEAK (BOSS)** / O PICO CONGELADO (CHEFE) | SNOW | 300 | 8 | VERY SLOW | LOW | HIGH | A LIVING MOUNTAIN OF ICE. / UMA MONTANHA VIVA DE GELO. | GROUND SLAM, ICE SHARD VOLLEY, SUMMONS ICE ELEMENTALS; BOSS / PANCADA NO CHÃO, CHUVA DE ESTILHAÇOS DE GELO, INVOCA ELEMENTAIS DE GELO | It was a mountain before it was a mood. / Era uma montanha antes de virar um humor. |
| 15 | **ROCK GOLEM** / GOLEM DE PEDRA | MOUNTAIN | 40 | 5 | VERY SLOW | VERY SLOW | TOUCH | A WALKING BOULDER, HEAVY AND ARMORED. / UM ROCHEDO ANDANTE, PESADO E BLINDADO. | SPLITS AT 50% HP; SPLIT_HALF / SE DIVIDE COM 50% DE VIDA | Lichen has been using it as a bench for three hundred years. / Há trezentos anos o líquen o usa como banco. |
| 16 | **AVALANCHE BRUTE** / BRUTO DA AVALANCHE | MOUNTAIN | 26 | 6 | NORMAL (BURST) | VERY SLOW | TOUCH | A CHARGING WALL OF STONE AND FURY. / UMA MURALHA DE PEDRA E FÚRIA QUE INVESTE. | CHARGES IN A LINE, TELEGRAPHED; CHARGE / INVESTE EM LINHA, ANUNCIADO ANTES | Arrives like a bad decision made by an entire hillside. / Chega como uma má decisão tomada por uma colina inteira. |
| 17 | **STONE SENTRY** / SENTINELA DE PEDRA | MOUNTAIN | 18 | 3 | STATIONARY | NORMAL | VERY HIGH | A STONE TURRET THAT NEVER BLINKS. / UMA TORRE DE PEDRA QUE NUNCA PISCA. | FIRES ROCK SHARDS IN A FAN; FAN / DISPARA LASCAS DE PEDRA EM LEQUE | Carved to watch one road, and still watching since the road moved. / Esculpido para vigiar uma estrada, e segue vigiando desde que a estrada mudou de lugar. |
| 18 | **CRYSTAL CRAWLER** / RASTEJANTE DE CRISTAL | MOUNTAIN | 10 | 4 | FAST | NORMAL | TOUCH | A GLASS CANNON COVERED IN CRYSTALS. / UM CANHÃO DE VIDRO COBERTO DE CRISTAIS. | SPITS A SHARD THAT BOUNCES OFF WALLS; BOUNCE / COSPE UM ESTILHAÇO QUE QUICA NAS PAREDES | Every facet shows you a slightly worse version of yourself. / Cada faceta mostra uma versão um pouco pior de você. |
| 19 | **THE MOUNTAIN KING (BOSS)** / O REI DA MONTANHA (CHEFE) | MOUNTAIN | 320 | 8 | VERY SLOW | LOW | HIGH | THE CROWNED RULER OF THE PEAKS. / O GOVERNANTE COROADO DOS PICOS. | GROUND SLAM, SUMMONS GOLEMS, ENRAGES AT 40%; BOSS / PANCADA NO CHÃO, INVOCA GOLEMS, ENFURECE COM 40% | His crown is a ring of summits, and he has never once taken it off. / A coroa é um anel de picos, e ele nunca a tirou. |
| 20 | **BLOOD ACOLYTE** / ACÓLITO DE SANGUE | ECLIPSE | 10 | 2 | NORMAL | NORMAL | HIGH | A ROBED CASTER THAT DRAINS YOUR STATS. / UM CONJURADOR ENCAPUZADO QUE DRENA SEUS ATRIBUTOS. | ON HIT DRAINS 1 RANDOM STAT; DRAIN / AO ACERTAR, DRENA 1 ATRIBUTO ALEATÓRIO | The robe is soaked red, and the prayer is mostly arithmetic. / O manto está encharcado de vermelho, e a oração é basicamente aritmética. |
| 21 | **FLESH HOUND** / CÃO DE CARNE | ECLIPSE | 18 | 4 | FAST | FAST | TOUCH | A WRITHING THING THAT HEALS FROM YOUR PAIN. / UMA COISA CONTORCIDA QUE SE CURA COM A SUA DOR. | HEALS ITSELF FOR DAMAGE DEALT; LIFESTEAL / CURA-SE PELO DANO CAUSADO | Sniffs out pain the way other hounds sniff out supper. / Fareja dor como outros cães farejam o jantar. |
| 22 | **EYE OF THE ECLIPSE** / OLHO DO ECLIPSE | ECLIPSE | 22 | 3 | VERY SLOW | NORMAL | VERY HIGH | A WATCHING EYE THAT BORROWS YOUR POWER. / UM OLHO VIGILANTE QUE TOMA SEU PODER EMPRESTADO. | COPIES ONE OF YOUR SPELLS AS AN ATTACK; COPY / COPIA UMA DAS SUAS MAGIAS COMO ATAQUE | It watches you, and takes notes in your handwriting. / Observa você e anota tudo com a sua letra. |
| 23 | **WAILING HUSK** / CASCA LAMURIOSA | ECLIPSE | 26 | 2 | VERY SLOW | VERY SLOW | HIGH | A HOLLOW SCREAMER THAT CALLS FOR HELP. / UMA CASCA OCA QUE GRITA POR AJUDA. | SCREAMS, SPAWNS 2 ENEMIES PER SCREAM; SCREAM / GRITA E INVOCA 2 INIMIGOS POR GRITO | Hollow as a bell and twice as loud. / Oco como um sino e duas vezes mais barulhento. |
| 24 | **THE UMBRAL CHOIR (BOSS)** / CORO UMBRAL (CHEFE) | ECLIPSE | 340 | 8 | LOW | NORMAL | VERY HIGH | A CHORUS OF MOUTHS SINGING THE END. / UM CORO DE BOCAS CANTANDO O FIM. | 3 PHASES: DRAIN, MIRROR, SUMMON; BOSS / 3 FASES: DRENAR, ESPELHAR, INVOCAR | Every mouth is singing your name, and none of them agree on the tune. / Cada boca canta seu nome, e nenhuma concorda com a melodia. |

## Seasons, campaigns, and arenas

Campaign seasons each contain three levels with wave quotas of 10, 15, and 20. Completing the prior season unlocks the next; completing the prior level unlocks the next level in that season.

| Season | Elements | Gimmick (EN / PT-BR) | Level 1 | Level 2 | Level 3 | All-Seasons availability |
|---|---|---|---|---|---|---|
| FOREST / FLORESTA | EARTH (TERRA) / AIR (AR) | THORN BRAMBLES BLOCK SHOTS AND SLOW ALL MOVEMENT. / SARÇAS DE ESPINHOS BLOQUEIAM OS TIROS E REDUZEM TODO O MOVIMENTO. | VERDANT PATH / CAMINHO VERDEJANTE (10 waves) | DEEP THICKET / MATA FECHADA (15 waves) | HEARTWOOD / CERNE DA FLORESTA (20 waves) | All-Seasons arena: clear all 15 campaign levels across the five seasons. |
| DESERT / DESERTO | FIRE (FOGO) / EARTH (TERRA) | HEAT PULSES BURN EVERYONE EVERY 15 SECONDS. / PULSOS DE CALOR QUEIMAM TODOS A CADA 15 SEGUNDOS. | SCORCHED APPROACH / CAMINHO ESCALDANTE (10 waves) | GLASS DUNES / DUNAS DE VIDRO (15 waves) | SUNKEN TEMPLE / TEMPLO SUBMERSO (20 waves) | All-Seasons arena: clear all 15 campaign levels across the five seasons. |
| SNOW / NEVE | WATER (ÁGUA) / AIR (AR) | FROST PATCHES SLOW MOVEMENT. STALKERS HIDE IN THE DRIFTS. / PLACAS DE GELO REDUZEM O MOVIMENTO. ESPREITADORES SE ESCONDEM NA NEVE. | FROZEN PASS / PASSAGEM CONGELADA (10 waves) | WHITEOUT EXPANSE / BRANDAO TOTAL (15 waves) | GLACIAL HEART / CORACAO GLACIAL (20 waves) | All-Seasons arena: clear all 15 campaign levels across the five seasons. |
| MOUNTAIN / MONTANHA | EARTH (TERRA) / AIR (AR) | FALLING ROCKS CRASH DOWN AFTER A SHORT WARNING. / PEDRAS CAEM APÓS UM CURTO AVISO. | STONE ASCENT / ASCENSAO DE PEDRA (10 waves) | THUNDER RIDGE / PICO DO TROVAO (15 waves) | SKYFORGE SUMMIT / CUME DA FORJA CELESTE (20 waves) | All-Seasons arena: clear all 15 campaign levels across the five seasons. |
| ECLIPSE / ECLIPSE | BLOOD (SANGUE) / LIGHTNING (RAIO) | DARKNESS HIDES THE EDGES OF THE ARENA. / A ESCURIDÃO ESCONDE AS BORDAS DA ARENA. | TWILIGHT VERGE / LIMIAR DO CREPUSCULO (10 waves) | BLACK SUN BASIN / BACIA DO SOL NEGRO (15 waves) | EVENTIDE THRONE / TRONO DO FIM DO DIA (20 waves) | All-Seasons arena: clear all 15 campaign levels across the five seasons. |

## Achievements

There are **22 achievements**. Exact completion criteria are taken from the localized descriptions; rewards are listed where applicable.

| # | Achievement (EN / PT-BR) | Requirement (EN / PT-BR) | Reward / note |
|---:|---|---|---|
| 1 | **FOREST CLEAR** / CONCLUSAO DA FLORESTA | Clear the Forest season. / Conclua a temporada da Floresta. | Unlocks the Forest music track and next season |
| 2 | **DESERT CLEAR** / CONCLUSAO DO DESERTO | Clear the Desert season. / Conclua a temporada do Deserto. | Unlocks the Desert music track and next season |
| 3 | **SNOW CLEAR** / CONCLUSAO DA NEVE | Clear the Snow season. / Conclua a temporada da Neve. | Unlocks the Snow music track and next season |
| 4 | **MOUNTAIN CLEAR** / CONCLUSAO DA MONTANHA | Clear the Mountain season. / Conclua a temporada da Montanha. | Unlocks the Mountain music track and next season |
| 5 | **ECLIPSE CLEAR** / CONCLUSAO DO ECLIPSE | Clear the Eclipse season. / Conclua a temporada do Eclipse. | Unlocks the Eclipse music track |
| 6 | **FIRST THOUSAND** / PRIMEIRO MILHAR | Defeat 1,000 enemies. / Derrote 1.000 inimigos. | Achievement only; no cosmetic reward. |
| 7 | **TEN THOUSAND** / DEZ MIL | Defeat 10,000 enemies. / Derrote 10.000 inimigos. | Achievement only; no cosmetic reward. |
| 8 | **HUNDRED THOUSAND** / CEM MIL | Defeat 100,000 enemies. / Derrote 100.000 inimigos. | Achievement only; no cosmetic reward. |
| 9 | **FOREST CATALOG** / CATALOGO DA FLORESTA | Discover the Forest catalog. / Descubra o catalogo da Floresta. | Achievement only; no cosmetic reward. |
| 10 | **DESERT CATALOG** / CATALOGO DO DESERTO | Discover the Desert catalog. / Descubra o catalogo do Deserto. | Achievement only; no cosmetic reward. |
| 11 | **SNOW CATALOG** / CATALOGO DA NEVE | Discover the Snow catalog. / Descubra o catalogo da Neve. | Achievement only; no cosmetic reward. |
| 12 | **MOUNTAIN CATALOG** / CATALOGO DA MONTANHA | Discover the Mountain catalog. / Descubra o catalogo da Montanha. | Achievement only; no cosmetic reward. |
| 13 | **ECLIPSE CATALOG** / CATALOGO DO ECLIPSE | Discover the Eclipse catalog. / Descubra o catalogo do Eclipse. | Achievement only; no cosmetic reward. |
| 14 | **MASTER CATALOG** / CATALOGO COMPLETO | Discover every spell and enemy in the game. / Descubra todas as magias e inimigos do jogo. | Master-catalog achievement; full discovery also satisfies the Arc Warden outfit requirement |
| 15 | **LITTLE DUMMY** / BOBINHO | Play for at least 1 hour. / Jogue por pelo menos 1 hora. | Achievement only; no cosmetic reward. |
| 16 | **ABYSS VETERAN** / VETERANO DO ABISMO | Play for at least 10 hours. / Jogue por pelo menos 10 horas. | Achievement only; no cosmetic reward. |
| 17 | **GO TOUCH GRASS** / VÁ TOCAR GRAMA! | Play for at least 50 hours. Please take a break. / Jogue por pelo menos 50 horas. Faca uma pausa. | Achievement only; no cosmetic reward. |
| 18 | **NIGHT WATCH** / VIGILANTE NOTURNO | Play for at least 1 hour to unlock the Nightwarden outfit. / Jogue por pelo menos 1 hora para liberar a roupa Nightwarden. | NIGHT COWL (parody headwear) |
| 19 | **WHY SO SERIOUS?** / POR QUE ESTÁ TÃO SÉRIO? | Play for at least 10 hours to unlock the Grinmask outfit. / Jogue por pelo menos 10 horas para liberar a roupa Grinmask. | GRINNING MASK (parody headwear) |
| 20 | **CRIMSON CAP** / BONE CARMESIM | Defeat 1,000 enemies to unlock the Crimson Cap outfit. / Derrote 1.000 inimigos para liberar a roupa Bone Carmesim. | Achievement only; no cosmetic reward. |
| 21 | **HEART OF IRON** / CORAÇÃO DE FERRO | Complete the full catalog to unlock the Arc Helmet outfit. / Complete o catálogo para liberar a roupa Capacete Arc. | ARC HELMET (parody headwear) |
| 22 | **ELEMENTALIST** / ELEMENTALISTA | Use at least two distinct spells from each spell type. / Use pelo menos duas magias diferentes de cada tipo. | AIR NOMAD ELEMENTALIST outfit |

## Customization

Hats/outfits, faces, and colors are cosmetic only. There are 11 headwear/outfit options, six faces, and six colors. Ordinary options are available immediately; special parody outfits and Elementalist require achievements.

### Headwear and outfits

| # | Option (EN / PT-BR) | Unlock |
|---:|---|---|
| 0 | KNIGHT HELM / ELMO DO CAVALEIRO | Available immediately |
| 1 | WIZARD HAT / CHAPEU DE MAGO | Available immediately |
| 2 | LEAF HOOD / CAPUZ DE FOLHAS | Available immediately |
| 3 | TINY CROWN / COROA PEQUENA | Available immediately |
| 4 | NONE / NENHUM | Available immediately |
| 5 | NIGHT COWL / CAPUZ NOTURNO | Achievement reward; see the matching achievement above |
| 6 | GRINNING MASK / MASCARA SORRIDENTE | Achievement reward; see the matching achievement above |
| 7 | CRIMSON CAP / BONE CARMESIM | Achievement reward; see the matching achievement above |
| 8 | ARC HELMET / CAPACETE ARC | Achievement reward; see the matching achievement above |
| 9 | AIR NOMAD ELEMENTALIST / ELEMENTALIST NOMADE DO AR | Achievement reward; see the matching achievement above |

### Faces

| # | Option (EN / PT-BR) | Unlock |
|---:|---|---|
| 0 | BEARD / BARBA | Available immediately |
| 1 | GLASSES / ÓCULOS | Available immediately |
| 2 | SCAR / CICATRIZ | Available immediately |
| 3 | FRECKLES / SARDAS | Available immediately |
| 4 | NONE / NENHUM | Available immediately |
| 5 | DWARF BEARD / BARBA DE ANAO | Available immediately |

### Colors

| # | Option (EN / PT-BR) | Unlock |
|---:|---|---|
| 0 | RED / VERMELHO | Available immediately |
| 1 | BLUE / AZUL | Available immediately |
| 2 | GREEN / VERDE | Available immediately |
| 3 | GOLD / DOURADO | Available immediately |
| 4 | GRAY / CINZA | Available immediately |
| 5 | WHITE / BRANCO | Available immediately |

## Player stats and progression

Base player values and per-point benefits are shown below. A point bonus is additive to the applicable derived multiplier; internal safety clamps still apply.

| Stat | Base value | Effect per allocated point |
|---|---:|---|
| Strength | 8 | +8% damage and +2% projectile size |
| Resistance | 5 | +6 maximum HP and 2% less damage taken |
| Speed | 4 | +4% movement speed and +4% attack speed |
| Luck | 3 | +3 percentage points critical chance, +8% XP, +2% rare-spell bonus |
| Intelligence | 5 | -4% cooldown and +6% area-of-effect size |

Base HP is 60. The starting movement speed is 62 px/s. Attacks automatically target enemies; move and dodge, collect XP, spend stat points, and build a spell loadout. The maximum spell-slot count is 12, including the starting weapon slot.

### Content and unlock summary

- **Six elements:** Water, Fire, Earth, Air, Blood, and Lightning.
- **Five campaign seasons / 15 named levels**, plus the All-Seasons arena unlocked by clearing Eclipse 1–3.
- **Five procedural catalog music tracks**, unlocked by completing their corresponding season; a separate menu pattern is always internal.
- **Nine weapons**, with Machine Gun gated behind full campaign completion across all five seasons.
- **25 enemy types**, including bosses, seasonal behaviors, and special attack patterns.
- **Achievements** and catalog discovery cover season clears, kill milestones, time played, spells/enemies discovered, and cosmetics.
- **Save data** preserves unlocks and discovery; the current v5 format migrates older v1–v4 files.

### Localization and scope notes

The game supports English and Brazilian Portuguese. CATALOG.md lists source-defined names, descriptions, stat strings, lore, effect payloads, unlocks, and combat values. Portuguese fields without a distinct translation are shown with the English fallback used by the runtime.
Some displayed spell names are not unique; use the numbered entry plus its scope/season to identify the exact spell. Source data is authoritative when a marketing summary and an implementation detail differ.
