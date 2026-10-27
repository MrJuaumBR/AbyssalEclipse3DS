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
enum Screen { SC_MENU, SC_CAMPAIGN, SC_WEAPON, SC_CUSTOM, SC_GAME, SC_CATALOG, SC_ACHIEVEMENTS, SC_OPTIONS, SC_CREDITS };

struct G {
  Screen scr = SC_MENU, back = SC_MENU;
  SaveData save;
  bool endless = false, allSeasons = false;
  float fpsElapsed = 0.0f; unsigned int fpsFrames = 0; int fps = 0;
  int season = 0, level = 0, weaponSel = 0, menuSel = 0;
  int tab = 0, slotSel = 0, allocSel = 0, offerSel = 0;
  int catTab = 0, catSel = 0, catScroll = 0, achSel = 0, achScroll = 0, optSel = 0, custRow = 0, loreScroll = 0;
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
void addLifetime(u32& total, u32 amount) { const u32 maxValue = 0xFFFFFFFFu; total = amount > maxValue - total ? maxValue : total + amount; }
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
static const StrId CLOTH_NAME[11] = {S_cloth_default, S_cloth_wizard, S_cloth_leaf, S_cloth_crown, S_cloth_none, S_cloth_batman, S_cloth_joker, S_cloth_terraria, S_cloth_mario, S_cloth_ironman, S_cloth_avatar};
static const StrId MUSIC_NAME[5] = {S_music_forest, S_music_desert, S_music_snow, S_music_mountain, S_music_eclipse};
bool clothUnlocked(int id) {
  if (g.save.unlockAll || g.secretCatalog || id < 5) return true;
  static const int ach[5] = {ACH_CLOTH_BATMAN, ACH_CLOTH_JOKER, ACH_CLOTH_TERRARIA, ACH_CLOTH_MARIO, ACH_CLOTH_IRONMAN};
  if (id >= 5 && id < 10) return achievementUnlocked(g.save, ach[id - 5]);
  return id == 10 && achievementUnlocked(g.save, ACH_AVATAR);
}
bool faceUnlocked(int id) {
  return id >= 0 && id < 6;
}
bool colorUnlocked(int id) {
  return id >= 0 && id < 6;
}
bool musicUnlocked(int id) { return g.save.unlockAll || g.secretCatalog || (id >= 0 && id < 5 && achievementUnlocked(g.save, ACH_SEASON_FOREST + id)); }
bool allSeasonsComplete() { for (int i = 0; i < SEA_COUNT; ++i) if ((g.save.levelCleared[i] & 7) != 7) return false; return true; }
bool weaponUnlocked(int id) { return id != W_MACHINE_GUN || g.save.unlockAll || g.secretCatalog || allSeasonsComplete(); }

void startRun() {
  g.save.lastWeapon = (u8)g.weaponSel;
  addLifetime(g.save.gamesPlayed, 1);
  persist();
  simStart(g_sim, g.season, g.level, g.endless, g.weaponSel, (u32)plat::nowMs() | 1u, g.allSeasons);
  g.scr = SC_GAME; g.tab = 0; g.slotSel = 0; g.allocSel = 0; g.offerSel = 0; g.paused = false; g.debug = false;
  audio::musicSet(g.season, 0.4f);
}
void finishRun(bool victory) {
  Sim& s = g_sim;
  addLifetime(g.save.totalKills, (u32)maxT(0, s.totalKills));
  addLifetime(victory ? g.save.runsWon : g.save.runsLost, 1);
  addLifetime(g.save.spellsAcquired, s.runSpellsAcquired);
  addLifetime(g.save.spellsCasted, s.runSpellsCasted);
  addLifetime(g.save.spellUpgrades, s.runSpellUpgrades);
  addLifetime(g.save.weaponThrows, (u32)maxT(0, s.p.throws));
  addLifetime(g.save.playTimeSeconds, (u32)maxT(0.0f, s.runT));
  for (int i = 0; i < NUM_ENEMIES; ++i) {
    if (s.seenEnemy[i]) g.save.enemySeen[i] = 1;
    int k = (int)g.save.enemyKills[i] + s.runKills[i]; g.save.enemyKills[i] = (u16)minT(k, 65535);
  }
  for (int i = 0; i < NUM_SPELLS; ++i) {
    if (s.seenSpell[i]) g.save.spellFound[i] = (u16)minT((int)g.save.spellFound[i] + s.seenSpell[i], 65535);
    if (s.usedSpell[i]) g.save.spellUsed[i] = 1;
  }
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
int weaponIcon(int w) { static const int t[W_COUNT] = {IC_DAGGER, IC_BOW, IC_AXE, IC_STAFF, IC_SHOTGUN, IC_PISTOL, IC_TRIDENT, IC_KATANA, IC_MACHINE_GUN}; return t[clampT(w, 0, W_COUNT - 1)]; }

void button(float x, float y, float w, float h, const char* label, bool sel, u32 col = C_TEXT) {
  gfx::panel(x, y, w, h, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : (u32)C_BORDER);
  // Buttons stay at the compact bitmap scale; scale 2 is reserved for headings and titles.
  const int scale = 1;
  gfx::textC(label, x + w * 0.5f, y + (h - 10.0f) * 0.5f, sel ? (u32)C_GOLD : col, scale);
}
static int fitScale(const char* t, int maxW, int preferred = 2) { return preferred >= 2 && gfx::textWidth(t, preferred) <= maxW ? preferred : 1; }
static void textFitC(const char* t, float cx, float y, int maxW, u32 col, int preferred = 2) { gfx::textC(t, cx, y, col, fitScale(t, maxW, preferred)); }
static void textFit(const char* t, float x, float y, int maxW, u32 col, int preferred = 1) { gfx::text(t, x, y, col, gfx::textWidth(t, preferred) <= maxW ? preferred : 1); }
void header(const char* t, u32 col = C_GOLD) { gfx::rect(0, 0, BOT_W, 20, C_PANEL); textFitC(t, BOT_W / 2, 6, BOT_W - 8, col, 1); }

// ---------------------------------------------------------------------------------------------- menu
void updMenu(const Input& in) {
  const int n = 6;
  if (pressed(in, plat::BTN_DOWN)) g.menuSel = (g.menuSel + 1) % n;
  if (pressed(in, plat::BTN_UP)) g.menuSel = (g.menuSel + n - 1) % n;
  int act = -1;
  for (int i = 0; i < n; ++i) if (inR(in, 40, 18 + i * 32, 240, 28)) { g.menuSel = i; act = i; }
  if (pressed(in, plat::BTN_A)) act = g.menuSel;
  if (act < 0) return;
  audio::sfx(audio::SFX_UI);
  switch (act) {
    case 0: g.endless = false; g.allSeasons = false; g.level = 0; g.back = SC_MENU; go(SC_CAMPAIGN); break;
    case 1: g.endless = true; g.allSeasons = false; g.back = SC_MENU; go(SC_CAMPAIGN); break;
    case 2: g.catSel = 0; g.catScroll = 0; go(SC_CATALOG); break;
    case 3: g.achSel = 0; g.achScroll = 0; go(SC_ACHIEVEMENTS); break;
    case 4: g.custRow = 0; go(SC_CUSTOM); break;
    case 5: g.optSel = 0; go(SC_OPTIONS); break;
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
  const StrId names[6] = {S_nav_campaign, S_nav_endless, S_nav_catalog, S_nav_achievements, S_nav_custom, S_nav_options};
  for (int i = 0; i < 6; ++i) button(40, 18 + i * 32, 240, 28, T(names[i]), g.menuSel == i);
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
  textFit(T(SEASON_NAME[g.season]), 140, 40, 230, c, 3);
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
      gfx::text(T(LEVEL_NAME[g.season][i]), 76, 58 + i * 56, un ? (u32)C_TEXT : (u32)C_DIM);
      snprintf(b, sizeof b, "%s", Tn(S_waves_n, LEVELS[i].waves)); gfx::text(b, 76, 72 + i * 56, C_DIM);
      if (!un) gfx::sprC(gfx::icon(IC_LOCK), 280, 75 + i * 56, 1.5f);
      else if (done) gfx::sprC(gfx::icon(IC_TROPHY), 280, 75 + i * 56, 1.5f);
    }
  }
  if (!g.endless && g.season == SEA_ECLIPSE && (g.save.levelCleared[SEA_ECLIPSE] & 4)) gfx::textC(T(S_all_seasons), 160, 222, C_GOLD);
  else gfx::textC(T(S_btn_back), 160, 222, C_DIM);
}

// ---------------------------------------------------------------------------------------------- weapon select
void updWeapon(const Input& in) {
  static const int pageSize = 4;
  if (pressed(in, plat::BTN_LEFT) || pressed(in, plat::BTN_L)) g.weaponSel = (g.weaponSel + W_COUNT - 1) % W_COUNT;
  if (pressed(in, plat::BTN_RIGHT) || pressed(in, plat::BTN_R)) g.weaponSel = (g.weaponSel + 1) % W_COUNT;
  const int pageStart = (g.weaponSel / pageSize) * pageSize;
  for (int i = 0; i < pageSize; ++i) if (pageStart + i < W_COUNT && inR(in, 8 + i * 77, 10, 72, 70)) { g.weaponSel = pageStart + i; audio::sfx(audio::SFX_UI); }
  if (pressed(in, plat::BTN_B)) { go(SC_CAMPAIGN); return; }
  if (pressed(in, plat::BTN_A) || inR(in, 60, 192, 200, 36)) { if (!weaponUnlocked(g.weaponSel)) { toast(T(S_locked)); audio::sfx(audio::SFX_HURT); } else { audio::sfx(audio::SFX_WAVE); startRun(); } }
}
void drawWeaponTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  textFitC(T(S_weapon_title), 200, 14, 380, C_GOLD, 2);
  gfx::sprC(gfx::knight(), 130, 110, 6.0f);
  gfx::sprC(gfx::icon(weaponIcon(g.weaponSel)), 250, 110, 6.0f);
  textFitC(T(WEAPON_NAME[g.weaponSel]), 250, 168, 190, weaponUnlocked(g.weaponSel) ? (u32)C_TEXT : (u32)C_RED, 2);
  gfx::textC(T(S_first_spell), 200, 210, C_DIM);
}
void drawWeaponBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  const int pageSize = 4, pageStart = (g.weaponSel / pageSize) * pageSize;
  char page[32]; snprintf(page, sizeof page, "%d-%d / %d", pageStart + 1, minT<int>(pageStart + pageSize, W_COUNT), W_COUNT);
  gfx::textC(page, 160, 0, C_DIM);
  for (int i = 0; i < pageSize; ++i) {
    const int w = pageStart + i; const bool has = w < W_COUNT;
    bool sel = has && w == g.weaponSel;
    gfx::panel(8 + i * 77, 10, 72, 70, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : (u32)C_BORDER);
    if (has) { gfx::sprC(gfx::icon(weaponIcon(w)), 44 + i * 77, 34, 2.0f); gfx::textC(T(WEAPON_NAME[w]), 44 + i * 77, 62, weaponUnlocked(w) ? (sel ? (u32)C_GOLD : (u32)C_TEXT) : (u32)C_DIM); if (!weaponUnlocked(w)) gfx::textC(T(S_locked), 44 + i * 77, 73, C_RED); }
  }
  static const StrId dmgW[W_COUNT] = {S_lvm_low, S_lvm_normal, S_lvm_high, S_lvm_very_high, S_lvm_normal, S_lvm_high, S_lvm_high, S_lvm_normal, S_lvm_normal};
  static const StrId atkW[W_COUNT] = {S_lvf_very_high, S_lvf_normal, S_lvf_low, S_lvf_very_low, S_lvf_low, S_lvf_fast, S_lvf_low, S_lvf_fast, S_lvf_very_fast};
  static const StrId rngW[W_COUNT] = {S_lvm_low, S_lvm_very_high, S_lvm_normal, S_lvm_high, S_lvm_normal, S_lvm_high, S_lvm_very_high, S_lvm_low, S_lvm_high};
  const int w = g.weaponSel;
  gfx::panel(8, 90, 304, 94, C_PANEL, C_BORDER);
  gfx::text(T(S_dmg), 18, 100, C_DIM); gfx::textR(T(dmgW[w]), 300, 100, C_TEXT);
  gfx::text(T(S_atk_speed), 18, 114, C_DIM); gfx::textR(T(atkW[w]), 300, 114, C_TEXT);
  gfx::text(T(S_range), 18, 128, C_DIM); gfx::textR(T(rngW[w]), 300, 128, C_TEXT);
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
    const int mod[3] = {11, 6, 6};
    for (int n = 0; n < mod[g.custRow]; ++n) {
      *v[g.custRow] = (u8)((*v[g.custRow] + mod[g.custRow] + d) % mod[g.custRow]);
      bool ok = g.custRow == 0 ? clothUnlocked(*v[g.custRow]) : (g.custRow == 1 ? faceUnlocked(*v[g.custRow]) : colorUnlocked(*v[g.custRow]));
      if (ok) { gfx::setKnight(cosm()); audio::sfx(audio::SFX_UI); break; }
    }
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
  const StrId val[3] = {CLOTH_NAME[clampT((int)g.save.hat, 0, 10)], FACE_NAME[g.save.face], COLOR_NAME[g.save.color]};
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
enum { CAT_COLS = 12, CAT_ROWS = 6, CAT_CELL = 24, CAT_TAB_COUNT = 5, CAT_TAB_W = 64 };
static int catCount() {
  switch (g.catTab) {
    case 0: return NUM_SPELLS;
    case 1: return NUM_ENEMIES;
    case 2: return 11;
    case 3: return 5;
    default: return W_COUNT;
  }
}
static bool catItemUnlocked(int i) {
  switch (g.catTab) {
    case 0: return spellFound(i);
    case 1: return enemyFound(i);
    case 2: return clothUnlocked(i);
    case 3: return musicUnlocked(i);
    default: return weaponUnlocked(i);
  }
}
void updCatalog(const Input& in) {
  int old = g.catSel;
  int touchTab = -1;
  for (int i = 0; i < CAT_TAB_COUNT; ++i) if (inR(in, i * CAT_TAB_W, 0, CAT_TAB_W, 20)) { touchTab = i; break; }
  int nextTab = g.catTab;
  if (touchTab >= 0) nextTab = touchTab;
  else if (pressed(in, plat::BTN_L)) nextTab = (g.catTab + CAT_TAB_COUNT - 1) % CAT_TAB_COUNT;
  else if (pressed(in, plat::BTN_R)) nextTab = (g.catTab + 1) % CAT_TAB_COUNT;
  if (nextTab != g.catTab) {
    g.catTab = nextTab; g.catSel = 0; g.catScroll = 0; g.loreScroll = 0;
    audio::sfx(audio::SFX_UI);
  }
  const int n = catCount();
  if (pressed(in, plat::BTN_RIGHT)) g.catSel = minT(n - 1, g.catSel + 1);
  if (pressed(in, plat::BTN_LEFT)) g.catSel = maxT(0, g.catSel - 1);
  if (pressed(in, plat::BTN_DOWN) && g.catSel + CAT_COLS < n) g.catSel += CAT_COLS;
  if (pressed(in, plat::BTN_UP) && g.catSel >= CAT_COLS) g.catSel -= CAT_COLS;
  for (int i = 0; i < n; ++i) {
    int r = i / CAT_COLS - g.catScroll;
    if (r >= 0 && r < CAT_ROWS && inR(in, 16 + (i % CAT_COLS) * CAT_CELL, 24 + r * CAT_CELL, CAT_CELL, CAT_CELL)) g.catSel = i;
  }
  if (pressed(in, plat::BTN_X)) g.loreScroll++;
  if (pressed(in, plat::BTN_Y)) g.loreScroll = maxT(0, g.loreScroll - 1);
  if (g.catTab == 3 && (pressed(in, plat::BTN_A) || inR(in, 16 + g.catSel * CAT_CELL, 24, CAT_CELL, CAT_CELL)) && musicUnlocked(g.catSel)) {
    audio::musicPreview(g.catSel); audio::sfx(audio::SFX_UI);
  }
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
    textFit(spellName(i), 76, 20, 316, RARITY_COLOR[d.rar], 2);
    snprintf(b, sizeof b, "%s / %s / %s / %s", T(RARITY_NAME[d.rar]), T(ELEM_NAME[d.el]), T(TYPE_NAME[d.ty]), T(ACT_NAME[d.act]));
    gfx::text(b, 76, 42, C_DIM);
    gfx::textWrap(spellDesc(i), 20, 76, 360, C_TEXT, 1, 3);
    gfx::textWrap(spellStats(i), 20, 106, 360, C_GOLD, 1, 2);
    snprintf(b, sizeof b, "%s %d", T(S_found), g.save.spellFound[i]); gfx::text(b, 20, 214, C_DIM);
    gfx::text(T(S_lore), 20, 128, C_EPIC);
    gfx::textWrap(spellLore(i), 20, 140, 360, C_DIM, 1, 7, g.loreScroll);
  } else if (g.catTab == 1) {
    const EnemyDef& d = ENEMIES[i];
    if (!enemyFound(i)) { gfx::sprC(gfx::icon(IC_LOCK), 200, 90, 4.0f); gfx::textC(T(S_not_found), 200, 140, C_DIM, 2); return; }
    gfx::panel(8, 8, 384, 224, C_PANEL, SEASON_COLOR[d.season]);
    gfx::spr(gfx::enemy(i), 20, 20, 3.0f);
    textFit(enemyName(i), 76, 20, 316, SEASON_COLOR[d.season], 2);
    snprintf(b, sizeof b, "%s %d  %s %s  %s %s", T(S_hp), d.hp, T(S_spd), T(speedWord(d.spd)), T(S_range), T(rangeWord(d.range)));
    gfx::text(b, 76, 42, C_DIM);
    gfx::textWrap(enemyDesc(i), 20, 76, 360, C_TEXT, 1, 3);
    snprintf(b, sizeof b, "%s: %s", T(S_special), enemySpecial(i)); gfx::textWrap(b, 20, 106, 360, C_GOLD, 1, 2);
    gfx::text(T(S_lore), 20, 128, C_EPIC);
    gfx::textWrap(enemyLore(i), 20, 140, 360, C_DIM, 1, 7, g.loreScroll);
    snprintf(b, sizeof b, "%s %d", T(S_kills_s), g.save.enemyKills[i]); gfx::text(b, 20, 214, C_DIM);
  } else if (g.catTab == 2) {
    gfx::panel(8, 8, 384, 224, C_PANEL, C_GOLD);
    gfx::sprC(gfx::icon(IC_CHEST), 70, 74, 5.0f);
    gfx::text(T(CLOTH_NAME[i]), 120, 28, C_GOLD, 2);
    if (!clothUnlocked(i)) { gfx::sprC(gfx::icon(IC_LOCK), 200, 96, 3.0f); gfx::textC(T(S_not_found), 200, 144, C_DIM, 2); }
    else { gfx::sprC(gfx::clothingPreview(i), 200, 100, 8.0f); gfx::textC(T(S_ach_unlocked), 200, 164, C_GREEN); }
  } else if (g.catTab == 3) {
    gfx::panel(8, 8, 384, 224, C_PANEL, SEASON_COLOR[i]);
    gfx::sprC(gfx::icon(seasonIcon(i)), 60, 70, 4.0f);
    gfx::text(T(MUSIC_NAME[i]), 100, 28, SEASON_COLOR[i], 2);
    if (!musicUnlocked(i)) { gfx::sprC(gfx::icon(IC_LOCK), 200, 92, 3.0f); gfx::textWrap(T(S_music_locked), 100, 140, 260, C_DIM, 1, 3); }
    else { gfx::textC(T(S_music_play), 200, 156, C_GREEN); gfx::textC(T(S_music_stop), 200, 180, C_DIM); }
  } else {
    const bal::WeaponStat& w = bal::WEAPON[i];
    const bool unlocked = weaponUnlocked(i);
    const float range = i == W_KATANA ? bal::KATANA_RANGE : w.range;
    gfx::panel(8, 8, 384, 224, C_PANEL, unlocked ? C_GOLD : C_BORDER);
    gfx::sprC(gfx::icon(weaponIcon(i)), 58, 78, 4.0f, unlocked ? 0xFFFFFFFFu : HEX(C_BORDER), unlocked ? plat::BLIT_PLAIN : plat::BLIT_TINT);
    textFitC(T(WEAPON_NAME[i]), 112, 22, 270, unlocked ? (u32)C_GOLD : (u32)C_DIM, 2);
    gfx::textWrap(T(WEAPON_FIRE[i]), 108, 52, 270, C_TEXT, 1, 4);
    snprintf(b, sizeof b, "%s: %.1f", T(S_dmg), w.dmg); gfx::text(b, 108, 112, C_GOLD);
    snprintf(b, sizeof b, "%s: %.2f s", T(S_atk_speed), w.interval); gfx::text(b, 108, 130, C_TEXT);
    snprintf(b, sizeof b, "%s: %.0f px", T(S_range), range); gfx::text(b, 108, 148, C_TEXT);
    if (!unlocked) { gfx::sprC(gfx::icon(IC_LOCK), 58, 166, 1.5f); gfx::text(T(S_locked), 108, 176, C_RED); }
  }
}
void drawCatalogBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  const StrId tabName[CAT_TAB_COUNT] = {S_sub_spells, S_sub_bestiary, S_cat_clothes, S_cat_music, S_cat_weapons};
  for (int i = 0; i < CAT_TAB_COUNT; ++i) {
    const float x = (float)(i * CAT_TAB_W);
    const bool active = i == g.catTab;
    gfx::panel(x + 1, 1, CAT_TAB_W - 2, 19, active ? (u32)0x3A2F5C : (u32)C_PANEL, active ? (u32)C_GOLD : (u32)C_BORDER);
    textFitC(T(tabName[i]), x + CAT_TAB_W * 0.5f, 6, CAT_TAB_W - 6, active ? (u32)C_GOLD : (u32)C_DIM, 1);
  }
  const int n = catCount();
  for (int i = g.catScroll * CAT_COLS; i < n && i < (g.catScroll + CAT_ROWS) * CAT_COLS; ++i) {
    int r = i / CAT_COLS - g.catScroll, c = i % CAT_COLS;
    float x = 16.0f + c * CAT_CELL, y = 24.0f + r * CAT_CELL;
    if (i == g.catSel) gfx::rect(x - 1, y - 1, CAT_CELL - 2, CAT_CELL - 2, C_GOLD);
    if (g.catTab == 0) {
      if (spellFound(i)) gfx::spr(gfx::spellIcon(SPELLS[i].ty, SPELLS[i].el, SPELLS[i].act), x + 2, y + 2, 1.0f);
      else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT);
    } else if (g.catTab == 1) {
      if (enemyFound(i)) gfx::spr(gfx::enemy(i), x + 2, y + 2, 1.0f);
      else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT);
    } else if (g.catTab == 2) {
      if (clothUnlocked(i)) gfx::spr(gfx::icon(IC_CHEST), x + 2, y + 2, 1.0f);
      else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT);
    } else if (g.catTab == 3) {
      if (musicUnlocked(i)) gfx::spr(gfx::icon(seasonIcon(i)), x + 2, y + 2, 1.0f);
      else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT);
    } else {
      if (weaponUnlocked(i)) gfx::spr(gfx::icon(weaponIcon(i)), x + 2, y + 2, 1.0f);
      else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT);
    }
  }
  int found = 0;
  for (int i = 0; i < n; ++i) found += catItemUnlocked(i) ? 1 : 0;
  char b[48]; snprintf(b, sizeof b, "%d/%d", found, n);
  gfx::textC(b, 160, 176, C_DIM);
  gfx::textC(g.catTab == 3 ? T(S_music_play) : "L/R OR TAP A TAB   X/Y: LORE   B: BACK", 160, 222, C_DIM);
}

