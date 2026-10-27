#include "game.h"
#include "sim.h"
#include "gfx.h"
#include "i18n.h"
#include "audio.h"
#include "save.h"
#include <stdio.h>
#include <string.h>

namespace {
using plat::Input;
enum Screen { SC_MENU, SC_CAMPAIGN, SC_WEAPON, SC_CUSTOM, SC_GAME, SC_CATALOG, SC_OPTIONS, SC_CREDITS };

struct G {
  Screen scr = SC_MENU, back = SC_MENU;
  SaveData save;
  bool endless = false, allSeasons = false;
  float fpsElapsed = 0.0f; unsigned int fpsFrames = 0; int fps = 0;
  int season = 0, level = 0, weaponSel = 0, menuSel = 0;
  int tab = 0, slotSel = 0, allocSel = 0, offerSel = 0;
  int catTab = 0, catSel = 0, catScroll = 0, optSel = 0, custRow = 0, loreScroll = 0;
  bool paused = false, debug = false; int pauseSel = 0, dbgSel = 0;
  char toast[48] = ""; float toastT = 0;
  bool inited = false, silent = false;
  int droneClicks = 0;         
  bool secretCatalog = false;  
};
G g;
Sim g_sim;

// ---------------------------------------------------------------------------------------------- helpers
bool inR(const Input& in, int x, int y, int w, int h) { return in.touchDown && in.tx >= x && in.tx < x + w && in.ty >= y && in.ty < y + h; }
bool pressed(const Input& in, u32 b) { return (in.down & b) != 0; }
void toast(const char* t) { snprintf(g.toast, sizeof g.toast, "%s", t); g.toastT = 2.5f; }
void persist() { if (!saveStore(g.save)) toast(T(S_save_failed)); }
void applyOptions() {
  g_lang = g.save.lang; audio::setVolumes(g.save.musicVol, g.save.sfxVol); plat::setTargetFps(g.save.fps30 ? 30 : 60);
}
void go(Screen s) {
  g.scr = s; g.menuSel = (s == SC_MENU) ? g.menuSel : 0;
  if (s != SC_GAME) audio::musicSet(s == SC_MENU ? -1 : (s == SC_CAMPAIGN ? g.season : -1), 0.3f);
}
gfx::Cosmetics cosm() { gfx::Cosmetics c; c.hat = g.save.hat; c.face = g.save.face; c.color = g.save.color; return c; }
bool spellFound(int i) { return g.save.unlockAll || g.secretCatalog || g.save.spellFound[i] > 0; }
bool enemyFound(int i) { return g.save.unlockAll || g.secretCatalog || g.save.enemySeen[i]; }

void startRun() {
  g.save.lastWeapon = (u8)g.weaponSel;
  simStart(g_sim, g.season, g.level, g.endless, g.weaponSel, (u32)plat::nowMs() | 1u, g.allSeasons);
  g.scr = SC_GAME; g.tab = 0; g.slotSel = 0; g.allocSel = 0; g.offerSel = 0; g.paused = false; g.debug = false;
  audio::musicSet(g.season, 0.4f);
}
void finishRun(bool victory) {
  Sim& s = g_sim;
  for (int i = 0; i < NUM_ENEMIES; ++i) {
    if (s.seenEnemy[i]) g.save.enemySeen[i] = 1;
    int k = (int)g.save.enemyKills[i] + s.runKills[i]; g.save.enemyKills[i] = (u16)minT(k, 65535);
  }
  for (int i = 0; i < NUM_SPELLS; ++i) if (s.seenSpell[i]) g.save.spellFound[i] = (u16)minT((int)g.save.spellFound[i] + s.seenSpell[i], 65535);
  if (s.endless) g.save.bestWave[s.season] = (u16)maxT((int)g.save.bestWave[s.season], minT(s.wave, 65535));
  if (victory && !s.endless) g.save.levelCleared[s.season] |= (u8)(1 << s.levelIdx);
  persist();
  g.season = s.season;
  go(s.endless ? SC_MENU : SC_CAMPAIGN);
}

int seasonIcon(int s) { static const int ic[5] = {IC_LEAF, IC_PYR, IC_SNOW, IC_PEAK, IC_ECL}; return ic[s]; }
StrId gimmick(int s) { static const StrId g_[5] = {S_gim_forest, S_gim_desert, S_gim_snow, S_gim_mountain, S_gim_eclipse}; return g_[s]; }
StrId speedWord(int w) { static const StrId t[7] = {S_lvf_stationary, S_lvf_very_slow, S_lvf_slow, S_lvf_low, S_lvf_normal, S_lvf_fast, S_lvf_very_fast}; return t[w % 7]; }
StrId rangeWord(int w) { static const StrId t[3] = {S_lvm_touch, S_lvm_high, S_lvm_very_high}; return t[w % 3]; }

void button(float x, float y, float w, float h, const char* label, bool sel, u32 col = C_TEXT) {
  gfx::panel(x, y, w, h, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : (u32)C_BORDER);
  gfx::textC(label, x + w * 0.5f, y + h * 0.5f - 4, sel ? (u32)C_GOLD : col);
}
void header(const char* t, u32 col = C_GOLD) { gfx::rect(0, 0, BOT_W, 20, C_PANEL); gfx::textC(t, BOT_W / 2, 6, col); }

// ---------------------------------------------------------------------------------------------- menu
void updMenu(const Input& in) {
  const int n = 5;
  if (pressed(in, plat::BTN_DOWN)) g.menuSel = (g.menuSel + 1) % n;
  if (pressed(in, plat::BTN_UP)) g.menuSel = (g.menuSel + n - 1) % n;
  int act = -1;
  for (int i = 0; i < n; ++i) if (inR(in, 40, 30 + i * 40, 240, 34)) { g.menuSel = i; act = i; }
  if (pressed(in, plat::BTN_A)) act = g.menuSel;
  if (act < 0) return;
  audio::sfx(audio::SFX_UI);
  switch (act) {
    case 0: g.endless = false; g.allSeasons = false; g.level = 0; g.back = SC_MENU; go(SC_CAMPAIGN); break;
    case 1: g.endless = true; g.allSeasons = false; g.back = SC_MENU; go(SC_CAMPAIGN); break;
    case 2: g.catSel = 0; g.catScroll = 0; go(SC_CATALOG); break;
    case 3: g.custRow = 0; go(SC_CUSTOM); break;
    case 4: g.optSel = 0; go(SC_OPTIONS); break;
  }
}
void drawMenuTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  for (int i = 0; i < 5; ++i) gfx::discC(i % 8, 60.0f + i * 70, 190, 30.0f + 6 * (i & 1), 70);
  gfx::textC("ABYSSAL", 200, 26, C_GOLD, 4);
  gfx::textC("ECLIPSE", 200, 66, C_ECLIPSE, 4);
  gfx::textC(T(S_tagline), 200, 112, C_DIM);
  gfx::sprC(gfx::knight(), 200, 160, 4.0f);
  gfx::textC(T(S_version), 200, 224, C_BORDER);
}
void drawMenuBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  const StrId names[5] = {S_nav_campaign, S_nav_endless, S_nav_catalog, S_nav_custom, S_nav_options};
  for (int i = 0; i < 5; ++i) button(40, 30 + i * 40, 240, 34, T(names[i]), g.menuSel == i);
  gfx::textC(T(S_hint_dpad), 160, 222, C_DIM);
}

