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

static float auraRadius(const Sim& s, const SpellSlot& slot, const Eff& e) {
  const float mult = (1.0f + LV_BONUS * (slot.lv - 1)) * (1.0f + s.m.allNumPct / 100.0f);
  switch (e.kind) {
    case E_AURA: return e.b * mult * s.m.aoeMul;
    case E_AURA_PULL: return e.a * mult * s.m.aoeMul;
    case E_AURA_WEAKEN: case E_AURA_MISS: return e.b * mult * s.m.aoeMul;
    case E_AURA_HEALKILL: return 56.0f * s.m.aoeMul;
    default: return 0.0f;
  }
}

static int auraColor(const SpellDef& d, u8 kind) {
  switch (kind) {
    case E_AURA_PULL: return 5;
    case E_AURA_WEAKEN: return 0; // Vacuum is a white outward-pressure field.
    case E_AURA_MISS: return 3;
    case E_AURA_HEALKILL: return 1;
    default:
      switch (d.el) {
        case EL_FIRE: return 2;
        case EL_EARTH: return 4;
        case EL_AIR: return 0;
        case EL_BLOOD: return 1;
        case EL_LIGHTNING: return 6;
        default: return 3;
      }
  }
}

static void drawAuras(const Sim& s) {
  int shown = 0;
  for (int i = 0; i < s.nSlots && shown < 12; ++i) {
    const SpellSlot& slot = s.slots[i];
    if (slot.id < 0) continue;
    const SpellDef& d = SPELLS[slot.id];
    for (int k = 0; k < 3 && shown < 12; ++k) {
      const Eff& e = d.eff[k];
      if (e.kind != E_AURA && e.kind != E_AURA_PULL && e.kind != E_AURA_WEAKEN && e.kind != E_AURA_MISS && e.kind != E_AURA_HEALKILL) continue;
      const float radius = auraRadius(s, slot, e);
      if (radius < 8.0f) continue;
      const int color = auraColor(d, e.kind);
      const float pulse = 0.5f + 0.5f * sinf((float)s.frame * 0.14f + i * 1.13f + k * 2.1f);
      const float outer = radius * (0.97f + 0.035f * pulse);
      const u32 alpha = (u32)(150.0f + 90.0f * pulse);
      gfx::discC(color, s.p.x, s.p.y, outer, e.kind == E_AURA_WEAKEN ? 22 : 16);
      gfx::ringC(color, s.p.x, s.p.y, outer, alpha);
      gfx::ringC(color, s.p.x, s.p.y, radius * (0.63f + 0.04f * pulse), (u32)(alpha * 0.25f));
      ++shown;
    }
  }
}

static void drawLightningArc(const Sim& s, float x0, float y0, float x1, float y1, int style) {
  const float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy);
  if (len < 5.0f) return;
  const float px = -dy / len, py = dx / len;
  for (int j = 0; j <= 8; ++j) {
    const float t = j / 8.0f;
    const float zig = (j == 0 || j == 8) ? 0.0f : ((j & 1) ? 3.8f : -3.8f) + 0.8f * sinf((float)s.frame * 0.9f + j * 1.7f + style);
    const float x = x0 + dx * t + px * zig, y = y0 + dy * t + py * zig;
    gfx::discC(6, x, y, 2.8f, 230);
    gfx::discC(0, x, y, 1.0f, 245);
  }
}

static void drawAuraAccents(const Sim& s) {
  bool vacuumDrawn = false;
  int boltCount = 0;
  for (int i = 0; i < s.nSlots; ++i) {
    const SpellSlot& slot = s.slots[i];
    if (slot.id < 0) continue;
    const SpellDef& d = SPELLS[slot.id];
    for (int k = 0; k < 3; ++k) {
      const Eff& e = d.eff[k];
      const float radius = auraRadius(s, slot, e);
      if (e.kind == E_AURA_WEAKEN && !vacuumDrawn && radius > 12.0f) {
        const int count = 10;
        for (int p = 0; p < count; ++p) {
          float phase = fmodf((float)s.frame * 0.035f + p / (float)count, 1.0f);
          float angle = 6.2831853f * p / count + (float)s.frame * 0.012f;
          float distance = 9.0f + phase * (radius - 9.0f);
          float rx = cosf(angle), ry = sinf(angle);
          float x = s.p.x + rx * distance, y = s.p.y + ry * distance;
          u32 alpha = (u32)(225.0f - phase * 95.0f);
          gfx::discC(0, x, y, 1.35f, alpha);
          gfx::discC(0, x - rx * 3.0f, y - ry * 3.0f, 0.7f, alpha / 2);
        }
        vacuumDrawn = true;
      }
      if (e.kind != E_AURA || d.el != EL_LIGHTNING || boltCount >= 2 || ((s.frame + i * 7) % 18) >= 10) continue;
      int target = -1; float best = radius * radius;
      for (int n = 0; n < MAX_ENEMIES; ++n) if (s.en[n].alive) {
        const float ex = s.en[n].x - s.p.x, ey = s.en[n].y - s.p.y, d2 = ex * ex + ey * ey;
        if (d2 < best) { best = d2; target = n; }
      }
      if (target >= 0) {
        drawLightningArc(s, s.p.x, s.p.y, s.en[target].x, s.en[target].y, boltCount);
        ++boltCount;
      }
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
  drawAuras(s);
  if (s.fairy.active) { gfx::discC(4, s.fairy.x, s.fairy.y, 5, 220); gfx::ringC(7, s.fairy.x, s.fairy.y, 8, 180); }
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
  drawAuraAccents(s);
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
      case FX_SLASH: {
        const float progress = clampT(f.t / f.life, 0.0f, 1.0f);
        const float fade = 1.0f - progress;
        const float center = f.ang + (progress - 0.5f) * 0.85f;
        const float span = 1.90f, start = center - span * 0.5f;
        const float pi = 3.14159265f;
        for (int j = 0; j <= 12; ++j) {
          const float u = j / 12.0f;
          const float ang = start + span * u;
          const float taper = 0.68f + 0.32f * sinf(pi * u);
          const float x = f.x + cosf(ang) * f.r, y = f.y + sinf(ang) * f.r;
          gfx::discC(8, x, y, 4.4f * taper, (u32)(fade * 105.0f));
          gfx::discC(9, x, y, 2.9f * taper, (u32)(fade * 240.0f));
          if ((j & 1) == 0) gfx::discC(0, x, y, 1.15f * taper, (u32)(fade * 230.0f));
        }
        for (int j = 0; j < 5; ++j) {
          const float u = (j + 1.0f) / 6.0f;
          const float ang = start + span * u;
          const float rr = f.r + 6.0f + (j & 1) * 3.0f;
          gfx::discC((j & 1) ? 0 : 9, f.x + cosf(ang) * rr, f.y + sinf(ang) * rr,
                     0.9f + (j % 3) * 0.25f, (u32)(fade * 190.0f));
        }
        break;
      }
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
  if (s.weapon == W_TRIDENT && p.tridentOut) gfx::textC(T(S_trident_out), 200, 24, C_GOLD);
  if (p.timeStopT > 0.0f) gfx::textC("TIME STOP", 200, 38, C_EPIC, 1);
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
