#include "sim_internal.h"
#include "gfx.h"
#include "i18n.h"
#include <string.h>
#include <stdio.h>
using namespace bal;

// ================================================================================ helpers
static void setToast(Sim& s, const char* t) { snprintf(s.toast, sizeof s.toast, "%s", t); s.toastT = 2.0f; }
static void spellCast(Sim& s) { if (s.runSpellsCasted != 0xFFFFFFFFu) ++s.runSpellsCasted; }

static Fx* newFx(Sim& s) {
  for (int i = 0; i < MAX_FX; ++i) if (s.fx[i].kind == FX_NONE) return &s.fx[i];
  return nullptr;
}
void fxText(Sim& s, float x, float y, const char* t, int col) {
  Fx* f = newFx(s); if (!f) return;
  memset(f, 0, sizeof *f); f->kind = FX_TEXT; f->col = (u8)col; f->x = x; f->y = y; f->life = 0.7f;
  snprintf(f->txt, sizeof f->txt, "%s", t);
}
void fxRing(Sim& s, float x, float y, float r, int col, float life) {
  Fx* f = newFx(s); if (!f) return;
  memset(f, 0, sizeof *f); f->kind = FX_RING; f->col = (u8)col; f->x = x; f->y = y; f->r = r; f->life = life;
}
void fxDisc(Sim& s, float x, float y, float r, int col, float life) {
  Fx* f = newFx(s); if (!f) return;
  memset(f, 0, sizeof *f); f->kind = FX_DISC; f->col = (u8)col; f->x = x; f->y = y; f->r = r; f->life = life;
}
static void fxSlash(Sim& s, float x, float y, float ang, float r) {
  Fx* f = newFx(s); if (!f) return;
  memset(f, 0, sizeof *f); f->kind = FX_SLASH; f->x = x; f->y = y; f->ang = ang; f->r = r; f->life = 0.18f;
}
void fxBolt(Sim& s, float x, float y) {
  Fx* f = newFx(s); if (!f) return;
  memset(f, 0, sizeof *f); f->kind = FX_BOLT; f->col = 6; f->x = x; f->y = y; f->life = 0.18f;
}
Proj* newProj(Sim& s) {
  for (int i = 0; i < MAX_PROJ; ++i) if (!s.pr[i].alive) { memset(&s.pr[i], 0, sizeof(Proj)); s.pr[i].alive = true; return &s.pr[i]; }
  return nullptr;
}
void newZone(Sim& s, u8 type, float x, float y, float r, float t, float dps, float a) {
  int best = -1; float bt = 1e8f;
  for (int i = 0; i < MAX_ZONES; ++i) {
    if (s.zn[i].type == Z_NONE) { best = i; break; }
    if (s.zn[i].type != Z_BRAMBLE && s.zn[i].type != Z_FROST && s.zn[i].t < bt) { bt = s.zn[i].t; best = i; }
  }
  if (best < 0) return;
  s.zn[best] = Zone{type, x, y, r, t, dps, a};
}
static int nearestEnemy(Sim& s, float x, float y, float maxR, int exclude) {
  int best = -1; float bd = maxR * maxR;
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    const Enemy& e = s.en[i];
    if (!e.alive || i == exclude) continue;
    if ((ENEMIES[e.type].beh & B_INVIS) && dist2(e.x, e.y, s.p.x, s.p.y) > sq(70)) continue;   // invisible until close
    float d = dist2(x, y, e.x, e.y);
    if (d < bd) { bd = d; best = i; }
  }
  return best;
}
static void heal(Sim& s, float v) {
  v *= s.m.healMul;
  if (v > 0) s.p.hp = minT(s.p.maxhp, s.p.hp + v);
}
int statValue(const Sim& s, int st) { return BASE_STAT[st] + s.alloc[st] - s.drain[st]; }
static int pts(const Sim& s, int st) { return maxT(0, s.alloc[st] - s.drain[st]); }
int simSpellLevelOf(const Sim& s, int id) {
  for (int i = 0; i < s.nSlots; ++i) if (s.slots[i].id == id) return s.slots[i].lv;
  return 0;
}
const char* phaseName(Phase p) {
  static const char* n[] = {"PLAY", "CLEARED", "ALLOC", "SPELL", "DEAD", "VICTORY"};
  return n[p];
}

// ================================================================================ spell engine
static Eff scaleEff(Eff e, float m, int level) {
  const int rank = maxT(0, level - 1);
  switch (e.kind) {
    case E_MAXHP: case E_MAXHP_PCT: case E_MOVE_PCT: case E_ATKSPD_PCT: case E_CD_PCT: case E_CRIT: case E_CRITDMG:
    case E_FLATDMG: case E_DMG_PCT: case E_RES: case E_TAKEN_PCT: case E_RANGED_TAKEN_PCT: case E_DEBUFF_PCT:
    case E_LIFESTEAL: case E_REFLECT: case E_DODGE: case E_THROWSPD_PCT: case E_RANGE_PCT: case E_HEAL_PCT:
    case E_DEALT_PCT: case E_LOWHP_DMG: case E_VS_SLOWED: case E_VS_BURNING: case E_VS_WOUNDED: case E_VS_FROZEN:
    case E_CRITBONUS: case E_IMMUNE_SLOW: case E_HP_PER_WAVE: case E_LIGHTNING_PCT: case E_REVIVE:
    case E_AURA_PULL: case E_AURA_HEALKILL: case E_HIT_ROOT: case E_CRIT_HEAL: case E_ARMOR_IGNORE:
    case E_DRONE_MINIGUN: case E_SENTINEL: case E_KNOCKBACK: case E_WARMTH: case E_SHIELD_WAVE:
    case E_MIRROR: case E_ATTACKER_DMG: case E_EXSANG: case E_TRAIL_SPARK:
      e.a *= m; break;
    case E_AURA: e.a *= m; e.b *= m; e.c *= m; break;
    case E_AVATAR: e.a = maxT(2.0f, e.a - rank); e.b *= m; e.c *= m; break;
    case E_FAIRY: e.a *= m; e.b = maxT(2.0f, e.b / m); break;
    case E_TIME_STOP: e.a = maxT(8.0f, e.a - rank); e.b = minT(1.6f, e.b + rank * 0.08f); e.c = maxT(8.0f, e.c - rank * 0.75f); break;
    case E_KILLHEAL: case E_REGEN: e.a *= m; e.b = maxT(0.5f, e.b / m); break;
    case E_KILL_MAXHP: case E_KILL_HEALEVERY: e.a *= m; e.b = maxT(1.0f, e.b - rank); break;
    case E_HIT_SLOW: case E_HIT_FREEZE: case E_HIT_STUN: case E_HIT_BURN: case E_HIT_BLEED:
    case E_HIT_BONUS: case E_MARK: case E_CRIT_BURN: case E_BURN_TOUCH:
      e.a *= m; e.b *= m; break;
    case E_HIT_CHAIN: case E_HIT_BOLT:
      e.a *= m; e.b += rank; break;
    case E_HIT_BURST: case E_KILL_BURST: case E_KILL_EXPLODE_BURNING: case E_EXPLODE:
      e.a *= m; e.b *= m; break;
    case E_KILL_POISON: case E_KILL_CLOUD: case E_KILL_SLOWAOE: case E_KILL_CURSE:
      e.a *= m; e.b *= m; e.c *= m; break;
    case E_KILL_CHAIN: e.a *= m; e.b *= m; e.c *= m; break;
    case E_CRIT_CHAIN: e.a += rank; e.b *= m; break;
    case E_NTH_RING:
      e.a = maxT(2.0f, e.a - rank); if (e.b <= 0) e.b = 12; if (e.c <= 0) e.c = 0.25f; e.c *= m; break;
    case E_NTH_DOUBLE:
      e.a = maxT(2.0f, e.a - rank); if (e.b <= 0) e.b = 1.0f; e.b *= m; break;
    case E_NTH_STRIKE3:
      e.a = maxT(2.0f, e.a - rank); if (e.b <= 0) e.b = 10; else e.b *= m;
      if (e.c <= 0) e.c = 3;
      e.c += rank; break;
    case E_NTH_BLAST: case E_NTH_STUN: case E_NTH_BOLT:
      e.a = maxT(2.0f, e.a - rank); e.b *= m; e.c *= m; break;
    case E_PERIODIC_FLARE: e.a = maxT(3.0f, e.a / m); e.b *= m; break;
    case E_PERIODIC_BOMB: e.a = maxT(4.0f, e.a / m); e.b *= m; e.c *= m; break;
    case E_CHAINPLUS: case E_BLOCK_WAVE: case E_PIERCE: case E_BOUNCE_WALL: case E_HOMING: break;
    case E_BOUNCE_ENEMY: e.b *= m; break;
    case E_SPLIT: e.a += rank; e.b *= m; break;
    case E_ORBIT: case E_WHIRL: e.a *= m; if (e.kind == E_WHIRL) e.b *= m; break;
    case E_TRAIL_FIRE: e.a *= m; e.b *= m; break;
    case E_AURA_WEAKEN: case E_AURA_MISS: e.a *= m; e.b *= m; break;
    case E_KILL_MOVE: e.a *= m; e.b *= m; break;
    default: break;
  }
  return e;
}

