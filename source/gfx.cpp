#include "gfx.h"
#include <string.h>

namespace gfx {
const u32 FX_PALETTE[FX_COLORS] = {0xFFFFFF, 0xC43D3D, 0xF08030, 0x4A9FD9, 0x4FA85A, 0x8B5FBF, 0xF0E050, 0xE8B54B, 0x8C3DFF, 0xFF55D8};

enum { AW = 256, AH = 256 };
static u32 g_atlas[AW * AH];                 // CPU copy (256 KB). Needed for the knight re-bake.

static Spr g_enemy[NUM_ENEMIES], g_icon[IC_COUNT], g_spellIcon[TY_COUNT][EL_COUNT][ACT_COUNT], g_knight, g_preview, g_drone, g_sentinel, g_proj[PS_COUNT];
static Spr g_disc[FX_COLORS], g_ring[FX_COLORS], g_shadow;
static float g_offX = 0.0f, g_offY = 0.0f;
enum { GLYPH_ASCII = 64, GLYPH_COMP = 12, GLYPH_DEG = GLYPH_ASCII + GLYPH_COMP, GLYPH_COUNT = GLYPH_DEG + 1 };
static Spr g_glyph[GLYPH_COUNT];

// --- packing -------------------------------------------------------------------------------------------
static int px = 0, py = 0, rowH = 0;
static Spr alloc(int w, int h) {
  if (px + w > AW) { px = 0; py += rowH; rowH = 0; }
  Spr s; s.x = (u16)px; s.y = (u16)py; s.w = (u16)w; s.h = (u16)h;
  px += w; if (h > rowH) rowH = h;
  return s;
}
static void put(const Spr& s, int x, int y, u32 rgba) {
  if (x < 0 || y < 0 || x >= s.w || y >= s.h) return;
  g_atlas[(s.y + y) * AW + s.x + x] = rgba;
}
static void fillR(const Spr& s, int x, int y, int w, int h, u32 rgb, u32 a = 255) {
  for (int j = 0; j < h; ++j) for (int i = 0; i < w; ++i) put(s, x + i, y + j, HEX(rgb, a));
}
static void paintArt(const Spr& s, const PixArt& a) {
  for (int i = 0; i < a.n; ++i) fillR(s, a.r[i].x, a.r[i].y, a.r[i].w, a.r[i].h, a.r[i].rgb);
}

// --- 5x7 font (rows are 5-bit, bit4 = leftmost), ASCII 32..95 ----------------------------------------------
static const u8 FONT[64][7] = {
    {0, 0, 0, 0, 0, 0, 0},                          // space
    {4, 4, 4, 4, 4, 0, 4},                          // !
    {10, 10, 10, 0, 0, 0, 0},                       // "
    {10, 10, 31, 10, 31, 10, 10},                   // #
    {4, 15, 20, 14, 5, 30, 4},                      // $
    {24, 25, 2, 4, 8, 19, 3},                       // %
    {12, 18, 20, 8, 21, 18, 13},                    // &
    {4, 4, 8, 0, 0, 0, 0},                          // '
    {2, 4, 8, 8, 8, 4, 2},                          // (
    {8, 4, 2, 2, 2, 4, 8},                          // )
    {0, 4, 21, 14, 21, 4, 0},                       // *
    {0, 4, 4, 31, 4, 4, 0},                         // +
    {0, 0, 0, 0, 12, 4, 8},                         // ,
    {0, 0, 0, 31, 0, 0, 0},                         // -
    {0, 0, 0, 0, 0, 12, 12},                        // .
    {0, 1, 2, 4, 8, 16, 0},                         // /
    {14, 17, 19, 21, 25, 17, 14},                   // 0
    {4, 12, 4, 4, 4, 4, 14},                        // 1
    {14, 17, 1, 2, 4, 8, 31},                       // 2
    {31, 2, 4, 2, 1, 17, 14},                       // 3
    {2, 6, 10, 18, 31, 2, 2},                       // 4
    {31, 16, 30, 1, 1, 17, 14},                     // 5
    {6, 8, 16, 30, 17, 17, 14},                     // 6
    {31, 1, 2, 4, 8, 8, 8},                         // 7
    {14, 17, 17, 14, 17, 17, 14},                   // 8
    {14, 17, 17, 15, 1, 2, 12},                     // 9
    {0, 12, 12, 0, 12, 12, 0},                      // :
    {0, 12, 12, 0, 12, 4, 8},                       // ;
    {2, 4, 8, 16, 8, 4, 2},                         // <
    {0, 0, 31, 0, 31, 0, 0},                        // =
    {8, 4, 2, 1, 2, 4, 8},                          // >
    {14, 17, 1, 2, 4, 0, 4},                        // ?
    {14, 17, 23, 21, 23, 16, 14},                   // @
    {14, 17, 17, 31, 17, 17, 17},                   // A
    {30, 17, 17, 30, 17, 17, 30},                   // B
    {14, 17, 16, 16, 16, 17, 14},                   // C
    {30, 17, 17, 17, 17, 17, 30},                   // D
    {31, 16, 16, 30, 16, 16, 31},                   // E
    {31, 16, 16, 30, 16, 16, 16},                   // F
    {14, 17, 16, 23, 17, 17, 15},                   // G
    {17, 17, 17, 31, 17, 17, 17},                   // H
    {14, 4, 4, 4, 4, 4, 14},                        // I
    {7, 2, 2, 2, 2, 18, 12},                        // J
    {17, 18, 20, 24, 20, 18, 17},                   // K
    {16, 16, 16, 16, 16, 16, 31},                   // L
    {17, 27, 21, 21, 17, 17, 17},                   // M
    {17, 17, 25, 21, 19, 17, 17},                   // N
    {14, 17, 17, 17, 17, 17, 14},                   // O
    {30, 17, 17, 30, 16, 16, 16},                   // P
    {14, 17, 17, 17, 21, 18, 13},                   // Q
    {30, 17, 17, 30, 20, 18, 17},                   // R
    {15, 16, 16, 14, 1, 1, 30},                     // S
    {31, 4, 4, 4, 4, 4, 4},                         // T
    {17, 17, 17, 17, 17, 17, 14},                   // U
    {17, 17, 17, 17, 17, 10, 4},                    // V
    {17, 17, 17, 21, 21, 21, 10},                   // W
    {17, 17, 10, 4, 10, 17, 17},                    // X
    {17, 17, 10, 4, 4, 4, 4},                       // Y
    {31, 1, 2, 4, 8, 16, 31},                       // Z
    {14, 8, 8, 8, 8, 8, 14},                        // [
    {0, 16, 8, 4, 2, 1, 0},                         // backslash
    {14, 2, 2, 2, 2, 2, 14},                        // ]
    {4, 10, 17, 0, 0, 0, 0},                        // ^
    {0, 0, 0, 0, 0, 0, 31},                         // _
};
enum Accent { AC_ACUTE, AC_GRAVE, AC_CIRC, AC_TILDE, AC_CEDIL };
struct Comp { u32 cp[2]; char base; u8 accent; };      // upper-case and lower-case code point
static const Comp COMPS[GLYPH_COMP] = {
    {{0xC1, 0xE1}, 'A', AC_ACUTE}, {{0xC0, 0xE0}, 'A', AC_GRAVE}, {{0xC2, 0xE2}, 'A', AC_CIRC}, {{0xC3, 0xE3}, 'A', AC_TILDE},
    {{0xC7, 0xE7}, 'C', AC_CEDIL}, {{0xC9, 0xE9}, 'E', AC_ACUTE}, {{0xCA, 0xEA}, 'E', AC_CIRC}, {{0xCD, 0xED}, 'I', AC_ACUTE},
    {{0xD3, 0xF3}, 'O', AC_ACUTE}, {{0xD4, 0xF4}, 'O', AC_CIRC}, {{0xD5, 0xF5}, 'O', AC_TILDE}, {{0xDA, 0xFA}, 'U', AC_ACUTE}};

static int glyphForCp(u32 cp) {
  if (cp >= 'a' && cp <= 'z') cp = cp - 'a' + 'A';
  if (cp >= 32 && cp <= 95) return (int)cp - 32;
  if (cp == '{') return '(' - 32;
  if (cp == '}') return ')' - 32;
  if (cp == '|') return 'I' - 32;
  if (cp == '~') return '-' - 32;
  if (cp == '`') return '\'' - 32;
  if (cp == 0xB0 || cp == 0xBA) return GLYPH_DEG;
  for (int i = 0; i < GLYPH_COMP; ++i) if (COMPS[i].cp[0] == cp || COMPS[i].cp[1] == cp) return GLYPH_ASCII + i;
  return '?' - 32;
}
static u32 nextCp(const char*& s) {
  const u8* p = (const u8*)s;
  u32 c = *p;
  if (c < 0x80) { s += 1; return c; }
  if ((c & 0xE0) == 0xC0 && p[1]) { s += 2; return ((c & 0x1F) << 6) | (p[1] & 0x3F); }
  if ((c & 0xF0) == 0xE0 && p[1] && p[2]) { s += 3; return ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); }
  s += 1; return '?';
}

