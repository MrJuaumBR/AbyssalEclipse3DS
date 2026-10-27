// Battlefield + HUD rendering (top screen).
#include "sim_internal.h"
#include "gfx.h"
#include "i18n.h"
#include <stdio.h>
using namespace bal;

static u32 shade(u32 c, int num, int den) {
  u32 r = ((c >> 16) & 255) * num / den, g = ((c >> 8) & 255) * num / den, b = (c & 255) * num / den;
  return (r << 16) | (g << 8) | b;
}

static void drawBackground(const Sim& s) {
  const u32 base = SEASON_COLOR[s.season];
  const float x0 = arenaMinX(s), y0 = arenaMinY(s), x1 = arenaMaxX(s), y1 = arenaMaxY(s);
  gfx::rect(0, 0, s.allSeasons ? 800.0f : (float)TOP_W, s.allSeasons ? 600.0f : (float)TOP_H, shade(base, 1, 4));
  gfx::rect(x0 - 4, y0 - 4, x1 - x0 + 8, y1 - y0 + 8, shade(base, 3, 8));
  const u32 tuft = shade(base, 1, 2);
  u32 h = 0x1234567u + s.season * 7919u;
  const int decor = s.allSeasons ? 180 : 26;
  for (int i = 0; i < decor; ++i) {
    h = h * 1664525u + 1013904223u;
    float x = x0 + (h >> 8) % (int)(x1 - x0 - 6);
    h = h * 1664525u + 1013904223u;
    float y = y0 + (h >> 8) % (int)(y1 - y0 - 4);
    gfx::rect(x, y, (i & 1) ? 5.0f : 3.0f, 2, tuft);
  }
}

static void drawZones(const Sim& s) {
  for (int i = 0; i < MAX_ZONES; ++i) {
    const Zone& z = s.zn[i];
    switch (z.type) {
      case Z_FIRE: gfx::discC(2, z.x, z.y, z.r, 200); break;
      case Z_POISON: case Z_CLOUD: gfx::discC(4, z.x, z.y, z.r, 160); break;
      case Z_SPARK: gfx::discC(6, z.x, z.y, z.r, 160); break;
      case Z_VORTEX: gfx::discC(3, z.x, z.y, z.r, 170); gfx::ringC(0, z.x, z.y, z.r, 200); break;
      case Z_CURSE: gfx::ringC(5, z.x, z.y, z.r, 200); gfx::discC(5, z.x, z.y, 5, 220); break;
      case Z_WARN: gfx::ringC(1, z.x, z.y, z.r, 230); gfx::discC(1, z.x, z.y, z.r * (1.0f - z.t / 1.4f), 150); break;
      case Z_BOMB: gfx::ringC(2, z.x, z.y, z.r, 220); gfx::discC(2, z.x, z.y, z.r * (1.0f - z.t), 90); gfx::sprC(gfx::proj(gfx::PS_TNT), z.x, z.y, 2.0f, 0xFFFFFFFFu, plat::BLIT_PLAIN, (1.0f - z.t) * 5.0f); break;
      case Z_BRAMBLE: gfx::discC(4, z.x, z.y, z.r, 255); gfx::sprC(gfx::icon(IC_LEAF), z.x, z.y, 1.0f, HEX(0x1E4A22), plat::BLIT_TINT); break;
      case Z_FROST: gfx::discC(3, z.x, z.y, z.r, 120); break;
      default: break;
    }
  }
}

static void bar(float x, float y, float w, float h, float frac, u32 col) {
  gfx::rect(x, y, w, h, C_VOID);
  gfx::rect(x + 1, y + 1, (w - 2) * clampT(frac, 0.0f, 1.0f), h - 2, col);
}