void recompute(Sim& s) {
  Mods& m = s.m; Player& p = s.p;
  float oldMax = p.maxhp;
  memset((void*)&m, 0, sizeof m);
  float flatHp = 0, hpPct = 0, movePct = 0, atkPct = 0, cdPct = 0, crit = 0, critDmg = 0, dmgPct = 0, res = 0;
  float taken = 0, rTaken = 0, debuff = 0, heal = 0, dealt = 0, thr = 0, rng = 0;
  float crown = 0;
  for (int i = 0; i < s.nSlots; ++i) if (s.slots[i].id >= 0)
    for (int k = 0; k < 3; ++k) if (SPELLS[s.slots[i].id].eff[k].kind == E_ALLNUM_PCT)
      crown += SPELLS[s.slots[i].id].eff[k].a * (1 + LV_BONUS * (s.slots[i].lv - 1));
  for (int i = 0; i < s.nSlots; ++i) {
    if (s.slots[i].id < 0) continue;
    float mult = (1 + LV_BONUS * (s.slots[i].lv - 1)) * (1 + crown / 100.0f);
    for (int k = 0; k < 3; ++k) {
      Eff e = SPELLS[s.slots[i].id].eff[k];
      if (e.kind == E_NONE || e.kind == E_ALLNUM_PCT) continue;
      const int lv = s.slots[i].lv;
      e = scaleEff(e, mult, lv);
      switch (e.kind) {
        case E_MAXHP: flatHp += e.a; break;
        case E_MAXHP_PCT: hpPct += e.a; break;
        case E_MOVE_PCT: movePct += e.a; break;
        case E_ATKSPD_PCT: atkPct += e.a; break;
        case E_CD_PCT: cdPct += e.a; break;
        case E_CRIT: crit += e.a; break;
        case E_CRITDMG: critDmg += e.a; break;
        case E_FLATDMG: m.flat += e.a; break;
        case E_DMG_PCT: dmgPct += e.a; break;
        case E_RES: res += e.a; break;
        case E_TAKEN_PCT: taken += e.a; break;
        case E_RANGED_TAKEN_PCT: rTaken += e.a; break;
        case E_DEBUFF_PCT: debuff += e.a; break;
        case E_LIFESTEAL: m.lifesteal += e.a; break;
        case E_KILLHEAL: m.killHeal = maxT(m.killHeal, e.a); m.killHealCd = (m.killHealCd == 0) ? e.b : minT(m.killHealCd, e.b); break;
        case E_REGEN: m.regenHp += e.a; m.regenInt = (m.regenInt == 0) ? e.b : minT(m.regenInt, e.b); break;
        case E_REFLECT: m.reflect += e.a; break;
        case E_DODGE: m.dodge += e.a; break;
        case E_THROWSPD_PCT: thr += e.a; break;
        case E_RANGE_PCT: rng += e.a; break;
        case E_HEAL_PCT: heal += e.a; break;
        case E_DEALT_PCT: dealt += e.a; break;
        case E_LOWHP_DMG: m.lowHpBonus += e.a; m.lowHpThresh = e.b; break;
        case E_VS_SLOWED: m.vsSlowed += e.a; break;
        case E_VS_BURNING: m.vsBurning += e.a; break;
        case E_VS_WOUNDED: m.vsWounded += e.a; break;
        case E_VS_FROZEN: m.vsFrozen += e.a; break;
        case E_CRITBONUS: m.critBonus += e.a; break;
        case E_IMMUNE_SLOW: m.immuneSlow = true; movePct += maxT(0.0f, e.a - 1.0f) * 10.0f; break;
        case E_HP_PER_WAVE: m.hpPerWave += e.a; break;
        case E_LIGHTNING_PCT: m.lightning += e.a; break;
        case E_CHAINPLUS: m.chainPlus += maxT(0, (int)e.a) + lv - 1; break;
        case E_SHIELD_WAVE: m.shieldWave += e.a; break;
        case E_BLOCK_WAVE: m.blockWave += maxT(1, (int)e.a) + lv - 1; break;
        case E_REVIVE: m.revivePct = maxT(m.revivePct, e.a); break;
        case E_MIRROR: m.mirror += e.a; break;
        case E_ATTACKER_DMG: m.attackerDmg += e.a; break;
        case E_BURN_TOUCH: m.burnTouchDps += e.a; m.burnTouchDur = e.b; break;
        case E_WARMTH: m.warmth += e.a; break;
        case E_ARMOR_IGNORE: m.armorIgnore = true; m.armorBonus += maxT(0.0f, e.a - 1.0f) * 25.0f; break;
        case E_DRONE_MINIGUN: m.hasDrone = true; m.dronePower += e.a; break;
        case E_SENTINEL: m.hasSentinel = true; m.sentinelPower += e.a; break;
        case E_AVATAR: if (m.nEv < MAX_EV) m.ev[m.nEv++] = Ev{e.kind, e.a, e.b, e.c}; break;
        case E_FAIRY: m.hasFairy = true; m.fairyPower += e.a; break;
        case E_TIME_STOP: if (m.nEv < MAX_EV) m.ev[m.nEv++] = Ev{e.kind, e.a, e.b, e.c}; break;
        case E_PIERCE: m.pierce += maxT(0, (int)e.a - 1) + lv - 1; break;
        case E_BOUNCE_ENEMY: m.bounceE += maxT(1, (int)e.a) + lv - 1; m.bounceDmg = e.b; break;
        case E_BOUNCE_WALL: m.bounceW += maxT(1, (int)e.a) + lv - 1; break;
        case E_HOMING: m.homing = true; m.homingLevel = maxT(m.homingLevel, lv); break;
        case E_SPLIT: m.split += (int)e.a; m.splitDmg = e.b; break;
        case E_EXPLODE: m.explodeDmg += e.a; m.explodeR = maxT(m.explodeR, e.b); break;
        case E_KNOCKBACK: m.knock = maxT(m.knock, e.a); break;
        case E_ORBIT: m.nOrbit += (int)e.b + (s.slots[i].lv - 1); m.orbitDps += e.a; break;
        case E_WHIRL: m.whirlDps += e.a; m.whirlDur = e.b; break;
        default: if (m.nEv < MAX_EV) m.ev[m.nEv++] = Ev{e.kind, e.a, e.b, e.c}; break;
      }
    }
  }
  if (!m.hasSentinel) for (int i = 0; i < 2; ++i) s.sentinels[i].active = false;
  const float STR = (float)pts(s, ST_STR), SPD = (float)pts(s, ST_SPD), LUCK = (float)pts(s, ST_LUCK), INT = (float)pts(s, ST_INT);
  const float resPts = (float)pts(s, ST_RES) + res;
  m.dmgMul = maxT(0.1f, 1 + 0.08f * STR + dmgPct / 100.0f);
  m.sizeMul = 1 + 0.02f * STR;
  m.maxHp = maxT(10.0f, (BASE_HP + 6 * resPts + flatHp + p.maxHpBonus) * (1 + hpPct / 100.0f));
  m.taken = clampT((1 + taken / 100.0f) * (1 - 0.02f * resPts), 0.2f, 2.0f);
  m.rangedTaken = clampT(1 + rTaken / 100.0f, 0.2f, 2.0f);
  m.debuff = clampT(1 + debuff / 100.0f, 0.1f, 2.0f);
  m.move = maxT(20.0f, PLAYER_SPEED * (1 + 0.04f * SPD + movePct / 100.0f));
  m.atkSpd = maxT(0.3f, 1 + 0.04f * SPD + atkPct / 100.0f);
  m.cdMul = maxT(0.3f, 1 - 0.04f * INT - cdPct / 100.0f);
  m.crit = BASE_CRIT + 3 * LUCK + crit;
  m.critDmg = critDmg;
  m.expMul = EXP_GAIN_MUL * (1 + 0.08f * LUCK); m.rareBonus = 2 * LUCK; m.aoeMul = 1 + 0.06f * INT;
  m.projSpd = 1 + thr / 100.0f; m.rangeMul = 1 + rng / 100.0f;
  m.healMul = maxT(0.0f, 1 + heal / 100.0f); m.dealt = maxT(0.2f, 1 + dealt / 100.0f);
  m.allNumPct = crown;
  m.nOrbit = minT(m.nOrbit, (int)MAX_ORBIT); s.nOrb = m.nOrbit;
  p.maxhp = m.maxHp;
  if (oldMax > 0 && p.maxhp > oldMax) p.hp += p.maxhp - oldMax;
  if (p.hp > p.maxhp) p.hp = p.maxhp;
}

// ================================================================================ damage pipeline
static void applyHitEvents(Sim& s, int ei, float dealt, bool crit, float dx, float dy);

void dotEnemy(Sim& s, int ei, float dmg) {
  Enemy& e = s.en[ei];
  if (!e.alive) return;
  e.hp -= dmg;
  if (e.hp <= 0) killEnemy(s, ei, 0);
}

void hitEnemy(Sim& s, int ei, float base, float dx, float dy, bool events, bool fromProj) {
  Enemy& e = s.en[ei];
  if (!e.alive) return;
  const EnemyDef& def = ENEMIES[e.type];
  Mods& m = s.m;
  float d = base;
  bool crit = false;
  if (events) {
    crit = s.rng.chance(m.crit);
    if (crit) d *= CRIT_MUL + (m.critDmg + m.critBonus) / 100.0f;
    float bonus = 0;
    if (e.slowT > 0 || e.rootT > 0) bonus += m.vsSlowed;
    if (e.burnT > 0) bonus += m.vsBurning;
    if (e.hp < e.maxhp) bonus += m.vsWounded;
    if (e.freezeT > 0 || e.stunT > 0) bonus += m.vsFrozen;
    if (m.lowHpBonus > 0 && s.p.hp <= s.p.maxhp * m.lowHpThresh / 100.0f) bonus += m.lowHpBonus;
    d *= 1 + bonus / 100.0f;
    if (e.markT > 0) d *= 1 + e.markPct / 100.0f;
    for (int i = 0; i < m.nEv; ++i) if (m.ev[i].kind == E_EXSANG && s.rng.chance(m.ev[i].a)) { d *= 2; heal(s, 1); }
  }
  if ((def.beh & B_SHIELD) && fromProj) {                 // frontal shield: only hits from behind land
    if (dx * e.dirx + dy * e.diry <= 0) { fxText(s, e.x, e.y - 10, "BLOCK", 0); audio::sfx(audio::SFX_UI); return; }
  }
  if (def.beh & B_ARMOR) {
    if (!m.armorIgnore) d = maxT(1.0f, d - 1);
    else d *= 1.0f + m.armorBonus / 100.0f;
  }
  e.hp -= d; e.flash = 0.08f;
  if (crit || d >= 12) {
    char b[8]; snprintf(b, sizeof b, "%d", (int)d); fxText(s, e.x, e.y - 8, b, crit ? 7 : 0);
    if (crit) fxRing(s, e.x, e.y, 16, 7, 0.25f);
  }
  if (m.lifesteal > 0 && s.p.lsBudget > 0) { float h = minT(d * m.lifesteal / 100.0f, s.p.lsBudget); s.p.lsBudget -= h; heal(s, h); }
  audio::sfx(audio::SFX_HIT);
  if (events) applyHitEvents(s, ei, d, crit, dx, dy);
  if (e.hp <= 0) killEnemy(s, ei, 0);
}