// ---------------------------------------------------------------------------------------------- achievements / lifetime stats
static const StrId ACH_NAME[ACH_COUNT] = {
  S_ach_season_forest, S_ach_season_desert, S_ach_season_snow, S_ach_season_mountain, S_ach_season_eclipse,
  S_ach_kills_1000, S_ach_kills_10000, S_ach_kills_100000,
  S_ach_catalog_forest, S_ach_catalog_desert, S_ach_catalog_snow, S_ach_catalog_mountain, S_ach_catalog_eclipse,
  S_ach_full_catalog, S_ach_little_dummy, S_ach_abyss_veteran, S_ach_touch_grass,
  S_ach_batman, S_ach_joker, S_ach_terraria, S_ach_mario, S_ach_iron_man, S_ach_avatar
};
static const StrId ACH_DESC[ACH_COUNT] = {
  S_ach_season_forest_desc, S_ach_season_desert_desc, S_ach_season_snow_desc, S_ach_season_mountain_desc, S_ach_season_eclipse_desc,
  S_ach_kills_1000_desc, S_ach_kills_10000_desc, S_ach_kills_100000_desc,
  S_ach_catalog_forest_desc, S_ach_catalog_desert_desc, S_ach_catalog_snow_desc, S_ach_catalog_mountain_desc, S_ach_catalog_eclipse_desc,
  S_ach_full_catalog_desc, S_ach_little_dummy_desc, S_ach_abyss_veteran_desc, S_ach_touch_grass_desc,
  S_ach_batman_desc, S_ach_joker_desc, S_ach_terraria_desc, S_ach_mario_desc, S_ach_iron_man_desc, S_ach_avatar_desc
};
void updAchievements(const Input& in) {
  if (pressed(in, plat::BTN_DOWN)) g.achSel = minT(ACH_COUNT - 1, g.achSel + 1);
  if (pressed(in, plat::BTN_UP)) g.achSel = maxT(0, g.achSel - 1);
  for (int row = 0; row < 6; ++row)
    if (inR(in, 8, 24 + row * 28, 304, 26)) g.achSel = minT(ACH_COUNT - 1, g.achScroll + row);
  if (pressed(in, plat::BTN_B) || inR(in, 0, 210, BOT_W, 30)) { go(SC_MENU); return; }
  const int row = g.achSel;
  if (row < g.achScroll) g.achScroll = row;
  if (row >= g.achScroll + 6) g.achScroll = row - 5;
  g.achScroll = clampT(g.achScroll, 0, ACH_COUNT - 6);
}
void drawAchievementsTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  gfx::textC(T(S_ach_title), 200, 10, C_GOLD, 2);
  static const StrId labels[9] = {S_stat_games, S_stat_wins, S_stat_losses, S_stat_kills, S_stat_picks, S_stat_casts, S_stat_upgrades, S_stat_throws, S_stat_time};
  char val[32];
  const u32 stats[8] = {g.save.gamesPlayed, g.save.runsWon, g.save.runsLost, g.save.totalKills,
                        g.save.spellsAcquired, g.save.spellsCasted, g.save.spellUpgrades, g.save.weaponThrows};
  for (int i = 0; i < 9; ++i) {
    const int x = 8 + (i & 1) * 196, y = 30 + (i >> 1) * 28;
    gfx::panel(x, y, 188, 24, C_PANEL, C_BORDER);
    gfx::text(T(labels[i]), x + 6, y + 4, C_DIM);
    if (i == 8) snprintf(val, sizeof val, "%lu:%02lu", (unsigned long)(g.save.playTimeSeconds / 3600),
                         (unsigned long)((g.save.playTimeSeconds / 60) % 60));
    else snprintf(val, sizeof val, "%lu", (unsigned long)stats[i]);
    gfx::textR(val, x + 182, y + 4, C_TEXT);
  }
  const int id = clampT(g.achSel, 0, ACH_COUNT - 1);
  u32 current, target; achievementProgress(g.save, id, current, target);
  textFit(T(ACH_NAME[id]), 12, 172, 376, achievementUnlocked(g.save, id) ? (u32)C_GREEN : (u32)C_GOLD, 2);
  gfx::textWrap(T(ACH_DESC[id]), 12, 190, 376, C_TEXT, 1, 2);
  snprintf(val, sizeof val, "%s  %lu/%lu", T(achievementUnlocked(g.save, id) ? S_ach_unlocked : S_ach_locked),
           (unsigned long)current, (unsigned long)target);
  gfx::text(val, 12, 228, C_DIM);
}
void drawAchievementsBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  header(T(S_ach_title));
  for (int row = 0; row < 6; ++row) {
    const int id = g.achScroll + row, y = 24 + row * 28;
    if (id >= ACH_COUNT) break;
    const bool done = achievementUnlocked(g.save, id), selected = id == g.achSel;
    gfx::panel(8, y, 304, 25, selected ? (u32)0x3A2F5C : (u32)C_PANEL,
               selected ? (u32)C_GOLD : (done ? (u32)C_GREEN : (u32)C_BORDER));
    gfx::spr(gfx::icon(IC_TROPHY), 12, y + 5, 1.0f, done ? (u32)C_GREEN : (u32)C_DIM,
             done ? plat::BLIT_TINT : plat::BLIT_PLAIN);
    gfx::text(T(ACH_NAME[id]), 30, y + 7, done ? (u32)C_GREEN : (u32)C_TEXT);
    u32 current, target; achievementProgress(g.save, id, current, target);
    char progress[24]; snprintf(progress, sizeof progress, "%lu/%lu", (unsigned long)current, (unsigned long)target);
    gfx::textR(progress, 306, y + 7, done ? (u32)C_GREEN : (u32)C_GOLD);
  }
  gfx::textC(T(S_ach_hint), 160, 222, C_DIM);
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
  if (sl.id < 0) { *name = T(WEAPON_NAME[-1 - sl.id]); *desc = T(WEAPON_FIRE[-1 - sl.id]); *stats = T(S_first_spell); *col = C_GOLD; *icon = &gfx::icon(weaponIcon(-1 - sl.id)); }
  else { *name = spellName(sl.id); *desc = spellDesc(sl.id); *stats = spellStats(sl.id); *col = RARITY_COLOR[SPELLS[sl.id].rar]; *icon = &gfx::spellIcon(SPELLS[sl.id].ty, SPELLS[sl.id].el, SPELLS[sl.id].act); }
  (void)dummy;
}
static void compactSlotLabel(const char* src, char* out, size_t cap) {
  size_t used = 0; int glyphs = 0;
  while (*src && glyphs < 11) {
    const unsigned char c = (unsigned char)*src;
    const size_t bytes = c < 0x80 ? 1 : (c < 0xE0 ? 2 : (c < 0xF0 ? 3 : 4));
    if (used + bytes >= cap) break;
    memcpy(out + used, src, bytes); used += bytes; src += bytes; ++glyphs;
  }
  out[used] = 0;
  if (*src) {
    while (used && gfx::textWidth(out, 1) + gfx::textWidth("...", 1) > 66) {
      size_t cut = used - 1;
      while (cut && (((unsigned char)out[cut] & 0xC0) == 0x80)) --cut;
      used = cut; out[used] = 0;
    }
    if (used + 4 <= cap) { memcpy(out + used, "...", 4); }
  }
}