// ---------------------------------------------------------------------------------------------- campaign / endless
void updCampaign(const Input& in) {
  const int n = g.endless ? 1 : 3;
  if (pressed(in, plat::BTN_L) || pressed(in, plat::BTN_LEFT) || inR(in, 0, 0, 60, 30)) { g.season = (g.season + 4) % 5; g.level = 0; audio::sfx(audio::SFX_UI); audio::musicSet(g.season, 0.3f); }
  if (pressed(in, plat::BTN_R) || pressed(in, plat::BTN_RIGHT) || inR(in, 260, 0, 60, 30)) { g.season = (g.season + 1) % 5; g.level = 0; audio::sfx(audio::SFX_UI); audio::musicSet(g.season, 0.3f); }
  if (!g.endless) {
    if (pressed(in, plat::BTN_DOWN)) g.level = (g.level + 1) % n;
    if (pressed(in, plat::BTN_UP)) g.level = (g.level + n - 1) % n;
  }
  bool go_ = pressed(in, plat::BTN_A);
  if (!g.endless) for (int i = 0; i < 3; ++i) if (inR(in, 20, 50 + i * 56, 280, 50)) { g.level = i; go_ = true; }
  if (g.endless && inR(in, 20, 80, 280, 60)) go_ = true;
  if (pressed(in, plat::BTN_B)) { go(SC_MENU); return; }
  if (!g.endless && g.season == SEA_ECLIPSE && (g.save.levelCleared[SEA_ECLIPSE] & 4) && pressed(in, plat::BTN_Y)) {
    g.allSeasons = true; g.level = 2; g.weaponSel = g.save.lastWeapon;
    audio::sfx(audio::SFX_UI); go(SC_WEAPON); return;
  }
  if (go_) {
    bool ok = g.endless ? seasonUnlocked(g.save, g.season) : levelUnlocked(g.save, g.season, g.level);
    if (!ok) { toast(T(g.endless || g.level == 0 ? S_sea_locked_hint : S_lvl_locked_hint)); audio::sfx(audio::SFX_HURT); return; }
    audio::sfx(audio::SFX_UI); g.allSeasons = false; g.weaponSel = g.save.lastWeapon; go(SC_WEAPON);
  }
}
void drawCampaignTop() {
  const u32 c = SEASON_COLOR[g.season];
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  gfx::panel(20, 14, 360, 212, C_PANEL, c);
  gfx::spr(gfx::icon(seasonIcon(g.season)), 40, 34, 5.0f);
  gfx::text(T(SEASON_NAME[g.season]), 140, 40, c, 3);
  gfx::text(T(S_elements), 140, 76, C_DIM);
  for (int k = 0; k < 2; ++k) {
    int e = SEASON_ELEM[g.season][k];
    gfx::rect(140 + k * 110, 90, 10, 10, ELEM_COLOR[e]);
    gfx::text(T(ELEM_NAME[e]), 154 + k * 110, 91, C_TEXT);
  }
  gfx::textWrap(T(gimmick(g.season)), 36, 134, 330, C_TEXT, 1, 4);
  char b[64];
  if (g.endless) { snprintf(b, sizeof b, "%s: %d", T(S_best), g.save.bestWave[g.season]); gfx::text(b, 36, 180, C_GOLD, 2); }
  else { int done = 0; for (int l = 0; l < 3; ++l) if (g.save.levelCleared[g.season] & (1 << l)) ++done; snprintf(b, sizeof b, "%s %d/3", T(S_cleared), done); gfx::text(b, 36, 180, C_GOLD, 2); }
  if (!seasonUnlocked(g.save, g.season)) { gfx::rect(20, 14, 360, 212, C_VOID, 190); gfx::sprC(gfx::icon(IC_LOCK), 200, 100, 4.0f); gfx::textC(T(S_season_locked), 200, 150, C_RED, 2); }
}
void drawCampaignBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  gfx::rect(0, 0, BOT_W, 30, C_PANEL);
  gfx::text("<", 20, 10, C_GOLD, 2); gfx::text(">", 288, 10, C_GOLD, 2);
  gfx::textC(T(SEASON_NAME[g.season]), 160, 11, SEASON_COLOR[g.season], 1);
  char b[64];
  if (g.endless) {
    button(20, 80, 280, 60, T(S_play_endless), true);
    snprintf(b, sizeof b, "%s: %d", T(S_best), g.save.bestWave[g.season]); gfx::textC(b, 160, 160, C_GOLD);
  } else {
    for (int i = 0; i < 3; ++i) {
      bool un = levelUnlocked(g.save, g.season, i), sel = g.level == i, done = g.save.levelCleared[g.season] & (1 << i);
      gfx::panel(20, 50 + i * 56, 280, 50, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : (u32)C_BORDER);
      snprintf(b, sizeof b, "%d-%d", g.season + 1, i + 1);
      gfx::text(b, 30, 58 + i * 56, un ? (u32)C_GOLD : (u32)C_DIM, 2);
      gfx::text(T(LEVEL_NAME[i]), 76, 58 + i * 56, un ? (u32)C_TEXT : (u32)C_DIM);
      snprintf(b, sizeof b, "%s", Tn(S_waves_n, LEVELS[i].waves)); gfx::text(b, 76, 72 + i * 56, C_DIM);
      if (g.season != 0) gfx::text(T(S_ph_placeholder), 76, 84 + i * 56, C_BORDER);
      if (!un) gfx::sprC(gfx::icon(IC_LOCK), 280, 75 + i * 56, 1.5f);
      else if (done) gfx::sprC(gfx::icon(IC_TROPHY), 280, 75 + i * 56, 1.5f);
    }
  }
  if (!g.endless && g.season == SEA_ECLIPSE && (g.save.levelCleared[SEA_ECLIPSE] & 4)) gfx::textC(T(S_all_seasons), 160, 222, C_GOLD);
  else gfx::textC(T(S_btn_back), 160, 222, C_DIM);
}