void aoeDamage(Sim& s, float x, float y, float r, float dmg) {
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    Enemy& e = s.en[i];
    if (e.alive && dist2(x, y, e.x, e.y) < sq(r + enemyTouchR(e))) hitEnemy(s, i, dmg, 0, 0, false, false);
  }
}

static void statusOn(Sim& s, Enemy& e, float slowPct, float slowT, float freeze, float stun, float root) {
  const bool boss = (ENEMIES[e.type].beh & B_BOSS) != 0;
  const float k = boss ? 0.3f : 1.0f;
  (void)s;
  if (slowT > 0) { e.slowPct = maxT(e.slowPct, slowPct); e.slowT = maxT(e.slowT, slowT * (boss ? 0.5f : 1.0f)); }
  if (freeze > 0) e.freezeT = maxT(e.freezeT, freeze * k);
  if (stun > 0) e.stunT = maxT(e.stunT, stun * k);
  if (root > 0) e.rootT = maxT(e.rootT, root * k);
}

static void applyHitEvents(Sim& s, int ei, float dealt, bool crit, float dx, float dy) {
  Mods& m = s.m;
  Enemy& e = s.en[ei];
  const float lm = 1 + m.lightning / 100.0f;
  for (int i = 0; i < m.nEv; ++i) {
    const Ev& v = m.ev[i];
    switch (v.kind) {
      case E_HIT_SLOW: spellCast(s); statusOn(s, e, v.a, v.b, 0, 0, 0); break;
      case E_HIT_FREEZE: if (s.rng.chance(v.a)) { spellCast(s); statusOn(s, e, 0, 0, v.b, 0, 0); } break;
      case E_HIT_STUN: if (crit && s.rng.chance(v.a)) { spellCast(s); statusOn(s, e, 0, 0, 0, v.b, 0); } break;
      case E_HIT_ROOT: if (crit) { spellCast(s); statusOn(s, e, 0, 0, 0, 0, v.a); } break;
      case E_HIT_BURN: spellCast(s); e.burnDps = maxT(e.burnDps, v.a); e.burnT = maxT(e.burnT, v.b); break;
      case E_HIT_BLEED: spellCast(s); e.bleedDps = maxT(e.bleedDps, v.a); e.bleedT = maxT(e.bleedT, v.b); break;
      case E_HIT_BONUS: if (s.rng.chance(v.a)) { spellCast(s); e.hp -= v.b * lm; fxText(s, e.x + 6, e.y - 12, "ZAP", 6); } break;
      case E_HIT_BURST: spellCast(s); aoeDamage(s, e.x, e.y, v.b * m.aoeMul, v.a); fxRing(s, e.x, e.y, v.b * m.aoeMul, 3, 0.25f); break;
      case E_HIT_CHAIN: case E_HIT_BOLT: {
        spellCast(s);
        int n = (int)v.b + m.chainPlus, from = ei; float px = e.x, py = e.y;
        for (int c = 0; c < n; ++c) {
          int t = nearestEnemy(s, px, py, 64, from);
          if (t < 0) break;
          fxBolt(s, s.en[t].x, s.en[t].y - 6);
          dotEnemy(s, t, v.a * lm); from = t; px = s.en[t].x; py = s.en[t].y;
        }
      } break;
      case E_CRIT_CHAIN: if (crit) {
        spellCast(s);
        int n = (int)v.a + m.chainPlus, from = ei; float px = e.x, py = e.y;
        for (int c = 0; c < n; ++c) {
          int t = nearestEnemy(s, px, py, 70, from);
          if (t < 0) break;
          fxBolt(s, s.en[t].x, s.en[t].y - 6);
          dotEnemy(s, t, dealt * v.b / 100.0f * lm); from = t; px = s.en[t].x; py = s.en[t].y;
        }
      } break;
      case E_CRIT_BURN: if (crit) { spellCast(s); e.burnDps = maxT(e.burnDps, v.a); e.burnT = maxT(e.burnT, v.b); } break;
      case E_CRIT_HEAL: if (crit && s.p.critHealT <= 0) { spellCast(s); heal(s, v.a); s.p.critHealT = 1.0f; } break;
      case E_EXSANG: break;
      case E_MARK: spellCast(s); e.markPct = maxT(e.markPct, v.a); e.markT = v.b; break;
      default: break;
    }
  }
  if (m.knock > 0 && !(ENEMIES[e.type].beh & B_BOSS)) {
    float l = sqrtf(dx * dx + dy * dy) + 0.001f;
    e.kx += dx / l * 90 * m.knock; e.ky += dy / l * 90 * m.knock;
  }
  if (m.whirlDps > 0) { spellCast(s); newZone(s, Z_VORTEX, e.x, e.y, 22, m.whirlDur, m.whirlDps, 0); }
}

// ================================================================================ kills, gems, healing
static void spawnGem(Sim& s, float x, float y, float v) {
  for (int i = 0; i < MAX_GEMS; ++i) if (!s.gem[i].alive) { s.gem[i] = Gem{true, x, y, v}; return; }
  s.gem[s.rng.i(MAX_GEMS)].v += v;                                 // pool full: merge
}
static void gainExp(Sim& s, float v) {
  s.p.exp += v * s.m.expMul;
  while (s.p.exp >= expNeed(s.p.level)) {
    s.p.exp -= (float)expNeed(s.p.level);
    s.p.level++; s.points++;
    heal(s, s.p.maxhp * 0.1f);
    fxText(s, s.p.x, s.p.y - 16, T(S_level_up), 7);
    audio::sfx(audio::SFX_LEVEL);
  }
}

void killEnemy(Sim& s, int ei, int depth) {
  Enemy& e = s.en[ei];
  if (!e.alive) return;
  const EnemyDef& def = ENEMIES[e.type];
  Mods& m = s.m;
  if ((def.beh & B_REVIVE) && !e.revived) {
    e.revived = true; e.hp = e.maxhp * 0.5f; e.stunT = 0.8f; e.burnT = e.bleedT = 0;
    fxText(s, e.x, e.y - 10, "REVIVE", 2); return;
  }
  e.alive = false;
  const bool boss = (def.beh & B_BOSS) != 0;
  s.totalKills++; s.p.kills++;
  if (s.runKills[e.type] < 65535) s.runKills[e.type]++;
  spawnGem(s, e.x, e.y, boss ? 25.0f : maxT(1.0f, (float)def.hp / 8.0f * (e.small ? 0.5f : 1.0f)));
  audio::sfx(audio::SFX_KILL);
  fxDisc(s, e.x, e.y, 9, 0, 0.16f);
  if ((def.beh & B_SPLIT_DEATH) && !e.small) {
    for (int k = 0; k < 2; ++k) {
      int n = spawnEnemy(s, e.type, e.x + (k ? 7 : -7), e.y, true);
      if (n >= 0) { s.en[n].hp = s.en[n].maxhp = e.maxhp * 0.4f; s.en[n].split = true; }
    }
  }
  if (s.phase != PH_PLAY) return;
  if (m.killHeal > 0 && s.p.killHealT <= 0) { spellCast(s); heal(s, m.killHeal); s.p.killHealT = m.killHealCd; }
  const float aoe = m.aoeMul;
  for (int i = 0; i < m.nEv; ++i) {
    const Ev& v = m.ev[i];
    switch (v.kind) {
      case E_KILL_BURST: spellCast(s); aoeDamage(s, e.x, e.y, v.b * aoe, v.a); fxRing(s, e.x, e.y, v.b * aoe, 2, 0.25f); break;
      case E_KILL_POISON: spellCast(s); newZone(s, Z_POISON, e.x, e.y, v.c * aoe, v.b, v.a / maxT(v.b, 0.1f), 0); break;
      case E_KILL_CLOUD: spellCast(s); newZone(s, Z_CLOUD, e.x, e.y, v.c * aoe, v.b, v.a, 0); break;
      case E_KILL_SLOWAOE:
        spellCast(s);
        for (int k = 0; k < MAX_ENEMIES; ++k) if (s.en[k].alive && dist2(e.x, e.y, s.en[k].x, s.en[k].y) < sq(v.c * aoe)) {
          s.en[k].slowPct = maxT(s.en[k].slowPct, v.a); s.en[k].slowT = maxT(s.en[k].slowT, v.b); }
        fxRing(s, e.x, e.y, v.c * aoe, 3, 0.3f); break;
      case E_KILL_CHAIN: if (depth < 2 && s.rng.chance(v.a)) {
        spellCast(s);
        int t = nearestEnemy(s, e.x, e.y, 70, ei);
        if (t >= 0) { fxBolt(s, s.en[t].x, s.en[t].y - 6); s.en[t].hp = 0; killEnemy(s, t, depth + 1); } } break;
      case E_KILL_MOVE: spellCast(s); s.p.moveBonus = minT(s.p.moveBonus + v.a, v.b); break;
      case E_KILL_MAXHP: if (++s.p.killsSinceMax >= (int)v.b) { spellCast(s); s.p.killsSinceMax = 0; s.p.maxHpBonus += v.a; recompute(s); } break;
      case E_KILL_CURSE: spellCast(s); newZone(s, Z_CURSE, e.x, e.y, v.b * aoe, 1.0f, 0, v.a); break;
      case E_KILL_EXPLODE_BURNING: if (e.burnT > 0) { spellCast(s); aoeDamage(s, e.x, e.y, v.b * aoe, v.a); fxRing(s, e.x, e.y, v.b * aoe, 2, 0.25f); } break;
      case E_KILL_HEALEVERY: if (++s.p.killsSinceHeal >= (int)v.b) { spellCast(s); s.p.killsSinceHeal = 0; heal(s, v.a); } break;
      case E_AURA_HEALKILL: if (dist2(e.x, e.y, s.p.x, s.p.y) < sq(56 * aoe)) { spellCast(s); heal(s, v.a); } break;
      default: break;
    }
  }
}

