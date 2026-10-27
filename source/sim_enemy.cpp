#include "sim_internal.h"
#include "gfx.h"
#include <string.h>
using namespace bal;

float enemyTouchR(const Enemy& e) {
  return (ENEMIES[e.type].beh & B_BOSS) ? 12.0f : (e.small ? 4.5f : ENEMY_RADIUS);
}

int spawnEnemy(Sim& s, int type, float x, float y, bool small) {
  if (type < 0 || type >= NUM_ENEMIES) return -1;
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (s.en[i].alive) continue;
    Enemy& e = s.en[i];
    const EnemyDef& d = ENEMIES[type];
    memset(&e, 0, sizeof e);
    const int tier = s.endless ? minT(3, (s.wave - 1) / 10) : (int)s.levelIdx;
    const bool boss = (d.beh & B_BOSS) != 0;
    const float hs = boss ? 1.0f + 0.35f * tier : hpScale(s.wave, tier);
    e.alive = true; e.type = (u8)type; e.small = small;
    e.maxhp = e.hp = d.hp * hs * (small ? 0.6f : 1.0f);
    e.dmg = d.dmg * dmgScale(s.wave, tier);
    e.spd = SPEED_WORD[d.spd] * (boss ? 1.0f : s.rng.range(0.92f, 1.08f));
    e.atkT = s.rng.range(0.4f, 1.2f); e.t0 = s.rng.range(0.5f, 2.0f); e.t1 = 3.0f;
    e.x = x; e.y = y;
    s.seenEnemy[type] = true;
    return i;
  }
  return -1;
}

static void shoot(Sim& s, const Enemy& e, float ang, float speed, float dmg, int spr, int kind, int bounces = 0) {
  Proj* q = newProj(s);
  if (!q) return;
  q->hostile = true; q->spr = (u8)spr; q->kind = (u8)kind; q->bounceW = (u8)bounces;
  q->x = e.x; q->y = e.y; q->vx = cosf(ang) * speed; q->vy = sinf(ang) * speed; q->dmg = dmg; q->r = 3.5f; q->life = 3.0f;
}
static void summon(Sim& s, const Enemy& e, int type, int n, bool small) {
  for (int k = 0; k < n; ++k) {
    int i = spawnEnemy(s, type, e.x + s.rng.range(-18, 18), e.y + s.rng.range(-18, 18), small);
    if (i < 0) break;
  }
  fxRing(s, e.x, e.y, 24, 5, 0.4f);
}
static float segDist(float px, float py, float ax, float ay, float bx, float by) {
  float vx = bx - ax, vy = by - ay, t = clampT(((px - ax) * vx + (py - ay) * vy) / (vx * vx + vy * vy + 0.001f), 0.0f, 1.0f);
  return sqrtf(dist2(px, py, ax + vx * t, ay + vy * t));
}