void drawOfferCard(const Sim& s, int i, float y, bool sel) {
  const int id = s.offers[i];
  const bool up = s.offersAreUpgrades;
  const char *name, *desc, *stats; u32 col; const gfx::Spr* ic;
  if (up) slotText(s, id, &name, &desc, &stats, &col, &ic);
  else { name = spellName(id); desc = spellDesc(id); stats = spellStats(id); col = RARITY_COLOR[SPELLS[id].rar]; ic = &gfx::spellIcon(SPELLS[id].ty, SPELLS[id].el, SPELLS[id].act); }
  gfx::panel(8, y, 304, 56, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : col);
  gfx::spr(*ic, 14, y + 12, 2.0f);
  const float textCx = 179.0f; // center of the 54..304 region beside the icon
  if (!up && simSpellLevelOf(s, id) == 0 && g.save.spellFound[id] == 0) gfx::textC("NEW!", textCx, y + 3, C_GREEN);
  if (up) { char b[24]; const int spellId = s.slots[id].id; snprintf(b, sizeof b, "%s %d/%d", T(S_plevel), s.slots[id].lv + 1, SPELLS[spellId].maxLevel); gfx::textC(b, textCx, y + 3, C_GOLD); }
  textFitC(name, textCx, y + 12, 242, col, 1);
  gfx::textWrap(desc, 54, y + 25, 250, C_TEXT, 1, 1);
  gfx::textWrap(stats, 54, y + 42, 250, C_GOLD, 1, 1);
}