// ---------------------------------------------------------------------------------------------- weapon select
void updWeapon(const Input& in) {
  if (pressed(in, plat::BTN_LEFT) || pressed(in, plat::BTN_L)) g.weaponSel = (g.weaponSel + 3) % 4;
  if (pressed(in, plat::BTN_RIGHT) || pressed(in, plat::BTN_R)) g.weaponSel = (g.weaponSel + 1) % 4;
  for (int i = 0; i < 4; ++i) if (inR(in, 8 + i * 77, 8, 72, 70)) { g.weaponSel = i; audio::sfx(audio::SFX_UI); }
  if (pressed(in, plat::BTN_B)) { go(SC_CAMPAIGN); return; }
  if (pressed(in, plat::BTN_A) || inR(in, 60, 192, 200, 36)) { audio::sfx(audio::SFX_WAVE); startRun(); }
}
void drawWeaponTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  gfx::textC(T(S_weapon_title), 200, 14, C_GOLD, 2);
  gfx::sprC(gfx::knight(), 130, 110, 6.0f);
  gfx::sprC(gfx::icon(IC_DAGGER + g.weaponSel), 250, 110, 6.0f);
  gfx::textC(T(WEAPON_NAME[g.weaponSel]), 250, 168, C_TEXT, 2);
  gfx::textC(T(S_first_spell), 200, 210, C_DIM);
}
void drawWeaponBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  for (int i = 0; i < 4; ++i) {
    bool sel = i == g.weaponSel;
    gfx::panel(8 + i * 77, 8, 72, 70, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : (u32)C_BORDER);
    gfx::sprC(gfx::icon(IC_DAGGER + i), 44 + i * 77, 32, 2.0f);
    gfx::textC(T(WEAPON_NAME[i]), 44 + i * 77, 60, sel ? (u32)C_GOLD : (u32)C_TEXT);
  }
  static const StrId dmgW[4] = {S_lvm_low, S_lvm_normal, S_lvm_high, S_lvm_very_high};
  static const StrId atkW[4] = {S_lvf_very_high, S_lvf_normal, S_lvf_low, S_lvf_very_low};
  static const StrId rngW[4] = {S_lvm_low, S_lvm_very_high, S_lvm_normal, S_lvm_high};
  const int w = g.weaponSel;
  gfx::panel(8, 88, 304, 96, C_PANEL, C_BORDER);
  gfx::text(T(S_dmg), 18, 98, C_DIM); gfx::textR(T(dmgW[w]), 300, 98, C_TEXT);
  gfx::text(T(S_atk_speed), 18, 112, C_DIM); gfx::textR(T(atkW[w]), 300, 112, C_TEXT);
  gfx::text(T(S_range), 18, 126, C_DIM); gfx::textR(T(rngW[w]), 300, 126, C_TEXT);
  gfx::textWrap(T(WEAPON_FIRE[w]), 18, 146, 280, C_GOLD, 1, 4);
  button(60, 192, 200, 36, T(S_start), true);
}

// ---------------------------------------------------------------------------------------------- customize
void updCustom(const Input& in) {
  if (pressed(in, plat::BTN_DOWN)) g.custRow = (g.custRow + 1) % 3;
  if (pressed(in, plat::BTN_UP)) g.custRow = (g.custRow + 2) % 3;
  int d = 0;
  if (pressed(in, plat::BTN_RIGHT) || pressed(in, plat::BTN_A)) d = 1;
  if (pressed(in, plat::BTN_LEFT)) d = -1;
  for (int i = 0; i < 3; ++i) {
    if (inR(in, 8, 30 + i * 60, 90, 50)) { g.custRow = i; d = -1; }
    if (inR(in, 222, 30 + i * 60, 90, 50)) { g.custRow = i; d = 1; }
  }
  if (d) {
    u8* v[3] = {&g.save.hat, &g.save.face, &g.save.color};
    const int mod[3] = {5, 5, 6};
    *v[g.custRow] = (u8)((*v[g.custRow] + mod[g.custRow] + d) % mod[g.custRow]);
    gfx::setKnight(cosm()); audio::sfx(audio::SFX_UI);
  }
  if (pressed(in, plat::BTN_B) || inR(in, 100, 214, 120, 24)) { persist(); go(SC_MENU); }
}
void drawCustomTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  gfx::textC(T(S_cosmetic), 200, 12, C_GOLD, 2);
  gfx::discC(2, 200, 130, 70, 60);
  gfx::sprC(gfx::knight(), 200, 126, 8.0f);
}
void drawCustomBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  const StrId lab[3] = {S_hat, S_face, S_color};
  const StrId val[3] = {HAT_NAME[g.save.hat], FACE_NAME[g.save.face], COLOR_NAME[g.save.color]};
  for (int i = 0; i < 3; ++i) {
    bool sel = g.custRow == i;
    gfx::panel(8, 30 + i * 60, 304, 50, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : (u32)C_BORDER);
    gfx::text(T(lab[i]), 20, 36 + i * 60, C_DIM);
    gfx::textC(T(val[i]), 160, 56 + i * 60, C_TEXT, 1);
    gfx::text("<", 20, 56 + i * 60, C_GOLD, 2); gfx::text(">", 286, 56 + i * 60, C_GOLD, 2);
  }
  button(100, 214, 120, 24, T(S_back), false);
}