// -------- bosses ---------------------------------------------------------------------------------------------
static void bossAI(Sim& s, int ei, float dt, float dist, float nx, float ny, int targetSentinel,
                   float targetX, float targetY, float& moveMul, bool& holdPos) {
  Enemy& e = s.en[ei];
  const float ratio = e.hp / e.maxhp;
  const float aim = atan2f(ny, nx);
  e.t0 -= dt; e.t1 -= dt;
  switch (e.type) {
    case EN_BROOD: {
      if (ratio < 0.5f && e.phase == 0) { e.phase = 1; fxText(s, e.x, e.y - 20, "ENRAGE", 1); }
      moveMul = e.phase ? 1.4f : 1.0f; holdPos = dist < 100;
      if (e.t0 <= 0) { shoot(s, e, aim, 100, e.dmg, gfx::PS_WEB, 1); e.t0 = e.phase ? 1.2f : 2.0f; }
      if (e.t1 <= 0) { summon(s, e, EN_WASP, 2, false); e.t1 = e.phase ? 3.5f : 6.0f; }
    } break;
    case EN_PHARAOH: {
      if (ratio < 0.3f && e.phase == 0) { e.phase = 1; fxText(s, e.x, e.y - 20, "ENRAGE", 1); }
      moveMul = e.phase ? 1.5f : 1.0f; holdPos = dist < 110;
      if (e.state == 0) {
        if (e.t0 <= 0) { e.state = 1; e.t2 = 0.7f; e.face = aim - 0.8f; e.telegraph = true; }
        if (e.t1 <= 0) { summon(s, e, EN_THRALL, 1, false); e.t1 = 8.0f; }
      } else if (e.state == 1) { holdPos = true; e.t2 -= dt; if (e.t2 <= 0) { e.state = 2; e.t2 = 1.4f; e.telegraph = false; } }
      else {
        holdPos = true; e.t2 -= dt;
        float ang = e.face + (1.4f - e.t2) / 1.4f * 1.6f;
        if (segDist(targetX, targetY, e.x, e.y, e.x + cosf(ang) * 240, e.y + sinf(ang) * 240) < 7) {
          if (targetSentinel >= 0) damageSentinel(s, targetSentinel, e.dmg);
          else hurtPlayer(s, e.dmg, true, nullptr);
        }
        if (e.t2 <= 0) { e.state = 0; e.t0 = e.phase ? 4.5f : 7.0f; }
      }
    } break;
    case EN_FROZEN: case EN_KING: {
      const bool king = e.type == EN_KING;
      if (king && ratio < 0.4f && e.phase == 0) { e.phase = 1; fxText(s, e.x, e.y - 20, "ENRAGE", 1); }
      moveMul = e.phase ? 1.5f : 1.0f;
      const float R = king ? 70.0f : 62.0f;
      if (e.state == 0) {
        if (e.t0 <= 0 && dist < R + 40) { e.state = 1; e.t2 = 0.8f; e.telegraph = true; }
        if (e.t1 <= 0) {
          if (king) summon(s, e, EN_GOLEM, 1, true); else summon(s, e, EN_ICE, 2, false);
          e.t1 = king ? 9.0f : 10.0f;
        }
        if (!king && e.t0 > 0 && fmodf(e.t0, 4.0f) < dt) { for (int k = 0; k < 8; ++k) shoot(s, e, k * 0.785398f, 90, e.dmg * 0.5f, gfx::PS_SHARD, 2); }
      } else {
        holdPos = true; e.t2 -= dt;
        if (e.t2 <= 0) {
          e.state = 0; e.telegraph = false; e.t0 = king ? (e.phase ? 3.0f : 4.5f) : 5.0f;
          fxDisc(s, e.x, e.y, R, 2, 0.25f); s.shake = 0.3f; audio::sfx(audio::SFX_BLAST);
          if (dist < R + (targetSentinel >= 0 ? 8.0f : PLAYER_RADIUS)) {
            if (targetSentinel >= 0) damageSentinel(s, targetSentinel, e.dmg);
            else hurtPlayer(s, e.dmg, false, &e);
          }
        }
      }
    } break;
    case EN_CHOIR: {
      e.phase = ratio > 0.66f ? 0 : (ratio > 0.33f ? 1 : 2);
      holdPos = dist < 120;
      if (e.phase == 0 && e.t0 <= 0) { for (int k = -1; k <= 1; ++k) shoot(s, e, aim + k * 0.3f, 95, e.dmg * 0.4f, gfx::PS_SHARD, 3); e.t0 = 1.6f; }
      if (e.phase == 1 && e.t0 <= 0) {
        static const u8 sprs[4] = {gfx::PS_DAGGER, gfx::PS_ARROW, gfx::PS_AXE, gfx::PS_ORB};
        for (int k = -2; k <= 2; ++k) shoot(s, e, aim + k * 0.28f, 120, e.dmg * 0.4f, sprs[s.weapon & 3], 0);
        e.t0 = 2.0f;
      }
      if (e.phase == 2) {
        if (e.t0 <= 0) { for (int k = -1; k <= 1; ++k) shoot(s, e, aim + k * 0.4f, 100, e.dmg * 0.4f, gfx::PS_SHARD, 3); e.t0 = 2.2f; }
        if (e.t1 <= 0) { summon(s, e, EN_HUSK, 1, false); summon(s, e, EN_ACOLYTE, 1, false); e.t1 = 6.0f; }
      }
    } break;
    default: break;
  }
  if (e.type == EN_PHARAOH && e.state == 2) {} // beam drawn in enemiesDraw
}