static bool spellHasEffect(const SpellDef& d, EffKind kind, float* a = nullptr, int* index = nullptr) { for (int k = 0; k < 3; ++k) if (d.eff[k].kind == kind) { if (a) *a = d.eff[k].a; if (index) *index = k; return true; } return false; }
static bool spellHasGenericRankScale(const SpellDef& d) {
  for (int k = 0; k < 3; ++k) {
    switch ((EffKind)d.eff[k].kind) {
      case E_NONE: case E_TIME_STOP: case E_CHAINPLUS: case E_BLOCK_WAVE: case E_PIERCE: case E_BOUNCE_WALL: case E_HOMING: break;
      default: return true;
    }
  }
  return false;
}
static void spellOverlay(const Sim& s, int slot, float x, float y) {
  if (slot >= s.nSlots || s.slots[slot].id < 0) return;
  const SpellDef& d = SPELLS[s.slots[slot].id]; float v = 0; char b[24];
  for (int k = 0; k < 3; ++k) { const Eff& e = d.eff[k]; if (e.kind == E_NTH_RING || e.kind == E_NTH_BLAST || e.kind == E_NTH_STUN || e.kind == E_NTH_DOUBLE || e.kind == E_NTH_STRIKE3 || e.kind == E_NTH_BOLT || e.kind == E_AVATAR) { int n = maxT(1, (int)e.a); snprintf(b, sizeof b, "%d/%d", (int)(s.p.throws % n), n); gfx::textC(b, x + 36, y + 3, C_GOLD); return; } }
  int effectIndex = -1; EffKind cooldownKind = E_NONE;
  if (spellHasEffect(d, E_PERIODIC_FLARE, &v, &effectIndex)) cooldownKind = E_PERIODIC_FLARE;
  else if (spellHasEffect(d, E_PERIODIC_BOMB, &v, &effectIndex)) cooldownKind = E_PERIODIC_BOMB;
  else if (spellHasEffect(d, E_REGEN, &v, &effectIndex)) cooldownKind = E_REGEN;
  else if (spellHasEffect(d, E_KILLHEAL, &v, &effectIndex)) cooldownKind = E_KILLHEAL;
  else if (spellHasEffect(d, E_CRIT_HEAL, &v, &effectIndex)) cooldownKind = E_CRIT_HEAL;
  else if (spellHasEffect(d, E_TIME_STOP, &v, &effectIndex)) cooldownKind = E_TIME_STOP;
  if (cooldownKind != E_NONE) {
    float cur = cooldownKind == E_PERIODIC_FLARE ? s.p.flareT : cooldownKind == E_PERIODIC_BOMB ? s.p.bombT : cooldownKind == E_REGEN ? s.p.regenT : cooldownKind == E_KILLHEAL ? s.p.killHealT : cooldownKind == E_CRIT_HEAL ? s.p.critHealT : maxT(0.0f, d.eff[effectIndex].c - s.p.timeStopCd);
    float interval = (effectIndex >= 0 && d.eff[effectIndex].b > 0) ? d.eff[effectIndex].b : maxT(v, 1.0f);
    if (cooldownKind == E_TIME_STOP) interval = d.eff[effectIndex].c;
    gfx::rect(x + 2, y + 2, 68, 3, C_VOID); gfx::rect(x + 2, y + 2, 68 * clampT(cur / interval, 0.0f, 1.0f), 3, C_EPIC);
  }
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
    char bank[64]; snprintf(bank, sizeof bank, "%s   %s", Tn(S_stores_left, s.bankStores), Tn(S_use_stored, s.bankedPoints));
    gfx::textC(bank, 160, 222, C_DIM);
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
      if (has && s.slots[i].id >= 0) spellOverlay(s, i, x, y);
      if (has) {
        const char *nm, *ds, *st; u32 col; const gfx::Spr* ic;
        slotText(s, i, &nm, &ds, &st, &col, &ic);
        char label[32]; compactSlotLabel(nm, label, sizeof label);
        gfx::textC(label, x + 36, y + 15, col, 1);
        gfx::spr(*ic, x + 4, y + 24, 1.0f);
        if (s.slots[i].id >= 0) { snprintf(b, sizeof b, "%s%d", T(S_plevel), s.slots[i].lv); gfx::textC(b, x + 49, y + 28, C_GOLD, 1); }
      } else gfx::textC("-", x + 36, y + 16, C_BORDER);
    }
    if (g.slotSel < s.nSlots) {
      const char *nm, *ds, *st; u32 col; const gfx::Spr* ic;
      slotText(s, g.slotSel, &nm, &ds, &st, &col, &ic);
      textFit(nm, 8, 160, 238, col, 1);
      snprintf(b, sizeof b, "%s", Tn(S_slots, s.nSlots)); gfx::textR(b, 312, 160, C_DIM);
      if (s.slots[g.slotSel].id >= 0) {
        const SpellDef& selected = SPELLS[s.slots[g.slotSel].id];
        const int lv = clampT((int)s.slots[g.slotSel].lv, 1, (int)selected.maxLevel);
        const int bonusPct = (int)(100.0f * bal::LV_BONUS * (lv - 1) + 0.5f);
        const char* rankEffect = spellHasGenericRankScale(selected) ? Tn(S_spell_rank_gain, bonusPct) : T(S_spell_rank_custom);
        snprintf(b, sizeof b, "%s %d/%d | %s", T(S_spell_rank), lv, selected.maxLevel, rankEffect);
        gfx::textWrap(b, 8, 202, 304, C_GOLD, 1, 1);
        snprintf(b, sizeof b, "%s: %s", T(S_spell_base_stats), st);
        gfx::textWrap(b, 8, 212, 304, C_DIM, 1, 3);
      } else {
        gfx::textWrap(st, 8, 202, 304, C_GOLD, 1, 2);
      }
      gfx::textWrap(ds, 8, 172, 304, C_TEXT, 1, 3);
    } else gfx::textC(T(S_empty_slot), 160, 180, C_DIM);
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
      if (pressed(in, plat::BTN_X)) simStorePoints(s);
      if (pressed(in, plat::BTN_Y)) simUseStoredPoints(s);
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
    case SC_ACHIEVEMENTS: updAchievements(in); break;
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
    case SC_ACHIEVEMENTS: drawAchievementsTop(); break;
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
    case SC_ACHIEVEMENTS: drawAchievementsBot(); break;
    case SC_OPTIONS: drawOptionsBot(); break;
    case SC_CREDITS:
      gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
      gfx::textC(T(S_btn_back), 160, 100, C_DIM);
      gfx::sprC(gfx::drone(), 160, 180, 4.0f);       // the easter-egg drone
      if (g.secretCatalog) gfx::textC("SECRET UNLOCKED!", 160, 220, C_GOLD);
      break;
  }
}
