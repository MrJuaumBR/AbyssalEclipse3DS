#!/usr/bin/env python3
"""Generates source/data_gen.cpp + source/strings_gen.h from tools/data/*.json
(exported from the Abyssal Eclipse HTML mockup).  Edit THIS file, never the output."""
import json, re, os, sys
from art_raster import rects
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
D = json.load(open(os.path.join(HERE, 'data', 'mock_data.json'), encoding='utf-8'))
A = json.load(open(os.path.join(HERE, 'data', 'art.json'), encoding='utf-8'))

SEAS = ['FOREST','DESERT','SNOW','MOUNTAIN','ECLIPSE']
ABSOLUTE_MAX_SPELL_LEVEL = 10
LEVEL_FIVE_EFFECTS = {'ORBIT', 'SENTINEL', 'DRONE_MINIGUN', 'FAIRY'}
LEVEL_CAP_OVERRIDES = {'TIME-STOP': 9}  # all tuned parameters reach their limits by Rank 9

def fl(x): return repr(float(x)) + 'f'

def esc(s):
    s = (s.replace('\u2019',"'").replace('\u2018',"'").replace('\u2014','-').replace('\u2013','-')
          .replace('\u2026','...').replace('\u201c','"').replace('\u201d','"').replace('\u00d7','x')
          .replace('\u2192','->').replace('\u2194','<->'))
    s = s.replace('\\','\\\\').replace('"','\\"').replace('\n','\\n').replace('?','\\?')
    return '"' + s + '"'