static void bakeGlyph(const Spr& s, char base, int accent, bool degree) {
  u32 W = 0xFFFFFFFFu;
  if (degree) {
    static const u8 deg[4] = {12, 18, 18, 12};
    for (int r = 0; r < 4; ++r) for (int c = 0; c < 5; ++c) if (deg[r] & (16 >> c)) put(s, c, 2 + r, W);
    return;
  }
  const u8* rows = FONT[base - 32];
  for (int r = 0; r < 7; ++r) for (int c = 0; c < 5; ++c) if (rows[r] & (16 >> c)) put(s, c, 2 + r, W);
  static const u8 acc[4][2] = {{2, 4}, {8, 4}, {4, 10}, {13, 22}};     // two rows of 5 bits
  if (accent >= 0 && accent <= AC_TILDE)
    for (int r = 0; r < 2; ++r) for (int c = 0; c < 5; ++c) if (acc[accent][r] & (16 >> c)) put(s, c, r, W);
  if (accent == AC_CEDIL) { put(s, 2, 9, W); put(s, 3, 9, W); }
}

// --- procedural spell icons: shape by type, colour by element, pattern by activation ------------------------
static void bakeSpellIcon(const Spr& s, int ty, int el, int act) {
  u32 col = ELEM_COLOR[el];
  auto inside = [&](float x, float y) -> bool {
    float dx = x + 0.5f - 8.0f, dy = y + 0.5f - 8.0f;
    if (ty == TY_DAMAGE) return fabsf(dx) + fabsf(dy) <= 6.6f;
    if (ty == TY_STATUS) return dx * dx + dy * dy <= 6.6f * 6.6f;
    if (dy < -3.0f) return dy >= -7.0f && fabsf(dx) <= (dy + 7.5f) * 0.65f;      // weapon: pointed bar
    return dy <= 7.0f && fabsf(dx) <= 2.6f;
  };
  for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) {
    if (!inside((float)x, (float)y)) continue;
    bool edge = !inside(x - 1.0f, (float)y) || !inside(x + 1.0f, (float)y) || !inside((float)x, y - 1.0f) || !inside((float)x, y + 1.0f);
    u32 c = edge ? 0x0B0A12 : col;
    if (!edge && (x + y) < 11) c = ((((col >> 16) & 255) + 255 * 2) / 3) << 16 | ((((col >> 8) & 255) + 255 * 2) / 3) << 8 | (((col & 255) + 255 * 2) / 3);
    put(s, x, y, HEX(c));
  }
  u32 W = HEX(C_TEXT);
  if (act == ACT_SELF) { put(s, 7, 7, W); put(s, 8, 7, W); put(s, 7, 8, W); put(s, 8, 8, W); }
  else if (act == ACT_TOUCH) { for (int i = 0; i < 6; ++i) put(s, 5 + i, 5 + i, W); }
  else { for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) {
      float dx = x + 0.5f - 8.0f, dy = y + 0.5f - 8.0f, d = sqrtf(dx * dx + dy * dy);
      if (fabsf(d - 3.0f) < 0.75f) put(s, x, y, W);
    } }
}