// -------- generic AI ------------------------------------------------------------------------------------------
void enemiesUpdate(Sim& s, float dt) {
  Player& p = s.p;
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    Enemy& e = s.en[i];
    if (!e.alive) continue;
    if (s.p.timeStopT > 0.0f) continue;
    const EnemyDef& def = ENEMIES[e.type];
    const bool boss = (def.beh & B_BOSS) != 0;
    e.flash -= dt; e.atkT -= dt;
    if (e.burnT > 0) { e.burnT -= dt; dotEnemy(s, i, e.burnDps * dt); if (!e.alive) continue; }
    if (e.bleedT > 0) { e.bleedT -= dt; dotEnemy(s, i, e.bleedDps * dt); if (!e.alive) continue; }
    e.slowT -= dt; if (e.slowT <= 0) e.slowPct = 0;
    e.markT -= dt;
    const int targetSentinel = nearestSentinelTarget(s, e.x, e.y);
    const float targetX = targetSentinel >= 0 ? s.sentinels[targetSentinel].x : p.x;
    const float targetY = targetSentinel >= 0 ? s.sentinels[targetSentinel].y : p.y;
    float dx = targetX - e.x, dy = targetY - e.y, dist = sqrtf(dx * dx + dy * dy) + 0.001f, nx = dx / dist, ny = dy / dist;
    e.dirx = nx; e.diry = ny;
    if (e.freezeT > 0) { e.freezeT -= dt; continue; }
    if (e.stunT > 0) { e.stunT -= dt; continue; }
    float spd = e.spd * (1 - minT(e.slowPct, 90.0f) / 100.0f);
    for (int z = 0; z < MAX_ZONES; ++z)
      if (s.zn[z].type == Z_BRAMBLE && dist2(e.x, e.y, s.zn[z].x, s.zn[z].y) < sq(s.zn[z].r)) spd *= 0.6f;
    if (e.rootT > 0) { e.rootT -= dt; spd = 0; }
    e.x += e.kx * dt; e.y += e.ky * dt;
    { float k = minT(1.0f, 8 * dt); e.kx -= e.kx * k; e.ky -= e.ky * k; }
    float mvx = nx, mvy = ny, moveMul = 1.0f; bool hold = false;
    // ---- behaviours
    if (boss) {
      bossAI(s, i, dt, dist, nx, ny, targetSentinel, targetX, targetY, moveMul, hold);
    } else if (def.beh & (B_DASH | B_LUNGE | B_CHARGE)) {
      float per = 2.5f, wind = 0.5f, dash = 0.35f, mult = 3.2f;
      if (def.beh & B_LUNGE) { per = 2.2f; wind = 0.4f; dash = 0.5f; mult = 3.5f; }
      if (def.beh & B_CHARGE) { per = 3.5f; wind = 0.9f; dash = 0.8f; mult = 4.0f; }
      if (e.state == 0) { e.t0 -= dt; if (e.t0 <= 0 && dist < 160) { e.state = 1; e.t1 = wind; e.telegraph = true; } }
      else if (e.state == 1) { hold = true; e.t1 -= dt; if (e.t1 <= 0) { e.state = 2; e.t1 = dash; e.telegraph = false; e.vx = nx; e.vy = ny; } }
      else { mvx = e.vx; mvy = e.vy; moveMul = mult; e.t1 -= dt; if (e.t1 <= 0) { e.state = 0; e.t0 = per; } }
    } else if (def.beh & B_ERRATIC) {
      e.t0 -= dt;
      if (e.t0 <= 0) { float a = s.rng.range(0, 6.2831853f); e.vx = cosf(a); e.vy = sinf(a); e.t0 = 0.4f; }
      mvx = nx * 0.6f + e.vx * 0.9f; mvy = ny * 0.6f + e.vy * 0.9f;
    } else if (def.range != RW_TOUCH) {
      const float R = RANGE_WORD[def.range], want = R * 0.7f;
      if (dist < want * 0.6f) { mvx = -nx; mvy = -ny; } else if (dist < want) hold = true;
      if (e.atkT <= 0 && dist < R) {
        const float ang = atan2f(ny, nx), dm = e.dmg;
        if (def.beh & B_WEB) shoot(s, e, ang, 95, dm, gfx::PS_WEB, 1);
        else if (def.beh & B_SLOWHIT) shoot(s, e, ang, 150, dm, gfx::PS_ARROW, 2);
        else if (def.beh & B_FAN) { for (int k = -1; k <= 1; ++k) shoot(s, e, ang + k * 0.35f, 110, dm, gfx::PS_SHARD, 0); }
        else if (def.beh & B_BOUNCE) shoot(s, e, ang, 110, dm, gfx::PS_SHARD, 0, 2);
        else if (def.beh & B_DRAIN) shoot(s, e, ang, 100, dm, gfx::PS_SHARD, 3);
        else if (def.beh & B_COPY) { static const u8 sp[4] = {gfx::PS_DAGGER, gfx::PS_ARROW, gfx::PS_AXE, gfx::PS_ORB}; shoot(s, e, ang, 120, dm, sp[s.weapon & 3], 0); }
        else shoot(s, e, ang, 110, dm, gfx::PS_SHARD, 0);
        e.atkT = ATK_WORD[def.atk];
      }
    }
    if (def.beh & B_SCREAM) {
      e.t1 -= dt;
      if (e.t1 <= 0) { e.t1 = 5.0f; const int group = (s.allSeasons ? s.rng.i(5) : s.season) * 5; summon(s, e, group + s.rng.i(2), 2, false); fxRing(s, e.x, e.y, 40, 5, 0.4f); }
    }
    if ((def.beh & B_SPLIT_HALF) && !e.split && e.hp <= e.maxhp * 0.5f) {
      e.split = true;
      int n = spawnEnemy(s, e.type, e.x + 8, e.y, true);
      if (n >= 0) { s.en[n].hp = s.en[n].maxhp = e.hp; s.en[n].split = true; }
    }
    // ---- movement
    if (!hold && spd > 0) { e.x += mvx * spd * moveMul * dt; e.y += mvy * spd * moveMul * dt; }
    // gentle separation with a rotating neighbour (cheap)
    int j = (i + 1 + (int)(s.frame % 7)) % MAX_ENEMIES;
    if (j != i && s.en[j].alive && !boss) {
      float ox = e.x - s.en[j].x, oy = e.y - s.en[j].y, d2 = ox * ox + oy * oy;
      if (d2 < 100 && d2 > 0.01f) { float d = sqrtf(d2); e.x += ox / d * 20 * dt; e.y += oy / d * 20 * dt; }
    }
    e.x = clampT(e.x, arenaMinX(s) - 40.0f, arenaMaxX(s) + 40.0f);
    e.y = clampT(e.y, arenaMinY(s) - 40.0f, arenaMaxY(s) + 40.0f);
    // ---- contact damage (touch enemies, bosses, dashers)
    const bool melee = def.range == RW_TOUCH || boss || (def.beh & (B_DASH | B_LUNGE | B_CHARGE));
    if (melee && e.atkT <= 0 && dist < enemyTouchR(e) + (targetSentinel >= 0 ? 8.0f : PLAYER_RADIUS) && !(boss && e.state == 1)) {
      bool landed = false;
      if (targetSentinel >= 0) { damageSentinel(s, targetSentinel, e.dmg); landed = true; }
      else landed = hurtPlayer(s, e.dmg, false, &e);
      if (landed) {
        e.atkT = ATK_WORD[def.atk];
        if (def.beh & B_LIFESTEAL) e.hp = minT(e.maxhp, e.hp + e.dmg);
        if (targetSentinel < 0 && (def.beh & B_DRAIN)) { int st = s.rng.i(ST_COUNT); if (s.alloc[st] > s.drain[st]) { s.drain[st]++; recompute(s); } }
      }
    }
  }
}