# ----------------------------------------------------------------- effects
# DSL: (spell name, season) -> list of (KIND, a, b, c)
M = {}
def S(name, *effs, season=None): M[(name, season)] = effs
# global
S('IRON WILL',('MAXHP',12)); S('QUICKSTEP',('MOVE_PCT',8)); S('FOCUS STONE',('CD_PCT',10))
S('LUCKY FANG',('CRIT',8)); S('FLINT SHARD',('FLATDMG',2)); S('WIND CHIME',('AURA',0,48,10))
S('SAP GLAND',('KILLHEAL',1,2)); S('EMBER CORE',('KILL_BURST',8,40))
S('SPARK TIP',('HIT_BONUS',15,3)); S('TOXIC SPORE',('KILL_POISON',5,3,40))
S('THORN MAIL',('REFLECT',40)); S('HUNTER\'S EYE',('CRIT',15),('CRITDMG',25))
S('STATIC FIELD',('AURA',2,48,0)); S('ECHO CHAMBER',('NTH_RING',5)); S('FROST LENS',('HIT_FREEZE',20,1))
S('RICOCHET',('BOUNCE_ENEMY',1,50)); S('TIDAL FORCE',('PIERCE',3))
S('PACT OF IRON',('RES',3),('MAXHP',25),('MOVE_PCT',-15)); S('ELEMENTAL CORE',('DMG_PCT',15))
S('GRAVITY WELL',('AURA_PULL',64)); S('ECLIPSE CROWN',('ALLNUM_PCT',30)); S('WORLD ENDER',('KILL_CHAIN',10))
# water
S('AQUA VEIL',('RES',2)); S('BRINE TOUCH',('VS_SLOWED',10)); S('BUBBLE BURST',('HIT_BURST',6,32))
S('SPLASH GUARD',('TAKEN_PCT',-10)); S('TIDE RING',('AURA',0,64,25)); S('RIPTIDE',('KNOCKBACK',1))
S('FROSTBITE',('HIT_SLOW',20,2)); S('TIDAL SHIELD',('SHIELD_WAVE',20)); S('UNDERTOW',('KILL_SLOWAOE',30,2,48))
S('DEEP CURRENT',('AURA',2,56,15)); S('MIRROR POOL',('MIRROR',20)); S('WHIRLPOOL',('WHIRL',2,3))
S('RAIN DANCE',('KILL_MOVE',2,20)); S('PRESSURE POINT',('CRITBONUS',40)); S('ABYSSAL PACT',('NTH_BLAST',10,60,110))
# fire
S('EMBER TRAIL',('TRAIL_FIRE',4,1)); S('BLAZING FANG',('VS_BURNING',12)); S('ASHEN WAKE',('KILL_CLOUD',3,2,40))
S('SCORCH',('HIT_BURN',2,2)); S('PYRE',('KILL_EXPLODE_BURNING',8,40)); S('CINDER CLOAK',('BURN_TOUCH',3,3))
S('FLAME WHEEL',('ORBIT',5,2)); S('HEAT SHOCK',('KILL_BURST',6,48)); S('SUNSPOT',('AURA',1,40,0))
S('WARMTH',('WARMTH',15)); S('MOLTEN CORE',('EXPLODE',10,40)); S('INFERNO BRAND',('CRIT_BURN',4,3))
S('FIRESTORM',('NTH_BLAST',6,8,64)); S('SOLAR FLARE',('PERIODIC_FLARE',8,10)); S('PHOENIX DOWN',('REVIVE',30))
# earth
S('STONE SKIN',('RES',2),('MOVE_PCT',-5)); S('ROOT SNARE',('HIT_ROOT',1)); S('DUST CLOUD',('KILL_SLOWAOE',20,2,48))
S('MOSS CLOAK',('REGEN',1,8)); S('BOULDER TOSS',('DMG_PCT',25),('ATKSPD_PCT',-15)); S('ECHO STONE',('BOUNCE_WALL',2))
S('TREMBLE',('NTH_STUN',5,0.5,48)); S('STURDY FRAME',('DEBUFF_PCT',-30)); S('CLAY GUARD',('BLOCK_WAVE',1))
S('GAIA\'S GIFT',('MAXHP',40),('MOVE_PCT',-10)); S('SPLIT STONE',('SPLIT',3,50)); S('AVALANCHE',('NTH_BLAST',8,12,56))
S('MOUNTAIN\'S WILL',('RES',3),('MAXHP',20)); S('PETRIFY',('HIT_STUN',5,2)); S('TECTONIC',('NTH_BLAST',12,20,80))
# air
S('GALE STEP',('MOVE_PCT',12)); S('FEATHER FALL',('RANGED_TAKEN_PCT',-20)); S('ZEPHYR BLADE',('THROWSPD_PCT',10))
S('WHISPER',('CRIT',5),('MOVE_PCT',5)); S('CYCLONE',('HOMING',1)); S('SONIC CRACK',('HIT_STUN',10,0.5))
S('TAILWIND',('ATKSPD_PCT',15)); S('VACUUM',('AURA_WEAKEN',15,56)); S('MIRAGE',('DODGE',10))
S('UPDRAFT',('PIERCE',2),('THROWSPD_PCT',20)); S('SHATTER WIND',('VS_FROZEN',20)); S('STORMBREAK',('NTH_BLAST',7,10,64))
S('SKY DANCE',('NTH_DOUBLE',3)); S('TEMPEST',('HIT_BOLT',8,2)); S('EYE OF THE STORM',('ATKSPD_PCT',25),('MOVE_PCT',10))
# blood
S('BLOOD PEBBLE',('LIFESTEAL',1)); S('THIRST',('VS_WOUNDED',15)); S('BLOOD SIGIL',('DMG_PCT',30),('MAXHP_PCT',-25))
S('CRIMSON PACT',('DMG_PCT',40),('HEAL_PCT',-50)); S('VAMPIRE TOUCH',('LIFESTEAL',3)); S('RUPTURE',('HIT_BLEED',3,3))
S('VEIN BURST',('KILL_BURST',8,48)); S('MARTYR',('TAKEN_PCT',-15),('DEALT_PCT',-10)); S('LAST BREATH',('LOWHP_DMG',50,30))
S('BLOOD MOON',('AURA',3,56,0),('AURA_HEALKILL',1)); S('SANGUINE RITE',('KILL_MAXHP',1,10)); S('CRIMSON CHAIN',('CRIT_HEAL',1))
S('RED TIDE',('AURA',2,56,0),('KILL_HEALEVERY',1,4)); S('HEART OF GLASS',('MAXHP_PCT',-40),('DMG_PCT',60)); S('EXSANGUINATE',('EXSANG',8))
# lightning
S('JOLT',('ATKSPD_PCT',8)); S('STATIC CLING',('HIT_CHAIN',2,1)); S('STATIC BLOOM',('KILL_BURST',5,40))
S('SHOCK THERAPY',('VS_FROZEN',25)); S('OVERCHARGE',('DMG_PCT',20),('MAXHP_PCT',-10)); S('THUNDER STEP',('MOVE_PCT',10),('TRAIL_SPARK',2))
S('VOLT SHIELD',('ATTACKER_DMG',4)); S('ARC FLASH',('NTH_STUN',6,0.5,56)); S('CHAIN LIGHTNING',('CRIT_CHAIN',2,50))
S('CONDUCTOR',('AURA',2.5,40,0)); S('STORMCALLER',('NTH_BOLT',1,6,30)); S('BALL LIGHTNING',('ORBIT',3,2))
S('THUNDERHEAD',('CHAINPLUS',1)); S('TEMPEST CROWN',('NTH_STRIKE3',5)); S('THUNDER GOD',('LIGHTNING_PCT',30),('ATKSPD_PCT',15))
# season exclusives
S('THORNBACK',('REFLECT',60),season='FOREST'); S('VERDANT GROWTH',('HP_PER_WAVE',2),season='FOREST')
S('CANOPY',('RANGED_TAKEN_PCT',-25),season='FOREST'); S('BRIAR TRAP',('HIT_ROOT',2),season='FOREST')
S('WILD HEART',('DMG_PCT',20),('MOVE_PCT',10),season='FOREST')
S('SUNBAKED',('DMG_PCT',10),season='DESERT'); S('SCORCHING SANDS',('AURA',2,48,0),season='DESERT')
S('MIRAGE',('DODGE',15),season='DESERT'); S('SANDSTORM',('NTH_BLAST',7,6,64),season='DESERT')
S('PHARAOH\'S CURSE',('KILL_CURSE',10,48),season='DESERT')
S('ICE ARMOR',('RES',3),('MOVE_PCT',-5),season='SNOW'); S('WHITEOUT',('AURA_MISS',20,56),season='SNOW')
S('BLIZZARD',('AURA',1,64,30),season='SNOW'); S('PERMAFROST',('HIT_FREEZE',40,1.5),season='SNOW')
S('FROZEN HEART',('IMMUNE_SLOW',1),('DMG_PCT',20),season='SNOW')
S('HIGH ALTITUDE',('RANGE_PCT',15),season='MOUNTAIN'); S('ECHO PEAK',('NTH_DOUBLE',4),season='MOUNTAIN')
S('STONEHEART',('RES',5),('MOVE_PCT',-15),season='MOUNTAIN'); S('LANDSLIDE',('NTH_BLAST',8,14,56),season='MOUNTAIN')
S('SUMMIT\'S EDGE',('DMG_PCT',35),('MAXHP_PCT',-20),season='MOUNTAIN')
S('ECLIPSE BRAND',('MARK',20,3),season='ECLIPSE'); S('UMBRAL VEIL',('DODGE',15),season='ECLIPSE')
S('DARK RITE',('KILLHEAL',1,1),season='ECLIPSE'); S('VOID TOUCH',('ARMOR_IGNORE',1),season='ECLIPSE')
S('TOTAL ECLIPSE',('NTH_BLAST',15,40,999),season='ECLIPSE')
# new custom spells
S('KNIFE CIRCLE',('ORBIT',4.0,1.0))
S('OCCASIONAL BOMB',('PERIODIC_BOMB',12.0,40.0,60.0))
S('SENTINEL',('SENTINEL',1.0))
S('SENTINEL DRONE',('DRONE_MINIGUN',1),season='ECLIPSE')
# Level-tuned payloads for formerly binary/interval-only spells.
S('ECHO CHAMBER',('NTH_RING',5,12,0.25))
S('TEMPEST CROWN',('NTH_STRIKE3',5,10,3))
# New hybrid spells combine existing systems and benefit from meaningful upgrades.
S('GLACIAL BLOOM',('HIT_FREEZE',18,1.2),('KILL_BURST',6,32))
S('STARLANCE',('NTH_BOLT',7,14,44))
S('SOUL HARVEST',('KILL_HEALEVERY',1.5,5),('LIFESTEAL',1))
S('AVATAR CYCLE',('AVATAR',8,18,42))
S('FAIRY COMPANION',('FAIRY',1.0,4.0,0.0))
S('TIME-STOP',('TIME_STOP',12,1.2,14))