static void bakeDisc(const Spr& s, u32 rgb, bool ringOnly) {
  for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x) {
    float dx = x + 0.5f - 12.0f, dy = y + 0.5f - 12.0f, d = sqrtf(dx * dx + dy * dy);
    if (!ringOnly && d <= 11.5f) put(s, x, y, HEX(rgb, d > 9.5f ? 150 : 90));
    if (ringOnly && d <= 11.5f && d >= 9.6f) put(s, x, y, HEX(rgb, 230));
  }
}

static void bakeProjectiles() {
  struct R { int x, y, w, h; u32 c; };
  for (int i = 0; i < PS_COUNT; ++i) g_proj[i] = alloc(8, 8);
  const u32 SIL = 0xC8D0D8, GOLD = 0xE8B54B, BRN = 0x6B4A2E, WOOD = 0xA07A50;
  fillR(g_proj[PS_DAGGER], 3, 0, 2, 5, SIL); fillR(g_proj[PS_DAGGER], 2, 5, 4, 1, GOLD); fillR(g_proj[PS_DAGGER], 3, 6, 2, 2, BRN);
  fillR(g_proj[PS_ARROW], 3, 0, 2, 2, SIL); fillR(g_proj[PS_ARROW], 3, 2, 2, 4, WOOD); fillR(g_proj[PS_ARROW], 2, 6, 4, 2, C_TEXT);
  fillR(g_proj[PS_AXE], 1, 0, 6, 4, SIL); fillR(g_proj[PS_AXE], 0, 1, 2, 2, C_TEXT); fillR(g_proj[PS_AXE], 3, 4, 2, 4, BRN);
  { const Spr& s = g_proj[PS_ORB]; fillR(s, 2, 1, 4, 6, C_EPIC); fillR(s, 1, 2, 6, 4, C_EPIC); fillR(s, 3, 3, 2, 2, C_TEXT); }
  fillR(g_proj[PS_PELLET], 2, 2, 4, 4, 0xE8B54B); fillR(g_proj[PS_PELLET], 3, 3, 2, 2, C_TEXT);
  fillR(g_proj[PS_PISTOL], 3, 0, 2, 5, 0xC8D0D8); fillR(g_proj[PS_PISTOL], 2, 5, 4, 1, C_TEXT); fillR(g_proj[PS_PISTOL], 3, 6, 2, 2, 0x6B4A2E);
  fillR(g_proj[PS_TRIDENT], 3, 0, 2, 6, 0xC8D0D8); fillR(g_proj[PS_TRIDENT], 1, 0, 2, 2, 0xC8D0D8); fillR(g_proj[PS_TRIDENT], 5, 0, 2, 2, 0xC8D0D8); fillR(g_proj[PS_TRIDENT], 2, 6, 4, 1, 0x6B4A2E); fillR(g_proj[PS_TRIDENT], 3, 7, 2, 1, 0x6B4A2E);
  { const Spr& s = g_proj[PS_SHARD]; fillR(s, 3, 0, 2, 8, 0xE04A4A); fillR(s, 1, 2, 6, 4, 0xE04A4A); fillR(s, 3, 3, 2, 2, 0xFFD0D0); }
  { const Spr& s = g_proj[PS_WEB]; fillR(s, 1, 1, 6, 6, 0xD8D8E8); fillR(s, 2, 2, 4, 4, 0x6B6480); fillR(s, 3, 3, 2, 2, 0xD8D8E8); }
  { const Spr& s = g_proj[PS_BOLT]; fillR(s, 4, 0, 2, 3, 0xF0E050); fillR(s, 2, 3, 4, 2, 0xF0E050); fillR(s, 2, 5, 2, 3, 0xF0E050); fillR(s, 3, 3, 1, 2, C_TEXT); }
  { const Spr& s = g_proj[PS_EMBER]; fillR(s, 2, 2, 4, 4, 0xF08030); fillR(s, 3, 3, 2, 2, 0xF0E050); }
  { const Spr& s = g_proj[PS_GEM]; fillR(s, 3, 0, 2, 8, C_EPIC); fillR(s, 1, 2, 6, 4, C_EPIC); fillR(s, 3, 2, 1, 2, 0xD8C8F0); }
  { const Spr& s = g_proj[PS_SPARK]; fillR(s, 3, 0, 2, 8, C_TEXT); fillR(s, 0, 3, 8, 2, C_TEXT); }
  { const Spr& s = g_proj[PS_KNIFE]; fillR(s, 3, 0, 2, 5, SIL); fillR(s, 2, 4, 4, 1, 0xE8EEF4); fillR(s, 2, 5, 4, 1, GOLD); fillR(s, 3, 6, 2, 2, BRN); }
  { const Spr& s = g_proj[PS_TNT]; fillR(s, 1, 2, 6, 6, 0xB53C36); fillR(s, 2, 3, 2, 4, 0xD65A48); fillR(s, 5, 3, 1, 4, 0x702B32); fillR(s, 3, 1, 2, 2, WOOD); fillR(s, 4, 0, 3, 1, 0xF0E050); fillR(s, 6, 1, 1, 1, 0xF08030); }
}