bool hurtPlayer(Sim& s, float dmg, bool ranged, Enemy* src, bool allowTiny) {
  Player& p = s.p; Mods& m = s.m;
  if (s.godMode || p.iframes > 0 || s.phase != PH_PLAY) return false;
  if (s.rng.chance(m.dodge)) { if (m.dodge > 0) spellCast(s); fxText(s, p.x, p.y - 12, "DODGE", 3); p.iframes = 0.25f; return false; }
  if (src && !ranged) for (int i = 0; i < m.nEv; ++i) if (m.ev[i].kind == E_AURA_MISS &&
      dist2(src->x, src->y, p.x, p.y) < sq(m.ev[i].b * m.aoeMul) && s.rng.chance(m.ev[i].a)) { spellCast(s); fxText(s, p.x, p.y - 12, "MISS", 0); p.iframes = 0.2f; return false; }
  if (p.blocks > 0) { p.blocks--; fxText(s, p.x, p.y - 12, "BLOCK", 0); p.iframes = IFRAMES; return false; }
  dmg *= m.taken;
  if (ranged) dmg *= m.rangedTaken;
  if (p.burnT > 0) dmg *= 1 - m.warmth / 100.0f;
  if (src) for (int i = 0; i < m.nEv; ++i) if (m.ev[i].kind == E_AURA_WEAKEN && dist2(src->x, src->y, p.x, p.y) < sq(m.ev[i].b * m.aoeMul)) { spellCast(s); dmg *= 1 - m.ev[i].a / 100.0f; }
  dmg = allowTiny ? maxT(0.0f, dmg) : maxT(0.5f, dmg);
  if (p.shield > 0) { float a = minT(p.shield, dmg); p.shield -= a; dmg -= a; if (dmg <= 0.01f) { p.iframes = IFRAMES * 0.5f; return true; } }
  p.hp -= dmg; p.iframes = IFRAMES; p.hurtFlash = 0.15f; s.shake = 0.15f;
  audio::sfx(audio::SFX_HURT);
  if (src) {
    int idx = (int)(src - s.en);
    if (idx >= 0 && idx < MAX_ENEMIES) {
      if (m.reflect > 0) { spellCast(s); dotEnemy(s, idx, dmg * m.reflect / 100.0f); }
      if (m.attackerDmg > 0) { spellCast(s); dotEnemy(s, idx, m.attackerDmg); }
      if (m.burnTouchDps > 0 && s.en[idx].alive) { spellCast(s); s.en[idx].burnDps = m.burnTouchDps; s.en[idx].burnT = m.burnTouchDur; }
    }
  }
  if (p.hp <= 0) {
    if (m.revivePct > 0 && !p.usedRevive) {
      spellCast(s); p.usedRevive = true; p.hp = p.maxhp * m.revivePct / 100.0f; p.iframes = 2.0f;
      fxText(s, p.x, p.y - 14, "REVIVE", 2); fxRing(s, p.x, p.y, 40, 2, 0.5f);
    } else { p.hp = 0; s.phase = PH_DEAD; s.phaseT = 0; audio::sfx(audio::SFX_DEATH); }
  }
  return true;
}

void damageSentinel(Sim& s, int index, float dmg) {
  if (index < 0 || index >= 2) return;
  Sentinel& sent = s.sentinels[index];
  if (!sent.active || sent.iframes > 0.0f) return;
  sent.hp -= dmg;
  sent.iframes = 0.4f;
  fxText(s, sent.x, sent.y - 12, "HIT", 6);
  if (sent.hp <= 0.0f) {
    sent.hp = 0.0f; sent.active = false;
    s.sentinelSpawnT = maxT(s.sentinelSpawnT, 5.0f);
    fxRing(s, sent.x, sent.y, 18, 5, 0.4f); fxText(s, sent.x, sent.y - 12, "DOWN", 1);
  }
}

// ================================================================================ weapon + projectiles
static void fireWeapon(Sim& s, int ti, float angOff, float dmgMul) {
  const WeaponStat& w = WEAPON[s.weapon];
  const Mods& m = s.m;
  const Enemy& t = s.en[ti];
  float ang = atan2f(t.y - s.p.y, t.x - s.p.x) + angOff;
  Proj* q = newProj(s);
  if (!q) return;
  static const u8 sprs[W_COUNT] = {gfx::PS_DAGGER, gfx::PS_ARROW, gfx::PS_AXE, gfx::PS_ORB,
                                     gfx::PS_PELLET, gfx::PS_PISTOL, gfx::PS_TRIDENT,
                                     gfx::PS_KNIFE, gfx::PS_PISTOL};
  const float speed = w.speed * m.projSpd;
  q->spr = sprs[s.weapon];
  q->x = s.p.x; q->y = s.p.y;
  q->vx = cosf(ang) * speed; q->vy = sinf(ang) * speed;
  q->dmg = (w.dmg + m.flat) * m.dmgMul * m.dealt * dmgMul;
  q->r = (s.weapon == W_AXE ? 4.0f : w.radius) * m.sizeMul;
  q->life = (w.range * m.rangeMul) / speed * 1.1f;
  q->pierce = w.pierce + m.pierce; q->bounceE = (u8)m.bounceE; q->bounceW = (u8)m.bounceW; q->homing = m.homing;
  if (s.weapon == W_TRIDENT) { q->retrievable = true; s.p.tridentOut = true; }
  q->homingRate = 7.0f * (1.0f + 0.20f * maxT(0, m.homingLevel - 1));
  if (s.weapon == W_AXE) {
    q->lob = true; q->tx = t.x; q->ty = t.y;
    float d = sqrtf(dist2(t.x, t.y, s.p.x, s.p.y));
    q->life = d / speed; q->aoe = w.radius * m.aoeMul;
  }
}

static void radialShots(Sim& s, int n, float dmg, bool child) {
  for (int i = 0; i < n; ++i) {
    Proj* q = newProj(s);
    if (!q) return;
    float a = 6.2831853f * i / n;
    q->spr = gfx::PS_EMBER; q->x = s.p.x; q->y = s.p.y; q->vx = cosf(a) * 170; q->vy = sinf(a) * 170;
    q->dmg = dmg; q->r = 4; q->life = 0.8f; q->kind = child ? 5 : 0;
  }
}