// ---------------------------------------------------------------------------------------------- catalog
enum { CAT_COLS = 12, CAT_ROWS = 6, CAT_CELL = 24 };
int catCount() { return g.catTab == 0 ? NUM_SPELLS : NUM_ENEMIES; }
void updCatalog(const Input& in) {
  const int n = catCount();
  int old = g.catSel;
  if (pressed(in, plat::BTN_RIGHT)) g.catSel = minT(n - 1, g.catSel + 1);
  if (pressed(in, plat::BTN_LEFT)) g.catSel = maxT(0, g.catSel - 1);
  if (pressed(in, plat::BTN_DOWN)) g.catSel = minT(n - 1, g.catSel + CAT_COLS);
  if (pressed(in, plat::BTN_UP)) g.catSel = maxT(0, g.catSel - CAT_COLS);
  if (pressed(in, plat::BTN_L) || pressed(in, plat::BTN_R) || inR(in, 0, 0, 320, 20)) { g.catTab ^= 1; g.catSel = 0; g.catScroll = 0; audio::sfx(audio::SFX_UI); }
  for (int i = 0; i < n; ++i) {
    int r = i / CAT_COLS - g.catScroll;
    if (r >= 0 && r < CAT_ROWS && inR(in, 16 + (i % CAT_COLS) * CAT_CELL, 24 + r * CAT_CELL, CAT_CELL, CAT_CELL)) g.catSel = i;
  }
  if (pressed(in, plat::BTN_X)) g.loreScroll++;
  if (pressed(in, plat::BTN_Y)) g.loreScroll = maxT(0, g.loreScroll - 1);
  if (g.catSel != old) g.loreScroll = 0;
  int row = g.catSel / CAT_COLS;
  if (row < g.catScroll) g.catScroll = row;
  if (row >= g.catScroll + CAT_ROWS) g.catScroll = row - CAT_ROWS + 1;
    if (pressed(in, plat::BTN_B)) { g.secretCatalog = false; g.droneClicks = 0; go(SC_MENU); }
}
void drawCatalogTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  const int i = g.catSel;
  char b[96];
  if (g.catTab == 0) {
    const SpellDef& d = SPELLS[i];
    if (!spellFound(i)) { gfx::sprC(gfx::icon(IC_LOCK), 200, 90, 4.0f); gfx::textC(T(S_not_found), 200, 140, C_DIM, 2); return; }
    gfx::panel(8, 8, 384, 224, C_PANEL, RARITY_COLOR[d.rar]);
    gfx::spr(gfx::spellIcon(d.ty, d.el, d.act), 20, 20, 3.0f);
    gfx::text(spellName(i), 76, 20, RARITY_COLOR[d.rar], 2);
    snprintf(b, sizeof b, "%s / %s / %s / %s", T(RARITY_NAME[d.rar]), T(ELEM_NAME[d.el]), T(TYPE_NAME[d.ty]), T(ACT_NAME[d.act]));
    gfx::text(b, 76, 42, C_DIM);
    gfx::textWrap(spellDesc(i), 20, 76, 360, C_TEXT, 1, 3);
    gfx::textWrap(spellStats(i), 20, 106, 360, C_GOLD, 1, 2);
    snprintf(b, sizeof b, "%s %d", T(S_found), g.save.spellFound[i]); gfx::text(b, 20, 214, C_DIM);
    gfx::text(T(S_lore), 20, 128, C_EPIC);
    gfx::textWrap(spellLore(i), 20, 140, 360, C_DIM, 1, 7, g.loreScroll);
  } else {
    const EnemyDef& d = ENEMIES[i];
    if (!enemyFound(i)) { gfx::sprC(gfx::icon(IC_LOCK), 200, 90, 4.0f); gfx::textC(T(S_not_found), 200, 140, C_DIM, 2); return; }
    gfx::panel(8, 8, 384, 224, C_PANEL, SEASON_COLOR[d.season]);
    gfx::spr(gfx::enemy(i), 20, 20, 3.0f);
    gfx::text(enemyName(i), 76, 20, SEASON_COLOR[d.season], 2);
    snprintf(b, sizeof b, "%s %d  %s %s  %s %s", T(S_hp), d.hp, T(S_spd), T(speedWord(d.spd)), T(S_range), T(rangeWord(d.range)));
    gfx::text(b, 76, 42, C_DIM);
    gfx::textWrap(enemyDesc(i), 20, 76, 360, C_TEXT, 1, 3);
    snprintf(b, sizeof b, "%s: %s", T(S_special), enemySpecial(i)); gfx::textWrap(b, 20, 106, 360, C_GOLD, 1, 2);
    gfx::text(T(S_lore), 20, 128, C_EPIC);
    gfx::textWrap(enemyLore(i), 20, 140, 360, C_DIM, 1, 7, g.loreScroll);
    snprintf(b, sizeof b, "%s %d", T(S_kills_s), g.save.enemyKills[i]); gfx::text(b, 20, 214, C_DIM);
  }
}
void drawCatalogBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  gfx::rect(0, 0, BOT_W, 20, C_PANEL);
  gfx::textC(T(g.catTab == 0 ? S_sub_spells : S_sub_bestiary), 160, 6, C_GOLD);
  const int n = catCount();
  for (int i = g.catScroll * CAT_COLS; i < n && i < (g.catScroll + CAT_ROWS) * CAT_COLS; ++i) {
    int r = i / CAT_COLS - g.catScroll, c = i % CAT_COLS;
    float x = 16.0f + c * CAT_CELL, y = 24.0f + r * CAT_CELL;
    if (i == g.catSel) gfx::rect(x - 1, y - 1, CAT_CELL - 2, CAT_CELL - 2, C_GOLD);
    if (g.catTab == 0) {
      if (spellFound(i)) gfx::spr(gfx::spellIcon(SPELLS[i].ty, SPELLS[i].el, SPELLS[i].act), x + 2, y + 2, 1.0f);
      else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT);
    } else {
      if (enemyFound(i)) gfx::spr(gfx::enemy(i), x + 2, y + 2, 1.0f);
      else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT);
    }
  }
  int found = 0;
  for (int i = 0; i < n; ++i) found += (g.catTab == 0 ? spellFound(i) : enemyFound(i)) ? 1 : 0;
  char b[48]; snprintf(b, sizeof b, "%d/%d", found, n);
  gfx::textC(b, 160, 176, C_DIM);
  gfx::textC("L/R: TAB   X/Y: LORE   B: BACK", 160, 222, C_DIM);
}