static void bakeKnightCell(const Spr& s, const Cosmetics& c) {
  for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) put(s, x, y, 0);
  const u32 K = 0x0B0A12, BRN = 0x6B4A2E, SKIN = 0xF0C8A0, BOOT = 0x3A3352;
  if (c.hat >= 5) {
    paintArt(s, ART_HAT[c.hat % 11]);
    return;
  }
  fillR(s, 5, 13, 2, 2, BOOT); fillR(s, 9, 13, 2, 2, BOOT);
  fillR(s, 4, 9, 8, 5, K); fillR(s, 5, 9, 6, 4, HAT_COLORS[c.color % 6]); fillR(s, 5, 11, 6, 1, BRN);
  fillR(s, 2, 10, 2, 2, SKIN); fillR(s, 12, 10, 2, 2, SKIN);
  fillR(s, 3, 2, 10, 8, K); fillR(s, 4, 3, 8, 6, SKIN); fillR(s, 5, 6, 1, 1, K); fillR(s, 10, 6, 1, 1, K);
  if (ART_FACE[c.face % 6].r) paintArt(s, ART_FACE[c.face % 6]);
  paintArt(s, ART_HAT[c.hat % 11]);
}

static void bakeDroneCell() {
  const Spr& s = g_drone;
  const u32 dark = 0x252936, metal = 0x8E98A8, light = 0xD2D9E0, glow = 0x61D7E8;
  fillR(s, 3, 4, 10, 8, dark);
  fillR(s, 4, 5, 8, 6, metal);
  fillR(s, 5, 6, 6, 2, light);
  fillR(s, 6, 9, 4, 1, 0x596273);
  fillR(s, 12, 7, 3, 2, dark);
  fillR(s, 14, 7, 2, 2, glow);
  fillR(s, 1, 2, 5, 1, dark); fillR(s, 10, 2, 5, 1, dark);
  fillR(s, 2, 2, 1, 2, metal); fillR(s, 13, 2, 1, 2, metal);
  fillR(s, 7, 3, 2, 1, glow);
}