static void doThrow(Sim& s) {
  Player& p = s.p; Mods& m = s.m;
  const WeaponStat& w = WEAPON[s.weapon];
  if (s.weapon == W_TRIDENT && p.tridentOut) { p.atkT = 0.20f; return; }
  int t = nearestEnemy(s, p.x, p.y, w.range * m.rangeMul, -1);
  if (t < 0) return;
  p.throws++;
  float extraShots = 0.0f;
  for (int i = 0; i < m.nEv; ++i) if (m.ev[i].kind == E_NTH_DOUBLE && p.throws % maxT(1, (int)m.ev[i].a) == 0) {
    extraShots = maxT(extraShots, m.ev[i].b); if (m.ev[i].b > 0) spellCast(s);
  }
  if (s.weapon == W_KATANA) {
    const float dx = s.en[t].x - p.x, dy = s.en[t].y - p.y;
    const float len = sqrtf(dx * dx + dy * dy) + 0.001f;
    for (int k = 0; k < MAX_ENEMIES; ++k) {
      Enemy& e = s.en[k]; if (!e.alive) continue;
      const float ex = e.x - p.x, ey = e.y - p.y, d = sqrtf(ex * ex + ey * ey) + 0.001f;
      if (d <= KATANA_RANGE + enemyTouchR(e) && (ex * dx + ey * dy) / (d * len) >= KATANA_ARC_COS) {
        hitEnemy(s, k, (w.dmg + m.flat) * m.dmgMul * m.dealt, ex, ey, true, false);
        if (s.en[k].alive && s.rng.chance(KATANA_BLEED_CHANCE)) { s.en[k].bleedDps = maxT(s.en[k].bleedDps, KATANA_BLEED_DPS); s.en[k].bleedT = maxT(s.en[k].bleedT, KATANA_BLEED_TIME); }
      }
    }
    fxSlash(s, p.x, p.y, atan2f(dy, dx), KATANA_RANGE * 0.72f);
  } else if (s.weapon == W_SHOTGUN) {
    static const float spread[5] = {-0.28f, -0.14f, 0.0f, 0.14f, 0.28f};
    for (int i = 0; i < 5; ++i) fireWeapon(s, t, spread[i], 0.58f);
  } else {
    fireWeapon(s, t, 0, 1.0f);
  }
  int bonusShots = s.weapon == W_TRIDENT ? 0 : (int)extraShots;
  for (int j = 0; j < bonusShots; ++j) fireWeapon(s, t, (j & 1) ? -0.16f : 0.12f, 1.0f);
  if (s.weapon != W_TRIDENT && extraShots > bonusShots && s.rng.chance((extraShots - bonusShots) * 100.0f)) fireWeapon(s, t, 0.20f, 1.0f);
  audio::sfx(s.weapon == W_KATANA ? audio::SFX_SWING : audio::SFX_SHOOT);
  const float baseDmg = (w.dmg + m.flat) * m.dmgMul * m.dealt;
  const float lm = 1 + m.lightning / 100.0f;
  for (int i = 0; i < m.nEv; ++i) if (m.ev[i].kind == E_AVATAR && p.throws % maxT(1, (int)m.ev[i].a) == 0) {
    spellCast(s); const int element = s.rng.i(4); const float r = m.ev[i].c * m.aoeMul;
    if (element == 0) { aoeDamage(s, s.en[t].x, s.en[t].y, r, m.ev[i].b * m.dmgMul); fxRing(s, s.en[t].x, s.en[t].y, r, 2, 0.3f); }
    else if (element == 1) { for (int k = 0; k < MAX_ENEMIES; ++k) if (s.en[k].alive && dist2(s.en[t].x, s.en[t].y, s.en[k].x, s.en[k].y) < sq(r)) hitEnemy(s, k, m.ev[i].b * m.dmgMul, 0, 0, false, false); fxRing(s, s.en[t].x, s.en[t].y, r, 3, 0.25f); }
    else if (element == 2) { for (int k = 0; k < MAX_ENEMIES; ++k) if (s.en[k].alive && dist2(p.x, p.y, s.en[k].x, s.en[k].y) < sq(r)) { statusOn(s, s.en[k], 35, 1.2f, 0, 0, 0); s.en[k].kx += (s.en[k].x-p.x) * 0.5f; s.en[k].ky += (s.en[k].y-p.y) * 0.5f; } fxRing(s, p.x, p.y, r, 4, 0.3f); }
    else { aoeDamage(s, s.en[t].x, s.en[t].y, r * 0.75f, m.ev[i].b * 1.15f * m.dmgMul); fxDisc(s, s.en[t].x, s.en[t].y, r * 0.75f, 1, 0.25f); }
  }
  for (int i = 0; i < m.nEv; ++i) {
    const Ev& v = m.ev[i];
    switch (v.kind) {
      case E_NTH_RING: if (p.throws % maxT(1, (int)v.a) == 0) { spellCast(s); radialShots(s, maxT(1, (int)v.b), baseDmg * v.c, false); } break;
      case E_NTH_BLAST: if (p.throws % (int)v.a == 0) {
        spellCast(s);
        const bool big = v.c >= 100;
        float cx = big ? p.x : s.en[t].x, cy = big ? p.y : s.en[t].y, r = minT(v.c, 999.0f) * m.aoeMul;
        if (v.c >= 900) { for (int k = 0; k < MAX_ENEMIES; ++k) if (s.en[k].alive) hitEnemy(s, k, v.b, 0, 0, false, false); fxDisc(s, p.x, p.y, 150, 7, 0.4f); s.shake = 0.3f; }
        else { aoeDamage(s, cx, cy, r, v.b); fxDisc(s, cx, cy, r, 2, 0.3f); fxRing(s, cx, cy, r, 7, 0.4f); }
        audio::sfx(audio::SFX_BLAST); } break;
      case E_NTH_STUN: if (p.throws % (int)v.a == 0) {
        spellCast(s);
        for (int k = 0; k < MAX_ENEMIES; ++k) if (s.en[k].alive && dist2(p.x, p.y, s.en[k].x, s.en[k].y) < sq(v.c * m.aoeMul)) statusOn(s, s.en[k], 0, 0, 0, v.b, 0);
        fxRing(s, p.x, p.y, v.c * m.aoeMul, 7, 0.3f); audio::sfx(audio::SFX_BLAST); } break;
      case E_NTH_BOLT: if (p.throws % (int)v.a == 0) {
        spellCast(s);
        fxBolt(s, s.en[t].x, s.en[t].y - 6); aoeDamage(s, s.en[t].x, s.en[t].y, v.c * m.aoeMul, v.b * lm); } break;
      case E_NTH_STRIKE3: if (p.throws % (int)v.a == 0) {
        spellCast(s);
        for (int k = 0, n = 0; k < MAX_ENEMIES && n < (int)v.c; ++k) if (s.en[k].alive) { fxBolt(s, s.en[k].x, s.en[k].y - 6); dotEnemy(s, k, v.b * lm); ++n; } } break;
      case E_TIME_STOP: if (p.throws % maxT(1, (int)v.a) == 0 && p.timeStopCd <= 0.0f) {
        spellCast(s); p.timeStopT = maxT(p.timeStopT, v.b); p.timeStopCd = v.c;
        fxRing(s, p.x, p.y, 72, 5, 0.45f); fxText(s, p.x, p.y - 18, "TIME STOP", 5);
      } break;
      default: break;
    }
  }
  p.atkT = w.interval / m.atkSpd * m.cdMul;
}

static void reflectProj(Proj& q) { q.hostile = false; q.vx = -q.vx; q.vy = -q.vy; q.life = 1.0f; }

static void updateProjectiles(Sim& s, float dt) {
  Mods& m = s.m; Player& p = s.p;
  for (int i = 0; i < MAX_PROJ; ++i) {
    Proj& q = s.pr[i];
    if (!q.alive) continue;
    if (q.retrievable && q.stuck) {
      if (dist2(q.x, q.y, p.x, p.y) < sq(TRIDENT_PICKUP_RANGE)) {
        q.alive = false; p.tridentOut = false;
        fxRing(s, p.x, p.y, 12, 6, 0.25f); fxText(s, p.x, p.y - 12, T(S_trident_ready), 6); audio::sfx(audio::SFX_PICKUP);
      }
      continue;
    }
    if (s.p.timeStopT > 0.0f && q.hostile) continue;
    q.life -= dt;
    if (q.homing && !q.hostile && !q.lob) {
      int t = nearestEnemy(s, q.x, q.y, 110, -1);
      if (t >= 0) {
        float sp = sqrtf(q.vx * q.vx + q.vy * q.vy) + 0.01f, dx = s.en[t].x - q.x, dy = s.en[t].y - q.y, l = sqrtf(dx * dx + dy * dy) + 0.01f;
        float k = minT(1.0f, (q.homingRate > 0 ? q.homingRate : 7.0f) * dt);
        q.vx += (dx / l * sp - q.vx) * k; q.vy += (dy / l * sp - q.vy) * k;
      }
    }
    q.x += q.vx * dt; q.y += q.vy * dt;
    q.rot = atan2f(q.vy, q.vx) + 1.5707963f;
    if (q.lob) {
      if (q.life <= 0) {
        for (int k = 0; k < MAX_ENEMIES; ++k) if (s.en[k].alive && dist2(q.tx, q.ty, s.en[k].x, s.en[k].y) < sq(q.aoe + enemyTouchR(s.en[k]))) hitEnemy(s, k, q.dmg, q.vx, q.vy, true, false);
        fxRing(s, q.tx, q.ty, q.aoe, 7, 0.25f); fxDisc(s, q.tx, q.ty, q.aoe, 2, 0.15f);
        q.alive = false;
      }
      continue;
    }
    if (q.retrievable && q.life <= 0) {
      q.stuck = true; q.vx = q.vy = 0; q.life = 9999.0f;
      q.x = clampT(q.x, arenaMinX(s) + 4, arenaMaxX(s) - 4); q.y = clampT(q.y, arenaMinY(s) + 4, arenaMaxY(s) - 4);
      continue;
    }
    bool dead = q.life <= 0;
    for (int z = 0; z < MAX_ZONES && !dead; ++z)
      if (s.zn[z].type == Z_BRAMBLE && dist2(q.x, q.y, s.zn[z].x, s.zn[z].y) < sq(s.zn[z].r)) dead = true;
    const float minX = arenaMinX(s) - 4, maxX = arenaMaxX(s) + 4;
    const float minY = arenaMinY(s) - 4, maxY = arenaMaxY(s) + 4;
    if (!dead && (q.x < minX || q.x > maxX || q.y < minY || q.y > maxY)) {
      if (q.retrievable) {
        q.stuck = true; q.vx = q.vy = 0; q.life = 9999.0f;
        q.x = clampT(q.x, arenaMinX(s) + 4, arenaMaxX(s) - 4); q.y = clampT(q.y, arenaMinY(s) + 4, arenaMaxY(s) - 4);
        continue;
      }
      if (q.bounceW > 0) {
        if (q.x < minX || q.x > maxX) q.vx = -q.vx; else q.vy = -q.vy;
        q.x = clampT(q.x, minX, maxX); q.y = clampT(q.y, minY, maxY);
        q.bounceW--;
      } else dead = true;
    }
    if (dead && q.retrievable) {
      q.stuck = true; q.vx = q.vy = 0; q.life = 9999.0f;
      q.x = clampT(q.x, arenaMinX(s) + 4, arenaMaxX(s) - 4); q.y = clampT(q.y, arenaMinY(s) + 4, arenaMaxY(s) - 4);
      continue;
    }
    if (dead) { q.alive = false; continue; }
    if (q.hostile) {
      for (int si = 0; si < 2; ++si) {
        Sentinel& sent = s.sentinels[si];
        if (sent.active && dist2(q.x, q.y, sent.x, sent.y) < sq(q.r + 8.0f)) {
          damageSentinel(s, si, q.dmg); q.alive = false; break;
        }
      }
      if (!q.alive) continue;
      if (dist2(q.x, q.y, p.x, p.y) < sq(q.r + PLAYER_RADIUS)) {
        if (s.rng.chance(m.mirror)) { reflectProj(q); fxText(s, p.x, p.y - 12, "REFLECT", 3); continue; }
        if (hurtPlayer(s, q.dmg, true, nullptr)) {
          if (!m.immuneSlow) {
            if (q.kind == 1) { p.slowPct = 35; p.slowT = 2.0f * m.debuff; }
            if (q.kind == 2) { p.slowPct = 25; p.slowT = 2.0f * m.debuff; }
          }
          if (q.kind == 3) {
            int st = s.rng.i(ST_COUNT);
            if (pts(s, st) > 0) { s.drain[st]++; recompute(s); fxText(s, p.x, p.y - 12, "DRAIN", 5); }
          }
        }
        q.alive = false;
      }
      continue;
    }
    for (int j = 0; j < MAX_ENEMIES; ++j) {
      Enemy& e = s.en[j];
      if (!e.alive) continue;
      bool seen = false;
      for (int h = 0; h < minT<int>(q.nhit, 4); ++h) if (q.hit[h] == j) seen = true;
      if (seen || dist2(q.x, q.y, e.x, e.y) >= sq(q.r + enemyTouchR(e))) continue;
      if ((ENEMIES[e.type].beh & B_INVIS) && dist2(e.x, e.y, p.x, p.y) > sq(70)) continue;
      hitEnemy(s, j, q.dmg, q.vx, q.vy, true, true);
      q.hit[q.nhit & 3] = (s16)j; q.nhit++;
      if (q.retrievable) { q.stuck = true; q.vx = q.vy = 0; q.life = 9999.0f; break; }
      if (m.explodeDmg > 0 && q.kind != 5) { aoeDamage(s, q.x, q.y, m.explodeR * m.aoeMul, m.explodeDmg); fxRing(s, q.x, q.y, m.explodeR * m.aoeMul, 2, 0.2f); }
      if (m.split > 0 && q.kind != 5) {
        float base = atan2f(q.vy, q.vx);
        for (int k = 0; k < m.split; ++k) {
          Proj* c = newProj(s);
          if (!c) break;
          float a = base + (k - (m.split - 1) * 0.5f) * 0.9f + 3.14159f * 0.0f;
          c->spr = gfx::PS_SPARK; c->x = q.x; c->y = q.y; c->vx = cosf(a) * 170; c->vy = sinf(a) * 170;
          c->dmg = q.dmg * m.splitDmg / 100.0f; c->r = 3; c->life = 0.5f; c->kind = 5; c->hit[0] = (s16)j; c->nhit = 1;
        }
      }
      if (q.bounceE > 0) {
        int t = nearestEnemy(s, q.x, q.y, 90, j);
        if (t >= 0) {
          float dx = s.en[t].x - q.x, dy = s.en[t].y - q.y, l = sqrtf(dx * dx + dy * dy) + 0.01f, sp = sqrtf(q.vx * q.vx + q.vy * q.vy);
          q.vx = dx / l * sp; q.vy = dy / l * sp; q.dmg *= m.bounceDmg / 100.0f; q.bounceE--; q.life = maxT(q.life, 0.5f);
          break;
        }
      }
      if (q.pierce > 0) q.pierce--; else { q.alive = false; break; }
    }
  }
}