// ---------------------------------------------------------------------------------------------- options / credits
void updOptions(const Input& in) {
  const int n = 7;
  if (pressed(in, plat::BTN_DOWN)) g.optSel = (g.optSel + 1) % n;
  if (pressed(in, plat::BTN_UP)) g.optSel = (g.optSel + n - 1) % n;
  int d = 0; bool act = pressed(in, plat::BTN_A);
  if (pressed(in, plat::BTN_RIGHT)) d = 1;
  if (pressed(in, plat::BTN_LEFT)) d = -1;
  for (int i = 0; i < n; ++i) if (inR(in, 20, 20 + i * 28, 280, 26)) { g.optSel = i; act = true; d = (in.tx < 110 ? -1 : (in.tx > 210 ? 1 : 0)); }
  if (pressed(in, plat::BTN_B)) { persist(); go(SC_MENU); return; }
  switch (g.optSel) {
    case 0: if (act || d) { g.save.lang = (u8)((g.save.lang + 1) % LANG_COUNT); applyOptions(); audio::sfx(audio::SFX_UI); } break;
    case 1: if (d || act) { int v = g.save.musicVol + (d ? d * 10 : 10); if (v > 100) v = 0; g.save.musicVol = (u8)clampT(v, 0, 100); applyOptions(); } break;
    case 2: if (d || act) { int v = g.save.sfxVol + (d ? d * 10 : 10); if (v > 100) v = 0; g.save.sfxVol = (u8)clampT(v, 0, 100); applyOptions(); audio::sfx(audio::SFX_PICKUP); } break;
    case 3: if (act || d) { g.save.fps30 ^= 1; applyOptions(); } break;
    case 4: if (act || d) { g.save.showFps ^= 1; } break;
    case 5: if (act) go(SC_CREDITS); break;
    case 6: if (act) { persist(); go(SC_MENU); } break;
  }
}
void drawOptionsTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  gfx::textC(T(S_nav_options), 200, 40, C_GOLD, 3);
  gfx::textC(T(S_changes_instant), 200, 100, C_DIM);
  gfx::textC(T(S_lang_selected), 200, 120, C_TEXT);
  if (g.silent) gfx::textC(T(S_no_sound), 200, 160, C_RED);
}
void drawOptionsBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  char b[64];
  const char* labels[7];
  labels[0] = g.save.lang == LANG_EN ? T(S_lang_en) : T(S_lang_pt);
  snprintf(b, sizeof b, "%s: %d%%", T(S_volume_music), g.save.musicVol); static char b1[64]; snprintf(b1, sizeof b1, "%s", b);
  snprintf(b, sizeof b, "%s: %d%%", T(S_volume_sfx), g.save.sfxVol); static char b2[64]; snprintf(b2, sizeof b2, "%s", b);
  snprintf(b, sizeof b, "%s: %s", T(S_opt_fps), g.save.fps30 ? T(S_on) : T(S_off)); static char b3[64]; snprintf(b3, sizeof b3, "%s", b);
  labels[1] = b1; labels[2] = b2; labels[3] = b3;
  snprintf(b, sizeof b, "%s: %s", T(S_opt_showfps), g.save.showFps ? T(S_on) : T(S_off)); static char b4[64]; snprintf(b4, sizeof b4, "%s", b);
  labels[4] = b4; labels[5] = T(S_credits); labels[6] = T(S_back_menu);
  for (int i = 0; i < 7; ++i) button(20, 20 + i * 28, 280, 26, labels[i], g.optSel == i);
  gfx::textC(T(S_btn_back), 160, 222, C_DIM);
}
void drawCreditsTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  gfx::textC(T(S_credits), 200, 24, C_GOLD, 3);
  const StrId cr[4] = {S_cr_1, S_cr_2, S_cr_3, S_cr_4};
  for (int i = 0; i < 4; ++i) gfx::textWrap(T(cr[i]), 24, 70 + i * 28, 352, C_TEXT, 1, 3);
  gfx::textC(T(S_thanks), 200, 200, C_ECLIPSE);
}

// ---------------------------------------------------------------------------------------------- in-game bottom panel
static const int STAT_ICON[5] = {IC_SKULL, IC_SHIELD, IC_BOLT, IC_CLOVER, IC_STAR};
static const StrId STAT_NAME[5] = {S_st_str, S_st_res, S_st_spd, S_st_luck, S_st_int};
static const StrId STAT_DESC[5] = {S_st_str_e, S_st_res_e, S_st_spd_e, S_st_luck_e, S_st_int_e};

void slotText(const Sim& s, int i, const char** name, const char** desc, const char** stats, u32* col, const gfx::Spr** icon) {
  const SpellSlot& sl = s.slots[i];
  static gfx::Spr dummy;
  if (sl.id < 0) { *name = T(WEAPON_NAME[-1 - sl.id]); *desc = T(WEAPON_FIRE[-1 - sl.id]); *stats = T(S_first_spell); *col = C_GOLD; *icon = &gfx::icon(IC_DAGGER + (-1 - sl.id)); }
  else { *name = spellName(sl.id); *desc = spellDesc(sl.id); *stats = spellStats(sl.id); *col = RARITY_COLOR[SPELLS[sl.id].rar]; *icon = &gfx::spellIcon(SPELLS[sl.id].ty, SPELLS[sl.id].el, SPELLS[sl.id].act); }
  (void)dummy;
}