void simDraw(Sim& s, bool showFps, int fps) {
  const Player& p = s.p;
  s.camX = s.allSeasons ? clampT(p.x - 200.0f, 0.0f, 400.0f) : 0.0f;
  s.camY = s.allSeasons ? clampT(p.y - 120.0f, 0.0f, 360.0f) : 0.0f;
  gfx::setOffset(-s.camX, -s.camY);
  drawBackground(s);
  drawZones(s);
  for (int i = 0; i < 2; ++i) if (s.sentinels[i].active) {
    const Sentinel& sent = s.sentinels[i];
    gfx::sprC(gfx::sentinel(), sent.x, sent.y, 1.0f);
    bar(sent.x - 12, sent.y - 13, 24, 4, sent.hp / sent.maxhp, C_GREEN);
  }
  for (int i = 0; i < MAX_GEMS; ++i) if (s.gem[i].alive) gfx::sprC(gfx::proj(gfx::PS_GEM), s.gem[i].x, s.gem[i].y);
  enemiesDraw(s);
  for (int i = 0; i < MAX_PROJ; ++i) {
    const Proj& q = s.pr[i];
    if (!q.alive) continue;
    if (q.lob) { float k = q.life; gfx::sprC(gfx::shadow(), q.x, q.y + 6, 0.6f); gfx::sprC(gfx::proj(q.spr), q.x, q.y - 10 + (k > 0 ? 0 : 0), 1.5f, 0xFFFFFFFFu, plat::BLIT_PLAIN, (float)s.frame * 0.4f); }
    else gfx::sprC(gfx::proj(q.spr), q.x, q.y, q.kind == 5 ? 0.8f : 1.0f, 0xFFFFFFFFu, plat::BLIT_PLAIN, q.rot);
  }
  if (s.nOrb > 0)
    for (int o = 0; o < s.nOrb; ++o) {
      float a = s.orb[0].ang + 6.2831853f * o / s.nOrb;
      gfx::sprC(gfx::proj(gfx::PS_KNIFE), p.x + cosf(a) * 34, p.y + sinf(a) * 34,
                2.0f, 0xFFFFFFFFu, plat::BLIT_PLAIN, a + 1.5707963f);
    }
  if (s.m.hasDrone) {
    const float dx = p.x + cosf(p.droneAngle) * 30.0f;
    const float dy = p.y + sinf(p.droneAngle) * 30.0f;
    gfx::sprC(gfx::drone(), dx, dy, 1.0f, 0xFFFFFFFFu, plat::BLIT_PLAIN, p.droneAngle);
  }
  if (p.shield > 0) gfx::ringC(3, p.x, p.y, 13, 200);
  if (s.phase != PH_DEAD) {
    gfx::sprC(gfx::shadow(), p.x, p.y + 8);
    bool blink = p.iframes > 0 && ((s.frame >> 2) & 1);
    if (!blink) {
      if (p.hurtFlash > 0) gfx::sprC(gfx::knight(), p.x, p.y, 1.0f, HEX(C_RED), plat::BLIT_TINT);
      else gfx::sprC(gfx::knight(), p.x, p.y);
    }
  }
  for (int i = 0; i < MAX_FX; ++i) {
    const Fx& f = s.fx[i];
    if (f.kind == FX_NONE) continue;
    float k = f.t / f.life;
    u32 a = (u32)(255 * (1 - k));
    switch (f.kind) {
      case FX_TEXT: gfx::textC(f.txt, f.x, f.y, gfx::FX_PALETTE[f.col], 1, a); break;
      case FX_RING: gfx::ringC(f.col, f.x, f.y, f.r * (0.4f + 0.6f * k), a); break;
      case FX_DISC: gfx::discC(f.col, f.x, f.y, f.r * (0.5f + 0.5f * k), a); break;
      case FX_BOLT: gfx::sprC(gfx::proj(gfx::PS_BOLT), f.x, f.y - 10, 3.0f); break;
    }
  }
  gfx::setOffset(0.0f, 0.0f);
  if (s.season == SEA_ECLIPSE) {                              // darkness: 4 translucent rects around a light square
    const float screenX = p.x - s.camX, screenY = p.y - s.camY;
    const float lx = clampT(screenX - 90, 0.0f, 220.0f), ly = clampT(screenY - 70, 0.0f, 100.0f), lw = 180, lh = 140;
    gfx::rect(0, 0, TOP_W, ly, C_VOID, 170); gfx::rect(0, ly + lh, TOP_W, TOP_H - ly - lh, C_VOID, 170);
    gfx::rect(0, ly, lx, lh, C_VOID, 170); gfx::rect(lx + lw, ly, TOP_W - lx - lw, lh, C_VOID, 170);
  }
  // ---- HUD
  gfx::rect(0, 0, TOP_W, 21, C_VOID);
  char b[48];
  bar(4, 3, 120, 9, p.hp / p.maxhp, C_RED);
  snprintf(b, sizeof b, "%d/%d", (int)ceilf(p.hp), (int)p.maxhp);
  gfx::text(b, 8, 4, C_TEXT);
  bar(4, 14, 120, 5, p.exp / (float)bal::expNeed(p.level), C_EPIC);
  snprintf(b, sizeof b, "%s %d", T(S_plevel), p.level);
  gfx::text(b, 130, 4, C_GOLD);
  if (s.endless) snprintf(b, sizeof b, "%s", Tn(S_wave_n, s.wave));
  else snprintf(b, sizeof b, "%s %d/%d", T(S_wave), s.wave, s.totalWaves);
  gfx::textC(b, 250, 4, C_TEXT);
  int secs = (int)s.runT;
  snprintf(b, sizeof b, "%d:%02d", secs / 60, secs % 60);
  gfx::textR(b, 396, 4, C_DIM);
  if (showFps) { snprintf(b, sizeof b, "%d FPS", fps); gfx::textR(b, 396, 24, C_GREEN); }
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    const Enemy& e = s.en[i];
    if (e.alive && (ENEMIES[e.type].beh & B_BOSS)) {
      bar(100, 226, 200, 8, e.hp / e.maxhp, C_RED);
      gfx::textC(enemyName(e.type), 200, 214, C_GOLD);
      break;
    }
  }
  if (s.toastT > 0) gfx::textC(s.toast, 200, 28, C_GOLD, 1, (u32)(255 * minT(1.0f, s.toastT)));
  if (s.phase == PH_CLEARED) {
    gfx::rect(0, 98, TOP_W, 40, C_VOID, 200);
    gfx::textC(T(S_wave_cleared), 200, 110, C_GOLD, 2);
  }
}