KINDS = sorted({e[0] for v in M.values() for e in v})

# ----------------------------------------------------------------- enemy behaviour map
BEH = {'wasp':['DASH'],'beetle':['ARMOR'],'spider':['WEB'],'brood':['BOSS'],'thrall':['REVIVE'],'hound':['LUNGE'],
 'guard':['SHIELD'],'pharaoh':['BOSS'],'bat':['ERRATIC'],'ice':['SPLIT_DEATH'],'archer':['SLOWHIT'],'stalker':['INVIS'],
 'frozen':['BOSS'],'golem':['SPLIT_HALF'],'brute':['CHARGE'],'sentry':['FAN'],'crawler':['BOUNCE'],'king':['BOSS'],
 'acolyte':['DRAIN'],'flesh':['LIFESTEAL'],'eye':['COPY'],'husk':['SCREAM'],'choir':['BOSS']}
SPD = {'STATIONARY':'SW_STATIONARY','VERY SLOW':'SW_VERY_SLOW','SLOW':'SW_SLOW','LOW':'SW_LOW','NORMAL':'SW_NORMAL',
       'NORMAL (BURST)':'SW_NORMAL','FAST':'SW_FAST','VERY FAST':'SW_VERY_FAST'}
ATK = {'VERY SLOW':'AW_VERY_SLOW','SLOW':'AW_SLOW','LOW':'AW_LOW','NORMAL':'AW_NORMAL','FAST':'AW_FAST'}
RNG = {'TOUCH':'RW_TOUCH','HIGH':'RW_HIGH','VERY HIGH':'RW_VERY_HIGH'}