void drawOfferCard(const Sim& s, int i, float y, bool sel) {
  const int id = s.offers[i];
  const bool up = s.offersAreUpgrades;
  const char *name, *desc, *stats; u32 col; const gfx::Spr* ic;
  if (up) slotText(s, id, &name, &desc, &stats, &col, &ic);
  else { name = spellName(id); desc = spellDesc(id); stats = spellStats(id); col = RARITY_COLOR[SPELLS[id].rar]; ic = &gfx::spellIcon(SPELLS[id].ty, SPELLS[id].el, SPELLS[id].act); }
  gfx::panel(8, y, 304, 56, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : col);
  gfx::spr(*ic, 14, y + 12, 2.0f);
  gfx::text(name, 54, y + 5, col);
  if (!up && simSpellLevelOf(s, id) == 0 && g.save.spellFound[id] == 0) gfx::textR("NEW!", 304, y + 5, C_GREEN);
  if (up) { char b[24]; snprintf(b, sizeof b, "%s", Tn(S_lvl_n, s.slots[id].lv + 1)); gfx::textR(b, 304, y + 5, C_GOLD); }
  gfx::textWrap(desc, 54, y + 17, 250, C_TEXT, 1, 2);
  gfx::textWrap(stats, 54, y + 41, 250, C_GOLD, 1, 1);
}

void drawGameBot() {
  Sim& s = g_sim;
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  char b[96];
  if (g.debug) {
    header(T(S_dbg_title), C_RED);
    const StrId lab[9] = {S_dbg_wave, S_dbg_boss, S_dbg_force, S_dbg_clear, S_dbg_death, S_dbg_xp, S_dbg_god, S_dbg_unlock, S_dbg_reset};
    for (int i = 0; i < 9; ++i) {
      snprintf(b, sizeof b, "%s%s", T(lab[i]), (i == 6 && s.godMode) ? " [X]" : ((i == 7 && g.save.unlockAll) ? " [X]" : ""));
      button(20, 24 + i * 23, 280, 21, b, g.dbgSel == i);
    }
    return;
  }
  if (g.paused) {
    header(T(S_paused));
    button(40, 60, 240, 40, T(S_resume), g.pauseSel == 0);
    button(40, 120, 240, 40, T(S_quit_menu), g.pauseSel == 1);
    return;
  }
  if (s.phase == PH_ALLOC) {
    snprintf(b, sizeof b, "%s  (%d %s)", T(S_spend), s.points, T(S_pts)); header(b);
    for (int i = 0; i < ST_COUNT; ++i) {
      float y = 26 + i * 30; bool sel = g.allocSel == i;
      gfx::panel(8, y, 304, 28, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : (u32)C_BORDER);
      gfx::spr(gfx::icon(STAT_ICON[i]), 12, y + 6, 1.0f);
      snprintf(b, sizeof b, "%s %d", T(STAT_NAME[i]), statValue(s, i)); gfx::text(b, 34, y + 4, C_TEXT);
      gfx::text(T(STAT_DESC[i]), 34, y + 16, C_DIM);
      if (s.points > 0) { gfx::panel(272, y + 2, 34, 24, C_GREEN, C_BORDER); gfx::textC("+", 289, y + 8, C_VOID, 1); }
    }
    button(60, 184, 200, 32, T(S_continue), g.allocSel == ST_COUNT);
    gfx::textC(T(S_bank), 160, 222, C_DIM);
    return;
  }
  if (s.phase == PH_SPELL) {
    header(T(s.offersAreUpgrades ? S_upgrade_t : S_choose_spell));
    for (int i = 0; i < s.nOffers; ++i) drawOfferCard(s, i, 24.0f + i * 60, g.offerSel == i);
    gfx::textC(T(S_choose_one), 160, 226, C_DIM);
    return;
  }
  if (s.phase == PH_DEAD || s.phase == PH_VICTORY) {
    header(T(s.phase == PH_DEAD ? S_run_over : S_victory), s.phase == PH_DEAD ? (u32)C_RED : (u32)C_GREEN);
    int secs = (int)s.runT;
    snprintf(b, sizeof b, "%s: %d", T(S_wave), s.wave); gfx::text(b, 30, 40, C_TEXT, 2);
    snprintf(b, sizeof b, "%s: %d", T(S_enemies_killed), s.totalKills); gfx::text(b, 30, 70, C_TEXT);
    snprintf(b, sizeof b, "%s: %d:%02d", T(S_time), secs / 60, secs % 60); gfx::text(b, 30, 86, C_TEXT);
    snprintf(b, sizeof b, "%s %d", T(S_plevel), s.p.level); gfx::text(b, 30, 102, C_TEXT);
    snprintf(b, sizeof b, "%s: %d", T(S_spells_found), s.nSlots - 1); gfx::text(b, 30, 118, C_TEXT);
    button(60, 170, 200, 40, T(S_retry), true);
    return;
  }
  // PLAY: tabs
  const StrId tabs[3] = {S_tab_spells, S_tab_char, S_tab_run};
  for (int i = 0; i < 3; ++i) {
    gfx::panel(2 + i * 106, 2, 104, 20, g.tab == i ? (u32)0x3A2F5C : (u32)C_PANEL, g.tab == i ? (u32)C_GOLD : (u32)C_BORDER);
    gfx::textC(T(tabs[i]), 54 + i * 106, 8, g.tab == i ? (u32)C_GOLD : (u32)C_DIM);
  }
  if (g.tab == 0) {
    for (int i = 0; i < bal::MAX_SLOTS; ++i) {
      float x = 8.0f + (i % 4) * 76, y = 26.0f + (i / 4) * 44;
      bool sel = g.slotSel == i, has = i < s.nSlots;
      gfx::panel(x, y, 72, 40, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : (u32)C_BORDER);
      if (has) {
        const char *nm, *ds, *st; u32 col; const gfx::Spr* ic;
        slotText(s, i, &nm, &ds, &st, &col, &ic);
        gfx::spr(*ic, x + 4, y + 8, 1.5f);
        if (s.slots[i].id >= 0) { snprintf(b, sizeof b, "%s%d", T(S_plevel), s.slots[i].lv); gfx::text(b, x + 34, y + 26, C_GOLD); }
      } else gfx::textC("-", x + 36, y + 16, C_BORDER);
    }
    if (g.slotSel < s.nSlots) {
      const char *nm, *ds, *st; u32 col; const gfx::Spr* ic;
      slotText(s, g.slotSel, &nm, &ds, &st, &col, &ic);
      gfx::text(nm, 8, 160, col);
      gfx::textWrap(ds, 8, 172, 304, C_TEXT, 1, 3);
      gfx::textWrap(st, 8, 202, 304, C_GOLD, 1, 2);
    } else gfx::textC(T(S_empty_slot), 160, 180, C_DIM);
    snprintf(b, sizeof b, "%s", Tn(S_slots, s.nSlots)); gfx::textR(b, 312, 226, C_DIM);
  } else if (g.tab == 1) {
    for (int i = 0; i < ST_COUNT; ++i) {
      float y = 28.0f + i * 32;
      gfx::panel(8, y, 304, 30, C_PANEL, C_BORDER);
      gfx::spr(gfx::icon(STAT_ICON[i]), 12, y + 7);
      snprintf(b, sizeof b, "%s %d", T(STAT_NAME[i]), statValue(s, i)); gfx::text(b, 34, y + 5, C_TEXT);
      gfx::text(T(STAT_DESC[i]), 34, y + 17, C_DIM);
    }
    snprintf(b, sizeof b, "%s %d   %s %d", T(S_plevel), s.p.level, T(S_pts), s.points); gfx::text(b, 8, 192, C_GOLD);
    snprintf(b, sizeof b, "%s %d/%d", T(S_hp), (int)s.p.hp, (int)s.p.maxhp); gfx::text(b, 8, 206, C_RED);
  } else {
    snprintf(b, sizeof b, "%s: %s", T(S_season), T(SEASON_NAME[s.season])); gfx::text(b, 12, 34, SEASON_COLOR[s.season]);
    if (s.endless) snprintf(b, sizeof b, "%s: %s", T(S_level), T(S_nav_endless)); else snprintf(b, sizeof b, "%s: %d-%d", T(S_level), s.season + 1, s.levelIdx + 1);
    gfx::text(b, 12, 50, C_TEXT);
    snprintf(b, sizeof b, "%s: %d", T(S_wave), s.wave); gfx::text(b, 12, 66, C_TEXT);
    snprintf(b, sizeof b, "%s: %d", T(S_enemies_killed), s.totalKills); gfx::text(b, 12, 82, C_TEXT);
    int secs = (int)s.runT; snprintf(b, sizeof b, "%s: %d:%02d", T(S_time), secs / 60, secs % 60); gfx::text(b, 12, 98, C_TEXT);
    snprintf(b, sizeof b, "%s: %d", T(S_best_wave), g.save.bestWave[s.season]); gfx::text(b, 12, 114, C_GOLD);
    gfx::textC(T(S_sel_dbg), 160, 226, C_BORDER);
  }
}

