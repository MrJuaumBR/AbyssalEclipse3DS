// Shared internals between sim.cpp (player/spells/world) and sim_enemy.cpp (AI).
#pragma once
#include "sim.h"
#include "audio.h"

static inline float sq(float x) { return x * x; }
static inline float dist2(float ax, float ay, float bx, float by) { return sq(ax - bx) + sq(ay - by); }
enum { ARENA_X0 = 10, ARENA_Y0 = 26, ARENA_X1 = 390, ARENA_Y1 = 230 };
static inline float arenaMinX(const Sim& s) { return s.allSeasons ? 0.0f : (float)ARENA_X0; }
static inline float arenaMinY(const Sim& s) { return s.allSeasons ? 0.0f : (float)ARENA_Y0; }
static inline float arenaMaxX(const Sim& s) { return s.allSeasons ? 800.0f : (float)ARENA_X1; }
static inline float arenaMaxY(const Sim& s) { return s.allSeasons ? 600.0f : (float)ARENA_Y1; }
static inline int nearestSentinelTarget(const Sim& s, float x, float y) {
  if (!s.m.hasSentinel) return -1;
  int best = -1; float bd = dist2(x, y, s.p.x, s.p.y);
  for (int i = 0; i < 2; ++i) if (s.sentinels[i].active) {
    float d = dist2(x, y, s.sentinels[i].x, s.sentinels[i].y);
    if (d < bd) { bd = d; best = i; }
  }
  return best;
}

Proj* newProj(Sim& s);
int spawnEnemy(Sim& s, int type, float x, float y, bool small);
void fxText(Sim& s, float x, float y, const char* t, int col);
void fxRing(Sim& s, float x, float y, float r, int col, float life);
void fxDisc(Sim& s, float x, float y, float r, int col, float life);
void fxBolt(Sim& s, float x, float y);
bool hurtPlayer(Sim& s, float dmg, bool ranged, Enemy* src, bool allowTiny = false);
void damageSentinel(Sim& s, int index, float dmg);
void newZone(Sim& s, u8 type, float x, float y, float r, float t, float dps, float a);
void killEnemy(Sim& s, int ei, int depth);
void dotEnemy(Sim& s, int ei, float dmg);
void hitEnemy(Sim& s, int ei, float base, float dx, float dy, bool events, bool fromProj);
void aoeDamage(Sim& s, float x, float y, float r, float dmg);
float enemyTouchR(const Enemy& e);
void enemiesUpdate(Sim& s, float dt);
void enemiesDraw(const Sim& s);