static void bakeSentinelCell() {
  const Spr& s = g_knight;
  const u32 edge = 0x252936, stone = 0x87909D, light = 0xC8D0D8, shade = 0x596273, glow = 0xE8B54B;
  fillR(s, 4, 1, 8, 13, edge);
  fillR(s, 5, 2, 6, 11, stone);
  fillR(s, 6, 3, 4, 3, light);
  fillR(s, 6, 7, 1, 2, glow); fillR(s, 9, 7, 1, 2, glow);
  fillR(s, 6, 10, 4, 1, shade);
  fillR(s, 3, 13, 10, 2, edge); fillR(s, 2, 15, 12, 1, shade);
  fillR(s, 3, 4, 2, 5, shade); fillR(s, 11, 4, 2, 5, shade);
}

bool init(const Cosmetics& c) {
  memset(g_atlas, 0, sizeof(g_atlas));
  px = py = rowH = 0;
  // Hardcode glyphs into an 8x16 grid (perfect 8x8 tile alignment)
  for (int i = 0; i < GLYPH_COUNT; ++i) {
    g_glyph[i].x = (u16)((i % 32) * 8);
    g_glyph[i].y = (u16)((i / 32) * 16);
    g_glyph[i].w = 6;
    g_glyph[i].h = 10;
  }
  for (int i = 0; i < GLYPH_ASCII; ++i) bakeGlyph(g_glyph[i], (char)(32 + i), -1, false);
  for (int i = 0; i < GLYPH_COMP; ++i) bakeGlyph(g_glyph[GLYPH_ASCII + i], COMPS[i].base, COMPS[i].accent, false);
  bakeGlyph(g_glyph[GLYPH_DEG], 0, -1, true);
  
  // Start packing the rest of the atlas BELOW the text (48 pixels down)
  px = 0; py = 48; rowH = 0;
  for (int i = 0; i < NUM_ENEMIES; ++i) { g_enemy[i] = alloc(16, 16); paintArt(g_enemy[i], ART_ENEMY[i]); }
  for (int i = 0; i < IC_COUNT; ++i) { g_icon[i] = alloc(16, 16); paintArt(g_icon[i], ART_ICON[i]); }
  g_knight = alloc(16, 16);
  g_preview = alloc(16, 16);
  g_drone = alloc(16, 16); bakeDroneCell();
  g_sentinel = alloc(16, 16); bakeSentinelCell();
  for (int t = 0; t < TY_COUNT; ++t) for (int e = 0; e < EL_COUNT; ++e) for (int a = 0; a < ACT_COUNT; ++a) {
    g_spellIcon[t][e][a] = alloc(16, 16); bakeSpellIcon(g_spellIcon[t][e][a], t, e, a);
  }
  px = 0; py += rowH; rowH = 0;
  bakeProjectiles();
  for (int i = 0; i < FX_COLORS; ++i) { g_disc[i] = alloc(24, 24); bakeDisc(g_disc[i], FX_PALETTE[i], false); }
  for (int i = 0; i < FX_COLORS; ++i) { g_ring[i] = alloc(24, 24); bakeDisc(g_ring[i], FX_PALETTE[i], true); }
  g_shadow = alloc(16, 6);
  for (int y = 0; y < 6; ++y) for (int x = 0; x < 16; ++x) {
    float dx = (x + 0.5f - 8.0f) / 8.0f, dy = (y + 0.5f - 3.0f) / 3.0f;
    if (dx * dx + dy * dy <= 1.0f) put(g_shadow, x, y, HEX(0x000000, 90));
  }
  bakeKnightCell(g_knight, c);
  bakeKnightCell(g_preview, c);
  if (py + rowH > AH) return false;                       // atlas overflow = build-time bug
  plat::atlasUpload(g_atlas, AW, AH);
  return true;
}
void setKnight(const Cosmetics& c) {
  bakeKnightCell(g_knight, c);
  plat::atlasUpdate(g_knight.x, g_knight.y, 16, 16, g_atlas + g_knight.y * AW + g_knight.x, AW);
}
const Spr& enemy(int id) { return g_enemy[clampT(id, 0, NUM_ENEMIES - 1)]; }
const Spr& icon(int id) { return g_icon[clampT(id, 0, (int)IC_COUNT - 1)]; }
const Spr& spellIcon(int t, int e, int a) { return g_spellIcon[clampT(t, 0, 2)][clampT(e, 0, 5)][clampT(a, 0, 2)]; }
const Spr& knight() { return g_knight; }
const Spr& knightPreview(const Cosmetics& c) {
  bakeKnightCell(g_preview, c);
  plat::atlasUpdate(g_preview.x, g_preview.y, 16, 16, g_atlas + g_preview.y * AW + g_preview.x, AW);
  return g_preview;
}
const Spr& clothingPreview(int hat) {
  for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) put(g_preview, x, y, 0);
  paintArt(g_preview, ART_HAT[clampT(hat, 0, 10)]);
  plat::atlasUpdate(g_preview.x, g_preview.y, 16, 16, g_atlas + g_preview.y * AW + g_preview.x, AW);
  return g_preview;
}
const Spr& drone() { return g_drone; }
const Spr& sentinel() { return g_sentinel; }
const Spr& proj(int id) { return g_proj[clampT(id, 0, (int)PS_COUNT - 1)]; }
const Spr& disc(int i) { return g_disc[clampT(i, 0, FX_COLORS - 1)]; }
const Spr& ring(int i) { return g_ring[clampT(i, 0, FX_COLORS - 1)]; }
const Spr& shadow() { return g_shadow; }