// ================================================================================ zones, auras, orbiters
static void updateZones(Sim& s, float dt) {
  Player& p = s.p;
  for (int i = 0; i < MAX_ZONES; ++i) {
    Zone& z = s.zn[i];
    if (z.type == Z_NONE) continue;
    if (z.type == Z_BRAMBLE || z.type == Z_FROST) continue;       // permanent hazards
    z.t -= dt;
    if (z.type == Z_FIRE || z.type == Z_POISON || z.type == Z_CLOUD || z.type == Z_SPARK || z.type == Z_VORTEX) {
      for (int k = 0; k < MAX_ENEMIES; ++k) {
        Enemy& e = s.en[k];
        if (!e.alive || dist2(z.x, z.y, e.x, e.y) > sq(z.r + 4)) continue;
        dotEnemy(s, k, z.dps * dt);
        if (z.type == Z_VORTEX && e.alive && !(ENEMIES[e.type].beh & B_BOSS)) {
          float dx = z.x - e.x, dy = z.y - e.y, l = sqrtf(dx * dx + dy * dy) + 0.01f;
          e.x += dx / l * 40 * dt; e.y += dy / l * 40 * dt;
        }
      }
    }
    if (z.t <= 0) {
      if (z.type == Z_BOMB) {
        aoeDamage(s, z.x, z.y, z.r, z.a);
        for (int k = 0; k < MAX_ENEMIES; ++k) {
          Enemy& e = s.en[k];
          if (e.alive && dist2(z.x, z.y, e.x, e.y) < sq(z.r + enemyTouchR(e))) {
            e.kx += (e.x - z.x) * 2.0f; e.ky += (e.y - z.y) * 2.0f;
          }
        }
        hurtPlayer(s, s.p.maxhp * 0.001f, false, nullptr, true);
        fxDisc(s, z.x, z.y, z.r, 2, 0.35f); fxRing(s, z.x, z.y, z.r, 7, 0.45f);
        s.shake = maxT(s.shake, 0.18f); audio::sfx(audio::SFX_BLAST);
      }
      if (z.type == Z_CURSE) { aoeDamage(s, z.x, z.y, z.r, z.a); fxRing(s, z.x, z.y, z.r, 5, 0.3f); fxDisc(s, z.x, z.y, z.r, 5, 0.2f); }
      if (z.type == Z_WARN) {
        aoeDamage(s, z.x, z.y, z.r, 8);
        if (dist2(z.x, z.y, p.x, p.y) < sq(z.r + PLAYER_RADIUS)) hurtPlayer(s, 6, false, nullptr);
        fxDisc(s, z.x, z.y, z.r, 2, 0.2f); s.shake = 0.2f; audio::sfx(audio::SFX_BLAST);
      }
      z.type = Z_NONE;
    }
  }
}

static void updateAuras(Sim& s, float dt) {
  Player& p = s.p; Mods& m = s.m;
  const float aoe = m.aoeMul, lm = 1 + m.lightning / 100.0f;
  p.auraT += dt;
  const bool tick = p.auraT >= 0.25f;
  if (tick) p.auraT -= 0.25f;
  for (int i = 0; i < m.nEv; ++i) {
    const Ev& v = m.ev[i];
    if (v.kind == E_AURA) {
      bool active = false;
      for (int k = 0; k < MAX_ENEMIES; ++k) {
        Enemy& e = s.en[k];
        if (!e.alive || dist2(p.x, p.y, e.x, e.y) > sq(v.b * aoe)) continue;
        active = true;
        if (v.c > 0) { e.slowPct = maxT(e.slowPct, v.c); e.slowT = maxT(e.slowT, 0.4f); }
        if (tick && v.a > 0) dotEnemy(s, k, v.a * 0.25f * lm);
      }
      if (tick && active) spellCast(s);
    } else if (v.kind == E_AURA_PULL) {
      bool active = false;
      for (int k = 0; k < MAX_ENEMIES; ++k) {
        Enemy& e = s.en[k];
        if (!e.alive || (ENEMIES[e.type].beh & B_BOSS) || dist2(p.x, p.y, e.x, e.y) > sq(v.a * aoe)) continue;
        active = true;
        float dx = p.x - e.x, dy = p.y - e.y, l = sqrtf(dx * dx + dy * dy) + 0.01f;
        if (l > 14) { e.x += dx / l * 30 * dt; e.y += dy / l * 30 * dt; }
      }
      if (tick && active) spellCast(s);
    } else if (v.kind == E_PERIODIC_FLARE) {
      p.flareT += dt;
      if (p.flareT >= v.a) {
        spellCast(s);
        p.flareT = 0;
        for (int k = 0; k < MAX_ENEMIES; ++k) if (s.en[k].alive) hitEnemy(s, k, v.b, 0, 0, false, false);
        fxDisc(s, p.x, p.y, 140, 2, 0.35f); audio::sfx(audio::SFX_BLAST);
      }
    } else if (v.kind == E_PERIODIC_BOMB) {
      p.bombT += dt;
      if (p.bombT >= v.a) {
        spellCast(s);
        p.bombT = 0.0f;
        newZone(s, Z_BOMB, p.x, p.y, v.c * aoe, 1.0f, 0.0f, v.b);
        fxRing(s, p.x, p.y, v.c * aoe, 2, 0.35f);
      }
    }
  }
  // orbiters
  if (s.nOrb > 0) {
    s.orb[0].ang += 3.0f * dt;
    for (int o = 0; o < s.nOrb; ++o) {
      float a = s.orb[0].ang + 6.2831853f * o / s.nOrb, ox = p.x + cosf(a) * 34, oy = p.y + sinf(a) * 34;
      for (int k = 0; k < MAX_ENEMIES; ++k)
        if (s.en[k].alive && dist2(ox, oy, s.en[k].x, s.en[k].y) < sq(9 + enemyTouchR(s.en[k]))) dotEnemy(s, k, m.orbitDps * dt);
    }
  }
}

// ================================================================================ waves and phases
static void startWave(Sim& s) {
  s.waveT = 0; s.spawned = 0; s.quota = waveQuota(s.wave); s.spawnAcc = 0; s.bossSpawned = false;
  s.bossWave = s.endless ? (s.wave % 10 == 0) : (s.wave == s.totalWaves);
  for (int i = 0; i < ST_COUNT; ++i) s.drain[i] = 0;
  s.p.moveBonus = 0; s.p.flareT = 0;
  recompute(s);
  s.p.shield = s.m.shieldWave; s.p.blocks = s.m.blockWave;
  s.phase = PH_PLAY;
  for (int i = 0; i < MAX_PROJ; ++i) s.pr[i].alive = false;
  s.p.tridentOut = false;
  char b[32]; snprintf(b, sizeof b, "%s", Tn(S_wave_n, s.wave)); setToast(s, b);
  audio::sfx(audio::SFX_WAVE);
  audio::musicSet(s.season, s.bossWave ? 1.0f : 0.4f);
}

static void endWave(Sim& s) {
  for (int i = 0; i < MAX_ENEMIES; ++i) if (s.en[i].alive) { s.en[i].alive = false; spawnGem(s, s.en[i].x, s.en[i].y, 1); }
  for (int i = 0; i < MAX_PROJ; ++i) s.pr[i].alive = false;
  if (s.m.hpPerWave > 0) { s.p.maxHpBonus += s.m.hpPerWave; recompute(s); }
  heal(s, s.p.maxhp * 0.15f);
  s.phase = (!s.endless && s.wave >= s.totalWaves) ? PH_VICTORY : PH_CLEARED;
  s.phaseT = 0; s.forced = false;
  audio::sfx(audio::SFX_LEVEL);
  audio::musicSet(s.season, 0.2f);
}