// -------- drawing ---------------------------------------------------------------------------------------------
void enemiesDraw(const Sim& s) {
  const Player& p = s.p;
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    const Enemy& e = s.en[i];
    if (!e.alive) continue;
    const EnemyDef& def = ENEMIES[e.type];
    const bool boss = (def.beh & B_BOSS) != 0;
    const float scale = boss ? 2.0f : (e.small ? 0.75f : 1.0f);
    u32 alpha = 255;
    if (def.beh & B_INVIS) {
      float d = sqrtf(dist2(e.x, e.y, p.x, p.y));
      for (int k = 0; k < 2; ++k) if (s.sentinels[k].active) d = minT(d, sqrtf(dist2(e.x, e.y, s.sentinels[k].x, s.sentinels[k].y)));
      if (d > 110) continue;
      if (d > 70) alpha = 60;
    }
    gfx::sprC(gfx::shadow(), e.x, e.y + 7 * scale, scale);
    if (e.telegraph) {
      if (boss && (e.type == EN_FROZEN || e.type == EN_KING)) {
        float r = e.type == EN_KING ? 70.0f : 62.0f;
        gfx::ringC(1, e.x, e.y, r, 220); gfx::discC(1, e.x, e.y, r * (1.0f - e.t2 / 0.8f), 160);
      } else if (boss && e.type == EN_PHARAOH) {
        for (int k = 1; k <= 8; ++k) gfx::discC(7, e.x + cosf(e.face + 0.8f) * k * 28, e.y + sinf(e.face + 0.8f) * k * 28, 3, 160);
      } else gfx::ringC(1, e.x, e.y, 14, 230);
    }
    if (boss && e.type == EN_PHARAOH && e.state == 2) {
      float ang = e.face + (1.4f - e.t2) / 1.4f * 1.6f;
      for (int k = 1; k <= 10; ++k) gfx::discC(7, e.x + cosf(ang) * k * 24, e.y + sinf(ang) * k * 24, 6, 230);
    }
    const gfx::Spr& sp = gfx::enemy(e.type);
    if (e.flash > 0) gfx::sprC(sp, e.x, e.y, scale, HEX(0xFFFFFF), plat::BLIT_TINT);
    else if (e.freezeT > 0) gfx::sprC(sp, e.x, e.y, scale, HEX(C_SNOW), plat::BLIT_TINT);
    else if (e.stunT > 0) gfx::sprC(sp, e.x, e.y, scale, HEX(0xF0E050), plat::BLIT_TINT);
    else if (alpha < 255) gfx::sprC(sp, e.x, e.y, scale, withAlpha(0xFFFFFF, alpha), plat::BLIT_ALPHA);
    else gfx::sprC(sp, e.x, e.y, scale);
  }
}