// --- drawing ---------------------------------------------------------------------------------------------
void setOffset(float x, float y) { g_offX = x; g_offY = y; }
void rect(float x, float y, float w, float h, u32 rgb, u32 a) { plat::rect(x + g_offX, y + g_offY, w, h, HEX(rgb, a)); }
void panel(float x, float y, float w, float h, u32 fill, u32 border) {
  plat::rect(x + g_offX, y + g_offY, w, h, HEX(border));
  plat::rect(x + 1 + g_offX, y + 1 + g_offY, w - 2, h - 2, HEX(fill));
}
void spr(const Spr& s, float x, float y, float scale, u32 color, int mode, float rot) {
  plat::blit(s.x, s.y, s.w, s.h, x + g_offX, y + g_offY, s.w * scale, s.h * scale, color, mode, rot);
}
void sprC(const Spr& s, float cx, float cy, float scale, u32 color, int mode, float rot) {
  float w = s.w * scale, h = s.h * scale;
  plat::blit(s.x, s.y, s.w, s.h, cx - w * 0.5f + g_offX, cy - h * 0.5f + g_offY, w, h, color, mode, rot);
}
void discC(int ci, float cx, float cy, float r, u32 alpha) {
  const Spr& s = disc(ci);
  plat::blit(s.x, s.y, s.w, s.h, cx - r + g_offX, cy - r + g_offY, r * 2, r * 2, withAlpha(0xFFFFFF, alpha), alpha >= 255 ? plat::BLIT_PLAIN : plat::BLIT_ALPHA, 0);
}
void ringC(int ci, float cx, float cy, float r, u32 alpha) {
  const Spr& s = ring(ci);
  plat::blit(s.x, s.y, s.w, s.h, cx - r + g_offX, cy - r + g_offY, r * 2, r * 2, withAlpha(0xFFFFFF, alpha), alpha >= 255 ? plat::BLIT_PLAIN : plat::BLIT_ALPHA, 0);
}