# ----------------------------------------------------------------- extra UI strings (not in the mockup)
EXTRA = {  # key: (en, pt)
 'paused':('PAUSED','PAUSADO'), 'resume':('RESUME','CONTINUAR'), 'quit_menu':('QUIT TO MENU','SAIR PARA O MENU'),
 'opt_fps':('30 FPS MODE (SAVES BATTERY)','MODO 30 FPS (ECONOMIZA BATERIA)'), 'opt_showfps':('SHOW FPS COUNTER','EXIBIR CONTADOR DE FPS'),
 'all_seasons':('Y: ALL-SEASONS RUN','Y: MODO TODAS AS ESTACOES'), 'on':('ON','LIGADO'), 'off':('OFF','DESLIGADO'),
 'upgrade_t':('UPGRADE A SPELL','MELHORE UM FEITICO'), 'lvl_n':('LV {n}','NV {n}'), 'run_over':('RUN OVER','FIM DA JORNADA'),
 'victory':('LEVEL CLEARED','FASE CONCLUIDA'), 'new_best':('NEW BEST WAVE','NOVO RECORDE DE ONDA'),
 'retry':('PRESS A TO CONTINUE','APERTE A PARA CONTINUAR'),
 'press_start':('PRESS START','APERTE START'),
 'not_found':('NOT FOUND YET','AINDA NAO ENCONTRADO'), 'unknown':('???','???'),
 'dbg_unlock':('UNLOCK ALL','DESBLOQUEAR TUDO'), 'dbg_title':('DEBUG','DEPURACAO'), 'dbg_god':('GOD MODE','MODO DEUS'),
 'dbg_xp':('GIVE LEVEL','DAR NIVEL'), 'sel_dbg':('SELECT: DEBUG','SELECT: DEPURACAO'),
 'wave_n':('WAVE {n}','ONDA {n}'), 'boss_incoming':('BOSS INCOMING','CHEFE CHEGANDO'),
 'level_up':('LEVEL UP','SUBIU DE NIVEL'), 'slots':('SLOTS {n}/12','SLOTS {n}/12'),
 'btn_back':('B: BACK','B: VOLTAR'), 'btn_ok':('A: OK','A: OK'),
 'save_failed':('SAVE FAILED (SD CARD?)','FALHA AO SALVAR (CARTAO SD?)'),
 'no_sound':('NO DSP FIRMWARE: SILENT MODE','SEM FIRMWARE DSP: MODO SILENCIOSO'),
 'cleared':('CLEARED','CONCLUIDA'), 'pts':('PTS','PTS'), 'kills_s':('KILLS','ABATES'),
 'sea_locked_hint':('CLEAR THE PREVIOUS SEASON','CONCLUA A ESTACAO ANTERIOR'),
 'lvl_locked_hint':('CLEAR THE PREVIOUS LEVEL','CONCLUA A FASE ANTERIOR'),
 'ph_placeholder':('(PLACEHOLDER NAME)','(NOME PROVISORIO)'),
 'lang_toggle':('A: CHANGE LANGUAGE','A: TROCAR IDIOMA'),
 'gim_desert':('HEAT PULSES BURN EVERYONE EVERY 15 SECONDS.','PULSOS DE CALOR QUEIMAM TODOS A CADA 15 SEGUNDOS.'),
 'gim_snow':('FROST PATCHES SLOW MOVEMENT. STALKERS HIDE IN THE DRIFTS.','PLACAS DE GELO REDUZEM O MOVIMENTO. ESPREITADORES SE ESCONDEM NA NEVE.'),
 'gim_mountain':('FALLING ROCKS CRASH DOWN AFTER A SHORT WARNING.','PEDRAS CAEM APÓS UM CURTO AVISO.'),
 'gim_eclipse':('DARKNESS HIDES THE EDGES OF THE ARENA.','A ESCURIDÃO ESCONDE AS BORDAS DA ARENA.'),
 'hint_dpad':('D-PAD: MOVE   A: OK   B: BACK','D-PAD: MOVER   A: OK   B: VOLTAR'),
 'sea_n':('SEASON','ESTAÇÃO'),
 'spell_lv':('LV {n}','NV {n}'), 'max_lv':('MAX LEVEL','NIVEL MAXIMO'),
 'new_label':('NEW!', 'NOVO!'),
 'nav_achievements':('ACHIEVEMENTS','CONQUISTAS'), 'ach_title':('ACHIEVEMENTS','CONQUISTAS'),
 'ach_locked':('LOCKED','BLOQUEADA'), 'ach_unlocked':('UNLOCKED','DESBLOQUEADA'),
 'ach_season_forest':('FOREST COMPLETE','FLORESTA COMPLETA'),
 'ach_season_forest_desc':('Clear all three Forest campaign levels.','Conclua as tres fases da Floresta.'),
 'ach_season_desert':('DESERT COMPLETE','DESERTO COMPLETO'),
 'ach_season_desert_desc':('Clear all three Desert campaign levels.','Conclua as tres fases do Deserto.'),
 'ach_season_snow':('SNOW COMPLETE','NEVE COMPLETA'),
 'ach_season_snow_desc':('Clear all three Snow campaign levels.','Conclua as tres fases da Neve.'),
 'ach_season_mountain':('MOUNTAIN COMPLETE','MONTANHA COMPLETA'),
 'ach_season_mountain_desc':('Clear all three Mountain campaign levels.','Conclua as tres fases da Montanha.'),
 'ach_season_eclipse':('ECLIPSE COMPLETE','ECLIPSE COMPLETO'),
 'ach_season_eclipse_desc':('Clear all three Eclipse campaign levels.','Conclua as tres fases do Eclipse.'),
 'ach_kills_1000':('1,000 ENEMIES','1.000 INIMIGOS'),
 'ach_kills_1000_desc':('Defeat 1,000 enemies across all runs.','Derrote 1.000 inimigos em todas as partidas.'),
 'ach_kills_10000':('10,000 ENEMIES','10.000 INIMIGOS'),
 'ach_kills_10000_desc':('Defeat 10,000 enemies across all runs.','Derrote 10.000 inimigos em todas as partidas.'),
 'ach_kills_100000':('100,000 ENEMIES','100.000 INIMIGOS'),
 'ach_kills_100000_desc':('Defeat 100,000 enemies across all runs.','Derrote 100.000 inimigos em todas as partidas.'),
 'ach_catalog_forest':('FOREST FIELD GUIDE','GUIA DA FLORESTA'),
 'ach_catalog_forest_desc':('Discover every Forest-exclusive spell and enemy.','Descubra todas as magias e inimigos exclusivos da Floresta.'),
 'ach_catalog_desert':('DESERT FIELD GUIDE','GUIA DO DESERTO'),
 'ach_catalog_desert_desc':('Discover every Desert-exclusive spell and enemy.','Descubra todas as magias e inimigos exclusivos do Deserto.'),
 'ach_catalog_snow':('SNOW FIELD GUIDE','GUIA DA NEVE'),
 'ach_catalog_snow_desc':('Discover every Snow-exclusive spell and enemy.','Descubra todas as magias e inimigos exclusivos da Neve.'),
 'ach_catalog_mountain':('MOUNTAIN FIELD GUIDE','GUIA DA MONTANHA'),
 'ach_catalog_mountain_desc':('Discover every Mountain-exclusive spell and enemy.','Descubra todas as magias e inimigos exclusivos da Montanha.'),
 'ach_catalog_eclipse':('ECLIPSE FIELD GUIDE','GUIA DO ECLIPSE'),
 'ach_catalog_eclipse_desc':('Discover every Eclipse-exclusive spell and enemy.','Descubra todas as magias e inimigos exclusivos do Eclipse.'),
 'ach_full_catalog':('MASTER CATALOG','CATALOGO COMPLETO'),
 'ach_full_catalog_desc':('Discover every spell and enemy in the game.','Descubra todas as magias e inimigos do jogo.'),
 'ach_little_dummy':('LITTLE DUMMY','PEQUENO BOBO'), 'ach_little_dummy_desc':('Play for at least 1 hour.','Jogue por pelo menos 1 hora.'),
 'ach_abyss_veteran':('ABYSS VETERAN','VETERANO DO ABISMO'), 'ach_abyss_veteran_desc':('Play for at least 10 hours.','Jogue por pelo menos 10 horas.'),
 'ach_touch_grass':('GO TOUCH GRASS','VAI TOCAR GRAMA'), 'ach_touch_grass_desc':('Play for at least 50 hours. Please take a break.','Jogue por pelo menos 50 horas. Faca uma pausa.'),
 'ach_batman':('NIGHT WATCH','VIGILANTE NOTURNO'), 'ach_batman_desc':('Play for at least 1 hour to unlock the Batman parody outfit.','Jogue por pelo menos 1 hora para liberar a roupa parodia do Batman.'),
 'ach_joker':('WHY SO SERIOUS?','POR QUE TAO SERIO?'), 'ach_joker_desc':('Play for at least 10 hours to unlock the Joker parody outfit.','Jogue por pelo menos 10 horas para liberar a roupa parodia do Joker.'),
 'ach_terraria':('DIG DEEP','CAVE FUNDO'), 'ach_terraria_desc':('Play for at least 50 hours to unlock the Terraria parody outfit.','Jogue por pelo menos 50 horas para liberar a roupa parodia de Terraria.'),
 'ach_mario':("IT'S-A ME",'AQUI E O MARIO'), 'ach_mario_desc':('Defeat 1,000 enemies to unlock the Mario parody outfit.','Derrote 1.000 inimigos para liberar a roupa parodia do Mario.'),
 'ach_iron_man':('HEART OF IRON','CORACAO DE FERRO'), 'ach_iron_man_desc':('Complete the full catalog to unlock the Iron Man parody outfit.','Complete o catalogo para liberar a roupa parodia do Homem de Ferro.'),
 'ach_avatar':('AVATAR','AVATAR'), 'ach_avatar_desc':('Use at least two distinct spells from each spell type to unlock the Avatar outfit.','Use pelo menos duas magias diferentes de cada tipo para liberar a roupa do Avatar.'),
 'stat_games':('RUNS','PARTIDAS'), 'stat_wins':('WINS','VITORIAS'), 'stat_losses':('LOSSES','DERROTAS'),
 'stat_kills':('LIFETIME KILLS','ABATES TOTAIS'), 'stat_picks':('SPELLS ACQUIRED','MAGIAS OBTIDAS'),
 'stat_casts':('SPELL ACTIVATIONS','ATIVACOES DE MAGIA'),
 'stat_upgrades':('SPELL UPGRADES','MELHORIAS DE MAGIA'), 'stat_throws':('WEAPON ATTACKS','ATAQUES DA ARMA'),
 'stat_time':('TIME PLAYED','TEMPO JOGADO'), 'ach_progress':('PROGRESS','PROGRESSO'),
 'ach_hint':('UP/DOWN: SELECT   B: BACK','CIMA/BAIXO: SELECIONAR   B: VOLTAR'),
 'cat_clothes':('CLOTHES','ROUPAS'), 'cat_music':('MUSIC','MUSICAS'),
 'music_locked':('CLEAR THE SEASON TO UNLOCK THIS TRACK','CONCLUA A ESTACAO PARA DESBLOQUEAR ESTA MUSICA'),
 'music_play':('A: PLAY TRACK','A: TOCAR MUSICA'), 'music_stop':('B: STOP PREVIEW','B: PARAR PREVIA'),
 'cloth_bat_cowl':('NIGHT COWL','CAPUZ NOTURNO'), 'cloth_joker_grin':('GRINNING MASK','MASCARA SORRIDENTE'),
 'cloth_terra_hat':('PIXEL MINER HELM','CAPACETE DO MINEIRO PIXEL'), 'cloth_abyss_crown':('ABYSSAL CROWN','COROA ABISSAL'),
 'cloth_default':('KNIGHT HELM','ELMO DO CAVALEIRO'), 'cloth_wizard':('WIZARD HAT','CHAPEU DE MAGO'), 'cloth_leaf':('LEAF HOOD','CAPUZ DE FOLHAS'), 'cloth_crown':('TINY CROWN','COROA PEQUENA'), 'cloth_none':('NONE','NENHUM'),
 'cloth_batman':('NIGHT COWL','CAPUZ NOTURNO'), 'cloth_joker':('GRINNING MASK','MASCARA SORRIDENTE'), 'cloth_terraria':('PIXEL MINER HELM','CAPACETE DO MINEIRO PIXEL'), 'cloth_mario':('RED PLUMBER CAP','BONE VERMULHO'), 'cloth_ironman':('ARC HELMET','CAPACETE ARC'),
 'face_dwarf_beard':('DWARF BEARD','BARBA DE ANAO'),
 'music_forest':('FOREST - ROOTS OF DUSK','FLORESTA - RAIZES DO CREPUSCULO'),
 'music_desert':('DESERT - SUNKEN HORIZON','DESERTO - HORIZONTE SUBMERSO'),
 'music_snow':('SNOW - WHITE SILENCE','NEVE - SILENCIO BRANCO'),
 'music_mountain':('MOUNTAIN - STONE ABOVE','MONTANHA - PEDRA ACIMA'),
 'music_eclipse':('ECLIPSE - THE LAST LIGHT','ECLIPSE - A ULTIMA LUZ'),
 'store_points':('X: STORE POINTS','X: GUARDAR PONTOS'), 'use_stored':('Y: USE STORED ({n})','Y: USAR GUARDADOS ({n})'),
 'stores_left':('BANK USES LEFT: {n}/3','USOS DO BANCO: {n}/3'), 'points_stored':('POINTS STORED','PONTOS GUARDADOS'),
 'release_status':('RELEASE','LANCAMENTO'),
 'cloth_avatar':('AIR NOMAD AVATAR','AVATAR NOMADE DO AR'),
 'trident_ready':('TRIDENT RECLAIMED','TRIDENTE RECUPERADO'),
 'trident_out':('TRIDENT OUT - MOVE CLOSE TO RECOVER','TRIDENTE FORA - CHEGUE PERTO PARA RECUPERAR')
}