static bool inPool(const Sim& s, int id) {
  const SpellDef& d = SPELLS[id];
  if (d.sc == SC_GLOBAL) return true;
  if (d.sc == SC_SEASON) return d.season == s.season;
  return SEASON_ELEM[s.season][0] == d.el || SEASON_ELEM[s.season][1] == d.el;
}
static void rollOffers(Sim& s) {
  s.nOffers = 0; s.offersAreUpgrades = false;
  if (s.nSlots >= MAX_SLOTS) {
    int cand[MAX_SLOTS], n = 0;
    for (int i = 1; i < s.nSlots; ++i) if (s.slots[i].id >= 0 && s.slots[i].lv < SPELLS[s.slots[i].id].maxLevel) cand[n++] = i;
    s.offersAreUpgrades = true;
    while (n > 0 && s.nOffers < 3) { int k = s.rng.i(n); s.offers[s.nOffers++] = cand[k]; cand[k] = cand[--n]; }
    return;
  }
  int pool[NUM_SPELLS], n = 0;
  for (int i = 0; i < NUM_SPELLS; ++i) {
    if (!inPool(s, i) || simSpellLevelOf(s, i) > 0) continue;
    pool[n++] = i;
  }
  const float b = minT(s.m.rareBonus, 40.0f);
  const float W[4] = {maxT(5.0f, 60 - b), 28 + b * 0.6f, 10 + b * 0.3f, 2 + b * 0.1f};
  while (n > 0 && s.nOffers < 3) {
    float tot = 0; for (int i = 0; i < n; ++i) tot += W[SPELLS[pool[i]].rar];
    float x = s.rng.f() * tot; int k = 0;
    for (; k < n - 1; ++k) { x -= W[SPELLS[pool[k]].rar]; if (x <= 0) break; }
    s.offers[s.nOffers++] = pool[k]; pool[k] = pool[--n];
  }
}
static void goSpellPhase(Sim& s) {
  rollOffers(s);
  if (s.nOffers == 0) { simResume(s); return; }
  s.phase = PH_SPELL;
}
void simResume(Sim& s) {
  if (s.forced) { s.forced = false; s.phase = PH_PLAY; return; }
  s.wave++; startWave(s);
}
bool simSpend(Sim& s, int st) {
  if (s.phase != PH_ALLOC || s.points <= 0 || st < 0 || st >= ST_COUNT) return false;
  s.points--; s.alloc[st]++; recompute(s); audio::sfx(audio::SFX_UI);
  return true;
}
bool simStorePoints(Sim& s) {
  if (s.phase != PH_ALLOC || s.points <= 0 || s.bankStores >= 3) return false;
  s.bankedPoints += s.points;
  s.points = 0;
  ++s.bankStores;
  audio::sfx(audio::SFX_UI);
  return true;
}
bool simUseStoredPoints(Sim& s) {
  if (s.phase != PH_ALLOC || s.bankedPoints <= 0) return false;
  s.points += s.bankedPoints;
  s.bankedPoints = 0;
  audio::sfx(audio::SFX_UI);
  return true;
}
void simFinishAlloc(Sim& s) { goSpellPhase(s); }
void simPickOffer(Sim& s, int i) {
  if (s.phase != PH_SPELL) return;
  if (i >= 0 && i < s.nOffers) {
    if (s.offersAreUpgrades) { int sl = s.offers[i]; if (s.slots[sl].id >= 0 && s.slots[sl].lv < SPELLS[s.slots[sl].id].maxLevel) { s.slots[sl].lv++; ++s.runSpellUpgrades; } }
    else if (s.nSlots < MAX_SLOTS) {
      int id = s.offers[i];
      s.slots[s.nSlots++] = SpellSlot{(s16)id, 1};
      if (s.seenSpell[id] < 255) s.seenSpell[id]++;
      s.usedSpell[id] = 1;
      ++s.runSpellsAcquired;
    }
    recompute(s); audio::sfx(audio::SFX_LEVEL);
  }
  simResume(s);
}

static void setupHazards(Sim& s) {
  for (int i = 0; i < MAX_ZONES; ++i) s.zn[i].type = Z_NONE;
  int n = s.season == SEA_FOREST ? 7 : (s.season == SEA_SNOW ? 4 : 0);
  for (int i = 0; i < n; ++i) {
    float x, y;
    do { x = s.rng.range(arenaMinX(s) + 20, arenaMaxX(s) - 20); y = s.rng.range(arenaMinY(s) + 20, arenaMaxY(s) - 20); } while (dist2(x, y, s.p.x, s.p.y) < sq(55));
    newZone(s, s.season == SEA_FOREST ? Z_BRAMBLE : Z_FROST, x, y, s.season == SEA_FOREST ? 14.0f : 26.0f, 1e9f, 0, 0);
  }
}

void simStart(Sim& s, int season, int levelIdx, bool endless, int weapon, u32 seed, bool allSeasons) {
  memset((void*)&s, 0, sizeof(Sim));
  s.rng.s = seed ? seed : 1u;
  s.season = (u8)season; s.levelIdx = (u8)levelIdx; s.endless = endless; s.allSeasons = allSeasons; s.weapon = (u8)weapon;
  s.totalWaves = endless ? 100000 : LEVELS[clampT(levelIdx, 0, 2)].waves;
  s.wave = 1; s.phase = PH_PLAY;
  s.slots[0] = SpellSlot{(s16)(-1 - weapon), 1}; s.nSlots = 1;
  s.p.x = allSeasons ? 400.0f : 200.0f; s.p.y = allSeasons ? 300.0f : 130.0f; s.p.level = 1; s.p.lsBudget = 1;
  recompute(s); s.p.hp = s.p.maxhp;
  setupHazards(s);
  startWave(s);
}

// ================================================================================ update
static void spawnTick(Sim& s) {
  int enemiesAlive = 0;
  for (int i = 0; i < MAX_ENEMIES; ++i) if (s.en[i].alive) ++enemiesAlive;
  s.enemiesAlive = enemiesAlive;
  if (s.bossWave && !s.bossSpawned && s.waveT >= 1.5f) {
    s.bossSpawned = true;
    int bossSeason = s.allSeasons ? s.rng.i(5) : s.season;
    int b = spawnEnemy(s, bossSeason * 5 + 4, (arenaMinX(s) + arenaMaxX(s)) * 0.5f, arenaMinY(s) - 14, false);
    if (b >= 0) setToast(s, T(S_boss_incoming));
    audio::sfx(audio::SFX_BOSS);
  }
  const int quota = s.bossWave ? s.quota / 2 : s.quota;
  const float span = WAVE_LEN - 8.0f;
  const int target = (int)(quota * minT(1.0f, s.waveT / span));
  while (s.spawned < target && enemiesAlive < MAX_ENEMIES - 4) {
    float w0 = 10, w1 = s.wave >= 2 ? 7.0f : 0.0f, w2 = s.wave >= 4 ? 5.0f : 0.0f, w3 = s.wave >= 6 ? 3.0f : 0.0f;
    float x = s.rng.f() * (w0 + w1 + w2 + w3); int t = 0;
    if (x > w0) { x -= w0; t = 1; if (x > w1) { x -= w1; t = 2; if (x > w2) t = 3; } }
    int side = s.rng.i(4); float px, py;
    const float x0 = arenaMinX(s), x1 = arenaMaxX(s), y0 = arenaMinY(s), y1 = arenaMaxY(s);
    if (side == 0) { px = s.rng.range(x0, x1); py = y0 - 12; }
    else if (side == 1) { px = s.rng.range(x0, x1); py = y1 + 12; }
    else if (side == 2) { px = x0 - 12; py = s.rng.range(y0, y1); }
    else { px = x1 + 12; py = s.rng.range(y0, y1); }
    const int group = (s.allSeasons ? s.rng.i(5) : s.season) * 5;
    if (spawnEnemy(s, group + t, px, py, false) < 0) break;
    ++s.spawned; ++enemiesAlive;
  }
}

static void updateGems(Sim& s, float dt, bool all) {
  Player& p = s.p;
  for (int i = 0; i < MAX_GEMS; ++i) {
    Gem& g = s.gem[i];
    if (!g.alive) continue;
    float d = dist2(g.x, g.y, p.x, p.y);
    if (all || d < sq(MAGNET_R)) {
      float l = sqrtf(d) + 0.01f, sp = all ? 260.0f : 150.0f;
      g.x += (p.x - g.x) / l * sp * dt; g.y += (p.y - g.y) / l * sp * dt;
    }
    if (d < sq(PICKUP_R)) { g.alive = false; gainExp(s, g.v); audio::sfx(audio::SFX_PICKUP); }
  }
}

static void updateFairy(Sim& s, float dt) {
  if (!s.m.hasFairy) { s.fairy.active = false; return; }
  if (!s.fairy.active) { s.fairy.active = true; s.fairy.x = s.p.x; s.fairy.y = s.p.y; s.fairy.healT = 4.0f; s.fairy.phase = 0; fxRing(s, s.p.x, s.p.y, 12, 4, 0.3f); }
  s.fairy.phase += dt * 3.0f; s.fairy.x += (s.p.x + cosf(s.fairy.phase) * 16.0f - s.fairy.x) * minT(1.0f, dt * 5.0f);
  s.fairy.y += (s.p.y - 18.0f + sinf(s.fairy.phase * 1.7f) * 5.0f - s.fairy.y) * minT(1.0f, dt * 5.0f);
  s.fairy.healT -= dt; if (s.fairy.healT <= 0) { s.fairy.healT += 4.0f; spellCast(s); heal(s, 2.0f * maxT(0.5f, s.m.fairyPower)); fxRing(s, s.p.x, s.p.y, 18, 4, 0.3f); }
}