int textWidth(const char* s, int scale) {
  int n = 0;
  while (*s) { nextCp(s); ++n; }
  return n * GLYPH_W * scale;
}
void text(const char* s, float x, float y, u32 rgb, int scale, u32 a) {
  u32 col = HEX(rgb, a);
  x += g_offX; y += g_offY;
  while (*s) {
    u32 cp = nextCp(s);
    if (cp != ' ') {
      const Spr& g = g_glyph[glyphForCp(cp)];
      plat::blit(g.x, g.y, g.w, g.h, x, y, (float)(g.w * scale), (float)(g.h * scale),
            col, plat::BLIT_TINT, 0);
    }
    x += GLYPH_W * scale;
  }
}
void textC(const char* s, float cx, float y, u32 rgb, int scale, u32 a) { text(s, cx - textWidth(s, scale) * 0.5f, y, rgb, scale, a); }
void textR(const char* s, float rx, float y, u32 rgb, int scale, u32 a) { text(s, rx - textWidth(s, scale), y, rgb, scale, a); }

int textWrap(const char* s, float x, float y, float w, u32 rgb, int scale, int maxLines, int skipLines, int* totalLines) {
  const int maxCols = (int)(w / (GLYPH_W * scale));
  char line[160];
  int len = 0, lineNo = 0, drawn = 0, lineCols = 0;
  auto flush = [&]() {
    line[len] = 0;
    if (lineNo >= skipLines && (maxLines <= 0 || drawn < maxLines)) {
      text(line, x, y + drawn * (LINE_H * scale), rgb, scale);
      ++drawn;
    }
    ++lineNo; len = 0; lineCols = 0;
  };
  const char* p = s;
  while (*p) {
    // measure next word
    const char* q = p; int wc = 0;
    while (*q && *q != ' ' && *q != '\n') { nextCp(q); ++wc; }
    int wbytes = (int)(q - p);
    if (lineCols > 0 && lineCols + 1 + wc > maxCols) flush();
    if (lineCols > 0 && len < 158) { line[len++] = ' '; ++lineCols; }
    if (len + wbytes < 158) { memcpy(line + len, p, (size_t)wbytes); len += wbytes; lineCols += wc; }
    p = q;
    if (*p == '\n') { flush(); ++p; }
    else if (*p == ' ') ++p;
  }
  if (len > 0) flush();
  if (totalLines) *totalLines = lineNo;
  return drawn;
}
}  // namespace gfx