def main():
    en = D['LANGS']['en']; pt = D['LANGS']['pt-br']
    ui_keys = [k for k,v in en.items() if isinstance(v,str)]
    ui_keys += [k for k in EXTRA if k not in en]
    def ident(k): return re.sub(r'[^A-Za-z0-9_]','_',k)
    hdr = ['// GENERATED by tools/gen_data.py - do not edit','#pragma once','enum StrId {']
    for k in ui_keys: hdr.append('  S_%s,' % ident(k))
    hdr += ['  S_COUNT','};','']
    open(os.path.join(ROOT,'source','strings_gen.h'),'w').write('\n'.join(hdr))

    o = ['// GENERATED by tools/gen_data.py - do not edit','#include "data.h"','']
    for lang,tbl in (('EN',en),('PT',pt)):
        o.append('static const char* const UI_%s[S_COUNT] = {' % lang)
        for k in ui_keys:
            if k in EXTRA and k not in tbl: v = EXTRA[k][0 if lang=='EN' else 1]
            else: v = tbl.get(k, '')
            o.append('  %s, // %s' % (esc(v), k))
        o.append('};')
    o.append('const char* const* const UI_TABLE[LANG_COUNT] = { UI_EN, UI_PT };\n')

    spells = D['SPELLS']
    assert len(spells) == 147, 'expected 144 spells; update source/data.h and tests with canonical data'
    o.append('const int SPELL_COUNT = %d;' % len(spells))
    o.append('const SpellDef SPELLS[%d] = {' % len(spells))
    for s in spells:
        key=(s['name'], s['season']); assert key in M, key
        assert len(M[key]) <= 3, 'spell %s has more than three effects' % (s['name'],)
        effs = list(M[key])
        while len(effs)<3: effs.append(('NONE',0))
        es=[]
        for e in effs:
            e = list(e)+[0]*(4-len(e))
            es.append('{E_%s,%s,%s,%s}' % (e[0],fl(e[1]),fl(e[2]),fl(e[3])))
        max_level = 5 if any(e[0] in LEVEL_FIVE_EFFECTS for e in effs) else LEVEL_CAP_OVERRIDES.get(s['name'], ABSOLUTE_MAX_SPELL_LEVEL)
        sc = {'global':'SC_GLOBAL','element':'SC_ELEMENT','season':'SC_SEASON'}[s['scope']]
        se = ('SEA_'+s['season']) if s['season'] else 'SEA_NONE'
        o.append('  {EL_%s,TY_%s,ACT_%s,R_%s,%s,%s,%d,{%s}}, // %s' % (s['element'].upper(),s['type'].upper(),s['activation'].upper(),
                 s['rarity'].upper(),sc,se,max_level,','.join(es),s['name'].replace('\\','')))
    o.append('};')
    ptsp = pt['spell']
    def spstats(x): return ', '.join(x) if isinstance(x,list) else (x or '')
    for lang in ('EN','PT'):
        o.append('static const SpellText SPT_%s[%d] = {' % (lang,len(spells)))
        for s in spells:
            if lang=='EN': n,d,st = s['name'],s['desc'],spstats(s['stats'])
            else:
                p = ptsp.get(s['name'])
                n,d,st = (p['n'],p['d'],spstats(p['s'])) if p else ('','','')
            o.append('  {%s,%s,%s},' % (esc(n),esc(d),esc(st)))
        o.append('};')
    o.append('const SpellText* const SPELL_TEXT[LANG_COUNT] = { SPT_EN, SPT_PT };')
    for lang,code in (('EN','en'),('PT','pt-br')):
        sl = D['LORE'][code]['spells']
        o.append('static const char* const SLORE_%s[%d] = {' % (lang,len(spells)))
        for s in spells: o.append('  %s,' % (esc(sl[s['name']]) if s['name'] in sl else '""'))
        o.append('};')
    o.append('const char* const* const SPELL_LORE[LANG_COUNT] = { SLORE_EN, SLORE_PT };\n')

    EN = D['EN']
    order = sorted(range(len(EN)), key=lambda i:(SEAS.index(EN[i]['biome']), i))
    EN = [EN[i] for i in order]
    keys = [e['key'] for e in EN]
    o.append('const int ENEMY_COUNT = %d;' % len(EN))
    o.append('const EnemyDef ENEMIES[%d] = {' % len(EN))
    for e in EN:
        b = '|'.join('B_'+x for x in BEH.get(e['key'],[])) or '0'
        o.append('  {%s,SEA_%s,%d,%d,%s,%s,%s,%s}, // %s' % (esc(e['key']),e['biome'],e['hp'],e['dmg'],SPD[e['spd']],ATK[e['atk']],RNG[e['range']],b,e['name']))
    o.append('};')
    pte = pt['enemy']
    for lang in ('EN','PT'):
        o.append('static const EnemyText ENT_%s[%d] = {' % (lang,len(EN)))
        for e in EN:
            if lang=='EN': n,d,sp = e['name'],e['desc'],e['sp']
            else: p=pte[e['key']]; n,d,sp = p['n'],p['d'],p['sp']
            o.append('  {%s,%s,%s},' % (esc(n),esc(d),esc(sp)))
        o.append('};')
    o.append('const EnemyText* const ENEMY_TEXT[LANG_COUNT] = { ENT_EN, ENT_PT };')
    for lang,code in (('EN','en'),('PT','pt-br')):
        el = D['LORE'][code]['enemies']
        o.append('static const char* const ELORE_%s[%d] = {' % (lang,len(EN)))
        for e in EN: o.append('  %s,' % (esc(el[e['key']]) if e['key'] in el else '""'))
        o.append('};')
    o.append('const char* const* const ENEMY_LORE[LANG_COUNT] = { ELORE_EN, ELORE_PT };\n')

    def emit(name, svg):
        r = rects(svg)
        if not r: return 0
        o.append('static const PixRect %s[] = {%s};' % (name, ','.join('{%d,%d,%d,%d,0x%06X}'%t for t in r)))
        return len(r)
        
    arts=[]
    for e in EN:
        n = emit('ART_E_'+e['key'], A['ENEMY'][e['key']]); arts.append(('ART_E_'+e['key'],n))
    o.append('const PixArt ART_ENEMY[%d] = {%s};' % (len(EN), ','.join('{%s,%d}'%a for a in arts)))
    
    hats = ['helm','wizard','leaf','crown','none','batman','joker','terraria','mario','ironman','avatar']; faces=['beard','glasses','scar','freckles','none','dwarf_beard']
    ha=[]
    for h in hats: n=emit('ART_H_'+h,A['HAT'][h]); ha.append(('ART_H_'+h,n))
    o.append('const PixArt ART_HAT[11] = {%s};' % ','.join('{%s,%d}'%a for a in ha))
    fa=[]
    for f in faces:
        svg=A['FACE'][f]
        if not rects(svg): fa.append(('nullptr',0)); continue
        n=emit('ART_F_'+f,svg); fa.append(('ART_F_'+f,n))
    o.append('const PixArt ART_FACE[6] = {%s};' % ','.join('{%s,%d}'%a for a in fa))
    
    icons = ['leaf','pyr','snow','peak','ecl','lock','book','heart','gem','shield','bolt','clover','star','skull','clock','trophy','dagger','bow','axe','staff','shotgun','pistol','trident','chest','bug','sprout','katana','machine_gun']
    ia=[]
    for ic in icons:
        n=emit('ART_I_'+ic,A['ICO'][ic]); ia.append(('ART_I_'+ic,n))
    o.append('const PixArt ART_ICON[%d] = {%s};' % (len(icons),','.join('{%s,%d}'%a for a in ia)))
    
    open(os.path.join(ROOT,'source','data_gen.cpp'),'w',encoding='utf-8').write('\n'.join(o)+'\n')
    print('kinds used:', ' '.join(KINDS))
    print('enemies:', ' '.join(keys))

if __name__=='__main__': main()
