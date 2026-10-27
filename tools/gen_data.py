#!/usr/bin/env python3
"""Generates source/data_gen.cpp + source/strings_gen.h from tools/data/*.json
(exported from the Abyssal Eclipse HTML mockup).  Edit THIS file, never the output."""
import json, re, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
D = json.load(open(os.path.join(HERE, 'data', 'mock_data.json'), encoding='utf-8'))
A = json.load(open(os.path.join(HERE, 'data', 'art.json'), encoding='utf-8'))

SEAS = ['FOREST','DESERT','SNOW','MOUNTAIN','ECLIPSE']

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
 'new_label':('NEW!', 'NOVO!')
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
    o.append('const int SPELL_COUNT = %d;' % len(spells))
    o.append('const SpellDef SPELLS[%d] = {' % len(spells))
    for s in spells:
        key=(s['name'], s['season']); assert key in M, key
        effs = list(M[key])[:3]
        while len(effs)<3: effs.append(('NONE',0))
        es=[]
        for e in effs:
            e = list(e)+[0]*(4-len(e))
            es.append('{E_%s,%s,%s,%s}' % (e[0],fl(e[1]),fl(e[2]),fl(e[3])))
        sc = {'global':'SC_GLOBAL','element':'SC_ELEMENT','season':'SC_SEASON'}[s['scope']]
        se = ('SEA_'+s['season']) if s['season'] else 'SEA_NONE'
        o.append('  {EL_%s,TY_%s,ACT_%s,R_%s,%s,%s,{%s}}, // %s' % (s['element'].upper(),s['type'].upper(),s['activation'].upper(),
                 s['rarity'].upper(),sc,se,','.join(es),s['name'].replace('\\','')))
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

    def rects(svg):
        out=[]
        # Determine stroke color for lines (default to white)
        stroke_match = re.search(r'stroke="#([0-9A-Fa-f]{6})"', svg)
        line_color = int(stroke_match.group(1), 16) if stroke_match else 0xFFFFFF

        # Parse Rects
        for m in re.finditer(r'<rect x="(\d+)" y="(\d+)" width="(\d+)" height="(\d+)" fill="#([0-9A-Fa-f]{6})"',svg):
            x,y,w,h,c = m.groups()
            out.append((int(x),int(y),int(w),int(h),int(c,16)))
            
        # Parse Circles
        for m in re.finditer(r'<circle cx="([\d\.]+)" cy="([\d\.]+)" r="([\d\.]+)" fill="#([0-9A-Fa-f]{6})"',svg):
            cx, cy, r, c = m.groups()
            cx, cy, r, c = float(cx), float(cy), float(r), int(c, 16)
            x, y, w, h = int(cx - r), int(cy - r), int(r * 2), int(r * 2)
            if w > 0 and h > 0:
                out.append((x, y, w, h, c))
                
        # Parse Lines (convert to series of 1x1 rects)
        for m in re.finditer(r'<line x1="([\d\.]+)" y1="([\d\.]+)" x2="([\d\.]+)" y2="([\d\.]+)"',svg):
            x1, y1, x2, y2 = map(float, m.groups())
            steps = max(abs(int(x2-x1)), abs(int(y2-y1)))
            for i in range(steps + 1):
                t = i / max(1, steps)
                px, py = int(x1 + (x2-x1)*t), int(y1 + (y2-y1)*t)
                out.append((px, py, 1, 1, line_color))
                
        return out

    def emit(name, svg):
        r = rects(svg)
        if not r: return 0
        o.append('static const PixRect %s[] = {%s};' % (name, ','.join('{%d,%d,%d,%d,0x%06X}'%t for t in r)))
        return len(r)
        
    arts=[]
    for e in EN:
        n = emit('ART_E_'+e['key'], A['ENEMY'][e['key']]); arts.append(('ART_E_'+e['key'],n))
    o.append('const PixArt ART_ENEMY[%d] = {%s};' % (len(EN), ','.join('{%s,%d}'%a for a in arts)))
    
    hats = ['helm','wizard','leaf','crown','none']; faces=['beard','glasses','scar','freckles','none']
    ha=[]
    for h in hats: n=emit('ART_H_'+h,A['HAT'][h]); ha.append(('ART_H_'+h,n))
    o.append('const PixArt ART_HAT[5] = {%s};' % ','.join('{%s,%d}'%a for a in ha))
    fa=[]
    for f in faces:
        svg=A['FACE'][f]
        if not rects(svg): fa.append(('nullptr',0)); continue
        n=emit('ART_F_'+f,svg); fa.append(('ART_F_'+f,n))
    o.append('const PixArt ART_FACE[5] = {%s};' % ','.join('{%s,%d}'%a for a in fa))
    
    icons = ['leaf','pyr','snow','peak','ecl','lock','book','heart','gem','shield','bolt','clover','star','skull','clock','trophy','dagger','bow','axe','staff','chest','bug','sprout']
    ia=[]
    for ic in icons:
        n=emit('ART_I_'+ic,A['ICO'][ic]); ia.append(('ART_I_'+ic,n))
    o.append('const PixArt ART_ICON[%d] = {%s};' % (len(icons),','.join('{%s,%d}'%a for a in ia)))
    
    open(os.path.join(ROOT,'source','data_gen.cpp'),'w',encoding='utf-8').write('\n'.join(o)+'\n')
    print('kinds used:', ' '.join(KINDS))
    print('enemies:', ' '.join(keys))

if __name__=='__main__': main()