void drawGameTop() {
  simDraw(g_sim, g.save.showFps != 0, g.fps);
  Sim& s = g_sim;
  if (g.paused) { gfx::rect(0, 0, TOP_W, TOP_H, C_VOID, 150); gfx::textC(T(S_paused), 200, 110, C_GOLD, 3); }
  if (s.phase == PH_DEAD) { gfx::rect(0, 90, TOP_W, 60, C_VOID, 200); gfx::textC(T(S_died), 200, 112, C_RED, 3); }
  if (s.phase == PH_VICTORY) { gfx::rect(0, 90, TOP_W, 60, C_VOID, 200); gfx::textC(T(S_victory), 200, 112, C_GREEN, 3); }
  if (g.toastT > 0) gfx::textC(g.toast, 200, 224, C_RED);
}

// ---------------------------------------------------------------------------------------------- in-game update
void runDebug(int i) {
  Sim& s = g_sim;
  switch (i) {
    case 0: dbgWaveEnd(s); g.debug = false; break;
    case 1: dbgBoss(s); g.debug = false; break;
    case 2: dbgForceSpell(s); g.debug = false; break;
    case 3: dbgClearSpells(s); break;
    case 4: dbgDeath(s); g.debug = false; break;
    case 5: dbgGiveLevel(s); break;
    case 6: s.godMode = !s.godMode; break;
    case 7: g.save.unlockAll = !g.save.unlockAll; break;
    case 8: startRun(); break;
  }
}

void updGame(const Input& in, float dt) {
  Sim& s = g_sim;
  if (pressed(in, plat::BTN_SELECT)) { g.debug = !g.debug; g.paused = false; }
  if (g.debug) {
    if (pressed(in, plat::BTN_DOWN)) g.dbgSel = (g.dbgSel + 1) % 9;
    if (pressed(in, plat::BTN_UP)) g.dbgSel = (g.dbgSel + 8) % 9;
    for (int i = 0; i < 9; ++i) if (inR(in, 20, 24 + i * 23, 280, 21)) { g.dbgSel = i; runDebug(i); return; }
    if (pressed(in, plat::BTN_A)) runDebug(g.dbgSel);
    if (pressed(in, plat::BTN_B)) g.debug = false;
    return;
  }
  if (pressed(in, plat::BTN_START) && s.phase == PH_PLAY) { g.paused = !g.paused; g.pauseSel = 0; }
  if (g.paused) {
    if (pressed(in, plat::BTN_DOWN) || pressed(in, plat::BTN_UP)) g.pauseSel ^= 1;
    int act = -1;
    if (inR(in, 40, 60, 240, 40)) act = 0;
    if (inR(in, 40, 120, 240, 40)) act = 1;
    if (pressed(in, plat::BTN_A)) act = g.pauseSel;
    if (pressed(in, plat::BTN_B)) act = 0;
    if (act == 0) g.paused = false;
    if (act == 1) { g.paused = false; finishRun(false); }
    return;
  }
  switch (s.phase) {
    case PH_PLAY: {
      if (pressed(in, plat::BTN_L)) g.tab = (g.tab + 2) % 3;
      if (pressed(in, plat::BTN_R)) g.tab = (g.tab + 1) % 3;
      for (int i = 0; i < 3; ++i) if (inR(in, 2 + i * 106, 2, 104, 20)) g.tab = i;
      if (g.tab == 0) {
        for (int i = 0; i < bal::MAX_SLOTS; ++i) if (inR(in, 8 + (i % 4) * 76, 26 + (i / 4) * 44, 72, 40)) g.slotSel = i;
        if (pressed(in, plat::BTN_X)) g.slotSel = (g.slotSel + 1) % bal::MAX_SLOTS;
        if (pressed(in, plat::BTN_Y)) g.slotSel = (g.slotSel + bal::MAX_SLOTS - 1) % bal::MAX_SLOTS;
      }
      SimInput si;
      si.mx = ((in.held & plat::BTN_RIGHT) ? 1.0f : 0.0f) - ((in.held & plat::BTN_LEFT) ? 1.0f : 0.0f) + in.cx;
      si.my = ((in.held & plat::BTN_DOWN) ? 1.0f : 0.0f) - ((in.held & plat::BTN_UP) ? 1.0f : 0.0f) + in.cy;
      simUpdate(s, si, dt);
    } break;
    case PH_CLEARED: simUpdate(s, SimInput{0, 0}, dt); break;
    case PH_ALLOC: {
      if (pressed(in, plat::BTN_DOWN)) g.allocSel = (g.allocSel + 1) % (ST_COUNT + 1);
      if (pressed(in, plat::BTN_UP)) g.allocSel = (g.allocSel + ST_COUNT) % (ST_COUNT + 1);
      bool cont = pressed(in, plat::BTN_B) || inR(in, 60, 184, 200, 32);
      if (pressed(in, plat::BTN_A)) { if (g.allocSel == ST_COUNT) cont = true; else simSpend(s, g.allocSel); }
      for (int i = 0; i < ST_COUNT; ++i) if (inR(in, 8, 26 + i * 30, 304, 28)) { g.allocSel = i; if (in.tx > 268) simSpend(s, i); }
      if (cont) { simFinishAlloc(s); g.offerSel = 0; }
    } break;
    case PH_SPELL: {
      if (pressed(in, plat::BTN_DOWN) && s.nOffers) g.offerSel = (g.offerSel + 1) % s.nOffers;
      if (pressed(in, plat::BTN_UP) && s.nOffers) g.offerSel = (g.offerSel + s.nOffers - 1) % s.nOffers;
      int pick = pressed(in, plat::BTN_A) ? g.offerSel : -2;
      for (int i = 0; i < s.nOffers; ++i) if (inR(in, 8, 24 + i * 60, 304, 56)) { if (g.offerSel == i) pick = i; g.offerSel = i; }
      if (pick >= 0) { simPickOffer(s, pick); g.offerSel = 0; }
    } break;
    case PH_DEAD: case PH_VICTORY:
      simUpdate(s, SimInput{0, 0}, dt);
      if (s.phaseT > 0.8f && (pressed(in, plat::BTN_A) || inR(in, 60, 170, 200, 40))) finishRun(s.phase == PH_VICTORY);
      break;
  }
}
}  // namespace