static void updateSentinels(Sim& s, float dt) {
  if (!s.m.hasSentinel) return;
  bool any = false;
  for (int i = 0; i < 2; ++i) any = any || s.sentinels[i].active;
  if (!any) {
    s.sentinelSpawnT = maxT(0.0f, s.sentinelSpawnT - dt);
    if (s.sentinelSpawnT <= 0.0f) {
      for (int i = 0; i < 2; ++i) if (!s.sentinels[i].active) {
        Sentinel& sent = s.sentinels[i];
        sent.active = true; sent.x = s.p.x; sent.y = s.p.y;
        sent.hp = sent.maxhp = 50.0f * maxT(0.5f, s.m.sentinelPower); sent.fireT = 0.0f;
        fxRing(s, sent.x, sent.y, 16, 3, 0.35f);
        break;
      }
    }
  }
  for (int i = 0; i < 2; ++i) {
    Sentinel& sent = s.sentinels[i];
    if (!sent.active) continue;
    sent.iframes = maxT(0.0f, sent.iframes - dt);
    sent.fireT -= dt;
    if (sent.fireT > 0.0f) continue;
    const float power = maxT(0.5f, s.m.sentinelPower);
    sent.fireT += maxT(0.25f, 0.8f / power);
    const int target = nearestEnemy(s, sent.x, sent.y, 1200.0f, -1);
    if (target < 0) continue;
    const float dx = s.en[target].x - sent.x, dy = s.en[target].y - sent.y;
    const float len = sqrtf(dx * dx + dy * dy) + 0.001f;
    Proj* q = newProj(s);
    if (!q) continue;
    q->spr = gfx::PS_BOLT; q->x = sent.x; q->y = sent.y;
    q->vx = dx / len * 320.0f; q->vy = dy / len * 320.0f;
    q->dmg = 6.0f * power * s.m.dmgMul * s.m.dealt; q->r = 3.0f; q->life = 2.0f;
    q->kind = 5; q->rot = atan2f(q->vy, q->vx) + 1.5707963f; spellCast(s);
  }
}

static void playUpdate(Sim& s, const SimInput& in, float dt) {
  Player& p = s.p; Mods& m = s.m;
  s.waveT += dt; s.runT += dt;
  // timers
  p.iframes = maxT(0.0f, p.iframes - dt); p.critHealT -= dt; p.killHealT -= dt; p.hurtFlash -= dt;
  p.lsBudget = minT(1.0f, p.lsBudget + dt); p.slowT -= dt; p.burnT -= dt;
  if (m.regenInt > 0) { p.regenT += dt; if (p.regenT >= m.regenInt) { spellCast(s); p.regenT = 0; heal(s, m.regenHp); } }
  // movement
  float mx = in.mx, my = in.my, l = sqrtf(mx * mx + my * my);
  if (l > 1) { mx /= l; my /= l; }
  float sp = m.move * (1 + p.moveBonus / 100.0f);
  if (s.weapon == W_MACHINE_GUN) sp *= MACHINE_GUN_MOVE_MUL;
  if (p.slowT > 0 && !m.immuneSlow) sp *= 1 - p.slowPct / 100.0f;
  for (int i = 0; i < MAX_ZONES; ++i) {
    const Zone& z = s.zn[i];
    if (z.type == Z_BRAMBLE && dist2(p.x, p.y, z.x, z.y) < sq(z.r)) sp *= 0.6f;
    if (z.type == Z_FROST && dist2(p.x, p.y, z.x, z.y) < sq(z.r)) sp *= 0.75f;
  }
  p.x = clampT(p.x + mx * sp * dt, arenaMinX(s), arenaMaxX(s));
  p.y = clampT(p.y + my * sp * dt, arenaMinY(s), arenaMaxY(s));
  if (m.hasDrone) {
    p.droneAngle += dt * 2.4f;
    if (p.droneAngle >= 6.2831853f) p.droneAngle -= 6.2831853f;
    p.droneFireT -= dt;
    if (p.droneFireT <= 0.0f) {
      p.droneFireT += 0.2f;
      const float dx0 = p.x + cosf(p.droneAngle) * 30.0f;
      const float dy0 = p.y + sinf(p.droneAngle) * 30.0f;
      const int target = nearestEnemy(s, dx0, dy0, 1200.0f, -1);
      if (target >= 0) {
        const float dx = s.en[target].x - dx0, dy = s.en[target].y - dy0;
        const float len = sqrtf(dx * dx + dy * dy) + 0.001f;
        Proj* q = newProj(s);
        if (q) {
          q->spr = gfx::PS_SPARK; q->x = dx0; q->y = dy0;
          q->vx = dx / len * 420.0f; q->vy = dy / len * 420.0f;
          q->dmg = 3.5f * maxT(0.5f, m.dronePower) * m.dmgMul * m.dealt; q->r = 3.0f; q->life = 1.8f;
          q->kind = 0; q->rot = atan2f(q->vy, q->vx) + 1.5707963f; spellCast(s);
        }
      }
    }
  }
  updateFairy(s, dt);
  updateSentinels(s, dt);
  const bool moving = l > 0.1f;
  if (moving) {
    p.trailT -= dt;
    if (p.trailT <= 0) {
      p.trailT = 0.22f;
      for (int i = 0; i < m.nEv; ++i) {
        if (m.ev[i].kind == E_TRAIL_FIRE) { spellCast(s); newZone(s, Z_FIRE, p.x, p.y, 9, m.ev[i].b + 0.4f, m.ev[i].a, 0); }
        if (m.ev[i].kind == E_TRAIL_SPARK) { spellCast(s); newZone(s, Z_SPARK, p.x, p.y, 8, 0.8f, m.ev[i].a, 0); }
      }
    }
  }
  // hazards
  s.hazardT += dt;
  if (s.season == SEA_MOUNTAIN && s.hazardT >= 5.0f) {
    s.hazardT = 0; newZone(s, Z_WARN, clampT(p.x + s.rng.range(-70, 70), arenaMinX(s) + 10, arenaMaxX(s) - 10), clampT(p.y + s.rng.range(-60, 60), arenaMinY(s) + 10, arenaMaxY(s) - 10), 22, 1.4f, 0, 0);
  }
  if (s.season == SEA_DESERT) {
    s.heatT += dt;
    if (s.heatT >= 15.0f) { s.heatT = 0; p.burnT = 3.0f; fxRing(s, p.x, p.y, 60, 2, 0.5f); hurtPlayer(s, 2, true, nullptr); }
  }
  // attack
  p.atkT -= dt;
  if (p.atkT <= 0) { doThrow(s); if (p.atkT <= 0) p.atkT = 0.05f; }
  updateAuras(s, dt);
  enemiesUpdate(s, dt);
  updateProjectiles(s, dt);
  updateZones(s, dt);
  updateGems(s, dt, false);
  spawnTick(s);
  // clear conditions
  bool bossAlive = false;
  for (int i = 0; i < MAX_ENEMIES; ++i) if (s.en[i].alive && (ENEMIES[s.en[i].type].beh & B_BOSS)) bossAlive = true;
  if (s.phase != PH_PLAY) return;
  if (s.bossWave) { if (s.bossSpawned && !bossAlive) endWave(s); }
  else if ((s.spawned >= s.quota && s.enemiesAlive == 0) || s.waveT >= WAVE_LEN + 12.0f) endWave(s);
}

void simUpdate(Sim& s, const SimInput& in, float dt) {
  s.frame++;
  if (s.toastT > 0) s.toastT -= dt;
  if (s.shake > 0) s.shake -= dt;
  for (int i = 0; i < MAX_FX; ++i) {
    Fx& f = s.fx[i];
    if (f.kind == FX_NONE) continue;
    f.t += dt;
    if (f.kind == FX_TEXT) f.y -= 18 * dt;
    if (f.t >= f.life) f.kind = FX_NONE;
  }
  if (s.phase == PH_PLAY) playUpdate(s, in, dt);
  else if (s.phase == PH_CLEARED) {
    s.phaseT += dt;
    updateGems(s, dt, true);
    if (s.phaseT > 1.5f) { if (s.points > 0) s.phase = PH_ALLOC; else goSpellPhase(s); }
  } else if (s.phase == PH_VICTORY || s.phase == PH_DEAD) { s.phaseT += dt; updateGems(s, dt, true); }
}

// ================================================================================ debug tools
void dbgWaveEnd(Sim& s) {
  if (s.phase != PH_PLAY) { setToast(s, T(S_finish_phase)); return; }
  endWave(s);
}
void dbgBoss(Sim& s) {
  if (s.phase != PH_PLAY) { setToast(s, T(S_finish_phase)); return; }
  for (int i = 0; i < MAX_ENEMIES; ++i) if (s.en[i].alive && (ENEMIES[s.en[i].type].beh & B_BOSS)) { s.en[i].alive = false; setToast(s, T(S_boss_off)); return; }
  const int group = (s.allSeasons ? s.rng.i(5) : s.season) * 5;
  spawnEnemy(s, group + 4, (arenaMinX(s) + arenaMaxX(s)) * 0.5f, arenaMinY(s) - 14, false);
  setToast(s, T(S_dbg_boss));
}
void dbgForceSpell(Sim& s) {
  if (s.phase != PH_PLAY) { setToast(s, T(S_finish_phase)); return; }
  if (s.nSlots >= MAX_SLOTS) { setToast(s, T(S_build_full_t)); return; }
  rollOffers(s); if (s.nOffers) { s.forced = true; s.phase = PH_SPELL; }
}
void dbgClearSpells(Sim& s) {
  if (s.phase != PH_PLAY) { setToast(s, T(S_finish_phase)); return; }
  s.nSlots = 1; recompute(s); setToast(s, T(S_spells_cleared));
}
void dbgDeath(Sim& s) { s.p.hp = 0; s.p.usedRevive = true; s.phase = PH_DEAD; s.phaseT = 0; }
void dbgGiveLevel(Sim& s) { s.p.level++; s.points++; }