// ================================================================================ public API
bool gameInit() {
  if (!plat::init()) return false;
  saveLoad(g.save);
  g.inited = true;
  applyOptions();
  audio::init();
  g.silent = !plat::audioInit(audio::mix);
  if (!gfx::init(cosm())) return false;
  g.weaponSel = g.save.lastWeapon;
  go(SC_MENU);
  return true;
}
void gameShutdown() { if (g.inited) persist(); plat::audioShutdown(); plat::shutdown(); }
int gameTargetFps() { return g.save.fps30 ? 30 : 60; }
Sim& gameSim() { return g_sim; }
void gameStartRunForTest(int season, int level, bool endless, int weapon, u32 seed) {
  g.season = season; g.level = level; g.endless = endless; g.allSeasons = false; g.weaponSel = weapon;
  simStart(g_sim, season, level, endless, weapon, seed, false); g.scr = SC_GAME;
}

bool gameFrame(const Input& in, float dt) {
  if (dt > 0.0f) {
    g.fpsElapsed += dt; ++g.fpsFrames;
    if (g.fpsElapsed >= 1.0f) {
      g.fps = (int)(g.fpsFrames / g.fpsElapsed + 0.5f);
      g.fpsFrames = 0; g.fpsElapsed = 0.0f;
    }
  }
  if (g.toastT > 0) g.toastT -= dt;
  switch (g.scr) {
    case SC_MENU: updMenu(in); break;
    case SC_CAMPAIGN: updCampaign(in); break;
    case SC_WEAPON: updWeapon(in); break;
    case SC_CUSTOM: updCustom(in); break;
    case SC_GAME: updGame(in, dt); break;
    case SC_CATALOG: updCatalog(in); break;
    case SC_OPTIONS: updOptions(in); break;
    case SC_CREDITS: {
      if (inR(in, 128, 150, 64, 60)) {
        g.droneClicks++;
        audio::sfx(audio::SFX_UI);
        if (g.droneClicks >= 3) {
          g.droneClicks = 0;
          g.secretCatalog = true;
          toast("SECRET UNLOCKED!");
          audio::sfx(audio::SFX_LEVEL);
        }
      } else if (pressed(in, plat::BTN_B) || pressed(in, plat::BTN_A) || in.touchDown) {
        g.droneClicks = 0;
        go(SC_OPTIONS);
      }
      break;
    }
  }
  return true;
}

void gameDraw() {
  plat::beginScreen(true);
  switch (g.scr) {
    case SC_MENU: drawMenuTop(); break;
    case SC_CAMPAIGN: drawCampaignTop(); break;
    case SC_WEAPON: drawWeaponTop(); break;
    case SC_CUSTOM: drawCustomTop(); break;
    case SC_GAME: drawGameTop(); break;
    case SC_CATALOG: drawCatalogTop(); break;
    case SC_OPTIONS: drawOptionsTop(); break;
    case SC_CREDITS: drawCreditsTop(); break;
  }
  if (g.toastT > 0 && g.scr != SC_GAME) gfx::textC(g.toast, 200, 224, C_RED);
  plat::beginScreen(false);
  switch (g.scr) {
    case SC_MENU: drawMenuBot(); break;
    case SC_CAMPAIGN: drawCampaignBot(); break;
    case SC_WEAPON: drawWeaponBot(); break;
    case SC_CUSTOM: drawCustomBot(); break;
    case SC_GAME: drawGameBot(); break;
    case SC_CATALOG: drawCatalogBot(); break;
    case SC_OPTIONS: drawOptionsBot(); break;
    case SC_CREDITS:
      gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
      gfx::textC(T(S_btn_back), 160, 100, C_DIM);
      gfx::sprC(gfx::drone(), 160, 180, 4.0f);       // the easter-egg drone
      if (g.secretCatalog) gfx::textC("SECRET UNLOCKED!", 160, 220, C_GOLD);
      break;
  }
}
