#include "game.h"
#include "sim.h"
#include "gfx.h"
#include "i18n.h"
#include "audio.h"
#include "save.h"
#include <stdio.h>
#include <string.h>

// Luma cheat target: write 0xAE13D3B to this volatile word, then press SELECT
// on any non-gameplay screen or during a run. The symbol is exported by the ELF so a cheat can resolve it;
// no debug menu is exposed until the magic value is present.
extern "C" { volatile u32 g_debugUnlock = 0; }

namespace {
using plat::Input;
const u32 DEBUG_MAGIC = 0x0AE13D3Bu;
enum Screen { SC_MENU, SC_CAMPAIGNS, SC_CAMPAIGN, SC_WEAPON, SC_CUSTOM, SC_GAME, SC_CATALOG, SC_ACHIEVEMENTS, SC_OPTIONS, SC_CREDITS, SC_NOTES };

struct G {
  Screen scr = SC_MENU, back = SC_MENU;
  SaveData save;
  bool endless = false, allSeasons = false;
  u8 endlessMask = 0x1F;
  u8 endlessLevelMask[SEA_COUNT] = {7, 7, 7, 7, 7};
  int endlessSel = 0, endlessDifficulty = 1, endlessLives = 1, endlessSpell = 0, dbgMainSel = 0;
  bool endlessRulesOpen = false; int notesScroll = 0; char notes[4096] = ""; size_t notesLen = 0;
  bool endlessRandomSpells = true; float sacrificeHoldT = 0.0f; int sacrificeHoldSlot = -1;
  float fpsElapsed = 0.0f; unsigned int fpsFrames = 0; int fps = 0;
  int season = 0, level = 0, weaponSel = 0, menuSel = 0;
  int tab = 0, slotSel = 0, allocSel = 0, offerSel = 0, runInfoScroll = 0;
  int runInfoTouchY = 0, runInfoDragPx = 0; bool runInfoTouching = false;
  int catTab = 0, catSel = 0, catScroll = 0, achSel = 0, achScroll = 0, optSel = 0, custRow = 0, loreScroll = 0;
  int campaignSel = 0, campaignContentSel = 0; bool campaignContentFocus = false;
  bool paused = false, debug = false, debugMenu = false, dbgSpellPicker = false; int pauseSel = 0, pauseTab = 0, dbgSel = 0, dbgSpellSel = 0;
  bool achKnown[ACH_COUNT] = {}; u8 achQueue[ACH_COUNT] = {}; int achQueueHead = 0, achQueueCount = 0, achPopupId = -1; float achPopupT = 0;
  bool saveWriteBlocked = false;
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
static const char* fitLabel(const char* t, int maxW);
static void textFitC(const char* t, float cx, float y, int maxW, u32 col, int preferred = 2);
static void textFit(const char* t, float x, float y, int maxW, u32 col, int preferred = 1);
void header(const char* t, u32 col = C_GOLD);
void toast(const char* t) { snprintf(g.toast, sizeof g.toast, "%s", t); g.toastT = 2.5f; }
void scanAchievementUnlocks(bool notify) {
  for (int i = 0; i < ACH_COUNT; ++i) {
    const bool unlocked = achievementUnlocked(g.save, i);
    if (unlocked && !g.achKnown[i] && notify && g.achQueueCount < ACH_COUNT) {
      const int tail = (g.achQueueHead + g.achQueueCount) % ACH_COUNT;
      g.achQueue[tail] = (u8)i; ++g.achQueueCount;
    }
    if (unlocked) g.achKnown[i] = true;
  }
}
void persist() {
  if (g.saveWriteBlocked) { toast(T(S_save_recovery)); return; }
  scanAchievementUnlocks(true);
  if (!saveStore(g.save)) toast(T(S_save_failed));
}
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
static const StrId MUSIC_NAME[5] = {S_music_forest, S_music_desert, S_music_snow, S_music_mountain, S_music_eclipse};
static const StrId CLOTH_NAME[10] = {S_cloth_default, S_cloth_wizard, S_cloth_leaf, S_cloth_crown, S_cloth_none, S_cloth_nightwarden, S_cloth_grinmask, S_cloth_redcap, S_cloth_arcwarden, S_cloth_elementalist};
bool clothUnlocked(int id) {
  if (g.save.unlockAll || g.secretCatalog || id < 5) return true;
  static const int ach[4] = {ACH_LITTLE_DUMMY, ACH_ABYSS_VETERAN, ACH_KILLS_1000, ACH_FULL_CATALOG};
  if (id >= 5 && id < 9) return achievementUnlocked(g.save, ach[id - 5]);
  return id == 9 && achievementUnlocked(g.save, ACH_ELEMENTALIST);
}
bool faceUnlocked(int id) {
  return id >= 0 && id < 6;
}
bool colorUnlocked(int id) {
  return id >= 0 && id < 6;
}
bool musicUnlocked(int id) { return g.save.unlockAll || g.secretCatalog || (id >= 0 && id < 5 && achievementUnlocked(g.save, ACH_SEASON_FOREST + id)); }
bool allSeasonsComplete() { for (int i = 0; i < SEA_COUNT; ++i) if ((g.save.levelCleared[i] & 7) != 7) return false; return true; }
bool weaponUnlocked(int id) { return id == W_INFINITY_GLOVE ? (g.save.unlockAll || g.secretCatalog || achievementUnlocked(g.save, ACH_UNSTOPPABLE)) : (id != W_MACHINE_GUN || g.save.unlockAll || g.secretCatalog || allSeasonsComplete()); }
bool endlessSpellEligible(int id) {
  if (id < 0 || id >= NUM_SPELLS) return false;
  int season = g.season;
  for (int i = 0; i < SEA_COUNT; ++i) if (g.endlessMask & (1u << i)) { season = i; break; }
  const SpellDef& d = SPELLS[id];
  if (d.sc == SC_GLOBAL) return true;
  if (d.sc == SC_SEASON) return d.season == season;
  return SEASON_ELEM[season][0] == d.el || SEASON_ELEM[season][1] == d.el;
}
void stepEndlessSpell(int direction) {
  for (int step = 0; step < NUM_SPELLS; ++step) {
    g.endlessSpell = (g.endlessSpell + (direction >= 0 ? 1 : NUM_SPELLS - 1)) % NUM_SPELLS;
    if (endlessSpellEligible(g.endlessSpell)) return;
  }
}

void startRun() {
  g.save.lastWeapon = (u8)g.weaponSel;
  addLifetime(g.save.gamesPlayed, 1);
  persist();
  simStart(g_sim, g.season, g.level, g.endless, g.weaponSel, (u32)plat::nowMs() | 1u, g.allSeasons, g.endlessMask,
           g.endless ? g.endlessRandomSpells : true, g.endless ? g.endlessLives : 1,
           -1,
           g.endless ? g.endlessDifficulty : 1);
  g_sim.shakeEnabled = !g.save.shakeOff;
  g.scr = SC_GAME; g.tab = 0; g.slotSel = 0; g.allocSel = 0; g.offerSel = 0; g.runInfoScroll = 0; g.runInfoTouching = false; g.paused = false; g.debug = false; g.dbgSpellPicker = false; g.sacrificeHoldT = 0.0f; g.sacrificeHoldSlot = -1;
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
  addSaveCoins(g.save, (u32)maxT(0, s.runCoins));
  for (int i = 0; i < NUM_ENEMIES; ++i) {
    if (s.seenEnemy[i]) g.save.enemySeen[i] = 1;
    int k = (int)g.save.enemyKills[i] + s.runKills[i]; g.save.enemyKills[i] = (u16)minT(k, 65535);
  }
  for (int i = 0; i < NUM_SPELLS; ++i) {
    if (s.seenSpell[i]) g.save.spellFound[i] = (u16)minT((int)g.save.spellFound[i] + s.seenSpell[i], 65535);
    if (s.usedSpell[i]) g.save.spellUsed[i] = 1;
  }
  if (s.endless) {
    u16& best = s.allSeasons ? g.save.bestAllSeasonsWave : g.save.bestWave[s.season];
    best = (u16)maxT((int)best, minT(s.wave, 65535));
  }
  if (victory && !s.endless) {
    g.save.levelCleared[s.season] |= (u8)(1 << s.levelIdx);
    const u8 seasonBit = (u8)(1u << s.season);
    if ((g.save.levelCleared[s.season] & 7) == 7 && !(g.save.coinSeasonClaimed & seasonBit)) {
      addSaveCoins(g.save, 100); g.save.coinSeasonClaimed |= seasonBit;
    }
  }
  persist();
  g.season = s.season;
  go(s.endless ? SC_MENU : SC_CAMPAIGN);
}

int seasonIcon(int s) { static const int ic[5] = {IC_LEAF, IC_PYR, IC_SNOW, IC_PEAK, IC_ECL}; return ic[s]; }
StrId gimmick(int s) { static const StrId g_[5] = {S_gim_forest, S_gim_desert, S_gim_snow, S_gim_mountain, S_gim_eclipse}; return g_[s]; }
StrId speedWord(int w) { static const StrId t[7] = {S_lvf_stationary, S_lvf_very_slow, S_lvf_slow, S_lvf_low, S_lvf_normal, S_lvf_fast, S_lvf_very_fast}; return t[w % 7]; }
StrId rangeWord(int w) { static const StrId t[3] = {S_lvm_touch, S_lvm_high, S_lvm_very_high}; return t[w % 3]; }
int weaponIcon(int w) { static const int t[W_COUNT] = {IC_DAGGER, IC_BOW, IC_AXE, IC_STAFF, IC_SHOTGUN, IC_PISTOL, IC_TRIDENT, IC_KATANA, IC_MACHINE_GUN, IC_INFINITY}; return t[clampT(w, 0, W_COUNT - 1)]; }

// Campaign membership is separate from runtime season/save IDs. The current
// Main Campaign entries deliberately reference the five legacy seasons; event
// entries are visible placeholders until their gameplay and save IDs exist.
struct CampaignEntry { StrId name; int season; int icon; bool comingSoon; };
struct CampaignInfo { StrId name, shortDesc, description; int icon; const CampaignEntry* entries; int count; };
static const CampaignEntry MAIN_CAMPAIGN_ENTRIES[] = {
  {S_campaign_current_forest, SEA_FOREST, IC_LEAF, false},
  {S_campaign_current_desert, SEA_DESERT, IC_PYR, false},
  {S_campaign_current_snow, SEA_SNOW, IC_SNOW, false},
  {S_campaign_current_mountain, SEA_MOUNTAIN, IC_PEAK, false},
  {S_campaign_current_eclipse, SEA_ECLIPSE, IC_ECL, false},
};
static const CampaignEntry EVENT_CAMPAIGN_ENTRIES[] = {
  {S_campaign_christmas, -1, IC_EVENT_CHRISTMAS, true},
  {S_campaign_easter, -1, IC_EVENT_EASTER, true},
  {S_campaign_halloween, -1, IC_EVENT_HALLOWEEN, true},
};
static const CampaignEntry FUTURE_CAMPAIGN_ENTRIES[] = {
  {S_campaign_future_cyberpunk, -1, IC_CYBER, true},
  {S_campaign_future_steampunk, -1, IC_STEAM, true},
};
static const CampaignInfo CAMPAIGNS[] = {
  {S_campaign_main, S_campaign_main_short, S_campaign_main_desc, IC_LEAF, MAIN_CAMPAIGN_ENTRIES, 5},
  {S_campaign_events, S_campaign_events_short, S_campaign_events_desc, IC_STAR, EVENT_CAMPAIGN_ENTRIES, 3},
  {S_campaign_future, S_campaign_future_short, S_campaign_future_desc, IC_GEAR, FUTURE_CAMPAIGN_ENTRIES, 2},
};
enum { CAMPAIGN_COUNT = (int)(sizeof CAMPAIGNS / sizeof CAMPAIGNS[0]) };

static void openCampaignEntry() {
  const CampaignInfo& campaign = CAMPAIGNS[clampT(g.campaignSel, 0, CAMPAIGN_COUNT - 1)];
  const CampaignEntry& entry = campaign.entries[clampT(g.campaignContentSel, 0, campaign.count - 1)];
  if (entry.comingSoon || entry.season < 0) { toast(T(S_campaign_coming)); return; }
  g.season = entry.season; g.level = 0; g.endless = false; g.allSeasons = false;
  g.back = SC_CAMPAIGNS; go(SC_CAMPAIGN);
}
void updCampaignHub(const Input& in) {
  if (pressed(in, plat::BTN_B)) {
    if (g.campaignContentFocus) g.campaignContentFocus = false;
    else go(SC_MENU);
    return;
  }
  if (!g.campaignContentFocus) {
    int selected = g.campaignSel;
    if (pressed(in, plat::BTN_DOWN)) selected = (selected + 1) % CAMPAIGN_COUNT;
    if (pressed(in, plat::BTN_UP)) selected = (selected + CAMPAIGN_COUNT - 1) % CAMPAIGN_COUNT;
    if (pressed(in, plat::BTN_LEFT)) selected = (selected + CAMPAIGN_COUNT - 1) % CAMPAIGN_COUNT;
    if (pressed(in, plat::BTN_RIGHT)) selected = (selected + 1) % CAMPAIGN_COUNT;
    // Touch belongs to the bottom screen: never interpret its coordinates as
    // taps on the top-screen campaign cards (the coordinate overlap selected
    // the wrong group and could immediately focus the first card).
    bool touchOpen = false;
    for (int i = 0; i < CAMPAIGN_COUNT; ++i) {
      const int y = 40 + i * 70;
      if (inR(in, 8, y, 304, 62)) {
        if (selected == i) touchOpen = true;
        selected = i;
      }
    }
    if (selected != g.campaignSel) { g.campaignSel = selected; g.campaignContentSel = 0; audio::sfx(audio::SFX_UI); }
    if (pressed(in, plat::BTN_A) || touchOpen) {
      g.campaignContentFocus = true; g.campaignContentSel = 0; audio::sfx(audio::SFX_UI);
    }
    return;
  }
  const CampaignInfo& campaign = CAMPAIGNS[clampT(g.campaignSel, 0, CAMPAIGN_COUNT - 1)];
  if (pressed(in, plat::BTN_UP)) g.campaignContentSel = (g.campaignContentSel + campaign.count - 1) % campaign.count;
  if (pressed(in, plat::BTN_DOWN)) g.campaignContentSel = (g.campaignContentSel + 1) % campaign.count;
  for (int i = 0; i < campaign.count; ++i) {
    const int y = 27 + i * 35;
    if (inR(in, 8, y, 304, 32)) {
      if (g.campaignContentSel == i) openCampaignEntry();
      else { g.campaignContentSel = i; audio::sfx(audio::SFX_UI); }
      return;
    }
  }
  if (pressed(in, plat::BTN_A)) { audio::sfx(audio::SFX_UI); openCampaignEntry(); }
}
void drawCampaignHubTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  gfx::textC(T(S_campaigns_title), 200, 8, C_GOLD, 2);
  for (int i = 0; i < CAMPAIGN_COUNT; ++i) {
    const CampaignInfo& c = CAMPAIGNS[i]; const int y = 34 + i * 68; const bool selected = i == g.campaignSel;
    gfx::panel(8, y, 184, 60, selected ? (u32)0x3A2F5C : (u32)C_PANEL, selected ? (u32)C_GOLD : (u32)C_BORDER);
    gfx::spr(gfx::icon(c.icon), 17, y + 18, 1.5f, selected ? (u32)C_GOLD : (u32)C_TEXT);
    textFit(T(c.name), 48, y + 8, 136, selected ? (u32)C_GOLD : (u32)C_TEXT, 2);
    gfx::textWrap(T(c.shortDesc), 48, y + 30, 136, C_DIM, 1, 2);
  }
  const CampaignInfo& chosen = CAMPAIGNS[clampT(g.campaignSel, 0, CAMPAIGN_COUNT - 1)];
  gfx::panel(200, 34, 192, 196, C_PANEL, C_BORDER);
  gfx::sprC(gfx::icon(chosen.icon), 296, 72, 2.5f, C_GOLD);
  textFitC(T(chosen.name), 296, 106, 180, C_GOLD, 2);
  gfx::rect(214, 132, 164, 1, C_BORDER);
  gfx::textWrap(T(chosen.description), 214, 145, 164, C_TEXT, 1, 7);
  if (g.campaignContentFocus) gfx::textC(T(S_campaign_browse), 200, 222, C_DIM);
  else gfx::textC(T(S_campaign_choose), 200, 222, C_DIM);
}
void drawCampaignHubBottom() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID); header(T(S_campaigns_title));
  const CampaignInfo& campaign = CAMPAIGNS[clampT(g.campaignSel, 0, CAMPAIGN_COUNT - 1)];
  if (!g.campaignContentFocus) {
    for (int i = 0; i < CAMPAIGN_COUNT; ++i) {
      const CampaignInfo& entry = CAMPAIGNS[i]; const int y = 40 + i * 70;
      const bool selected = i == g.campaignSel;
      gfx::panel(8, y, 304, 62, selected ? (u32)0x3A2F5C : (u32)C_PANEL,
                 selected ? (u32)C_GOLD : (u32)C_BORDER);
      gfx::spr(gfx::icon(entry.icon), 16, y + 18, 1.5f, selected ? (u32)C_GOLD : (u32)C_DIM);
      textFit(T(entry.name), 48, y + 8, 248, selected ? (u32)C_GOLD : (u32)C_TEXT, 2);
      gfx::textWrap(T(entry.shortDesc), 48, y + 31, 250, C_DIM, 1, 2);
    }
    gfx::textC(T(S_campaign_choose), 160, 222, C_DIM);
    return;
  }
  for (int i = 0; i < campaign.count; ++i) {
    const CampaignEntry& entry = campaign.entries[i]; const int y = 26 + i * 35;
    const bool selected = i == g.campaignContentSel;
    gfx::panel(8, y, 304, 32, selected ? (u32)0x3A2F5C : (u32)C_PANEL,
               selected ? (u32)C_GOLD : (u32)C_BORDER);
    gfx::spr(gfx::icon(entry.icon), 14, y + 8, 1.0f, entry.comingSoon ? (u32)C_DIM : (u32)C_TEXT);
    gfx::text(T(entry.name), 38, y + 10, selected ? C_GOLD : C_TEXT);
    if (entry.comingSoon) gfx::textR(T(S_campaign_coming), 304, y + 10, C_DIM);
    else {
      const int done = (int)((g.save.levelCleared[entry.season] & 1) != 0) +
                       (int)((g.save.levelCleared[entry.season] & 2) != 0) +
                       (int)((g.save.levelCleared[entry.season] & 4) != 0);
      char progress[12]; snprintf(progress, sizeof progress, "%d/3", done);
      gfx::textR(progress, 302, y + 10, done == 3 ? (u32)C_GREEN : (u32)C_DIM);
      if (!seasonUnlocked(g.save, entry.season)) gfx::spr(gfx::icon(IC_LOCK), 272, y + 8, 1.0f, C_BORDER, plat::BLIT_TINT);
    }
  }
  gfx::textC(T(S_campaign_browse), 160, 222, C_DIM);
}

void button(float x, float y, float w, float h, const char* label, bool sel, u32 col = C_TEXT) {
  gfx::panel(x, y, w, h, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : (u32)C_BORDER);
  // Buttons stay at the compact bitmap scale; scale 2 is reserved for headings and titles.
  int scale = 1; while (scale > 1 && gfx::textWidth(label, scale) > (int)w - 8) --scale;
  if (gfx::textWidth(label, scale) > (int)w - 8) { char clipped[96]; snprintf(clipped,sizeof clipped,"%s",label); while (clipped[0] && gfx::textWidth(clipped,1) > (int)w - 8) clipped[strlen(clipped)-1]=0; gfx::textC(clipped,x+w*0.5f,y+(h-10.0f)*0.5f,sel?(u32)C_GOLD:col,1); } else gfx::textC(label, x + w * 0.5f, y + (h - 10.0f) * 0.5f, sel ? (u32)C_GOLD : col, scale);
}
static const char* fitLabel(const char* t, int maxW) {
  static char clipped[4][192]; static int buffer=0; char* out=clipped[(buffer++)&3];
  snprintf(out,192,"%s",t?t:"");
  const int room=maxW>12?maxW-12:maxW;
  while (out[0] && gfx::textWidth(out,1)>room) { size_t n=strlen(out); do { --n; } while(n>0 && (((unsigned char)out[n]&0xC0)==0x80)); out[n]=0; }
  if (strcmp(out,t?t:"")) { size_t n=strlen(out); if(n<188){out[n++]='.';out[n++]='.';out[n]=0;} }
  return out;
}
static int fitScale(const char* t, int maxW, int preferred = 2) { return preferred >= 2 && gfx::textWidth(t, preferred) <= maxW ? preferred : 1; }
static void textFitC(const char* t, float cx, float y, int maxW, u32 col, int preferred) { const char* f=fitLabel(t,maxW); gfx::textC(f,cx,y,col,fitScale(f,maxW,preferred)); }
static void textFit(const char* t, float x, float y, int maxW, u32 col, int preferred) { const char* f=fitLabel(t,maxW); gfx::text(f,x,y,col,gfx::textWidth(f,preferred)<=maxW?preferred:1); }
void header(const char* t, u32 col) { gfx::rect(0, 0, BOT_W, 20, C_PANEL); textFitC(t, BOT_W / 2, 6, BOT_W - 8, col, 1); }

// ---------------------------------------------------------------------------------------------- menu
void runDebug(int i);
static void forceAchievements() {
  for (int sea = 0; sea < SEA_COUNT; ++sea) { g.save.levelCleared[sea] = 7; g.save.bestWave[sea] = 70; }
  g.save.bestAllSeasonsWave = 70; g.save.totalKills = 100000; g.save.playTimeSeconds = 50u * 60u * 60u;
  for (int i = 0; i < NUM_SPELLS; ++i) { g.save.spellFound[i] = 1; g.save.spellUsed[i] = 1; }
  for (int i = 0; i < NUM_ENEMIES; ++i) g.save.enemySeen[i] = 1;
  persist(); toast("ALL ACHIEVEMENTS UNLOCKED");
}
static void runMainDebug(int action) {
  switch (action) {
    case 0: forceAchievements(); break;
    case 1: g.save.unlockAll ^= 1; persist(); toast(g.save.unlockAll ? "ALL CONTENT UNLOCKED" : "CONTENT LOCKS RESTORED"); break;
    case 2:
      for (int i = 0; i < NUM_SPELLS; ++i) g.save.spellFound[i] = maxT<u16>(g.save.spellFound[i], 1);
      for (int i = 0; i < NUM_ENEMIES; ++i) g.save.enemySeen[i] = 1;
      persist(); toast("CATALOG DISCOVERIES COMPLETE"); break;
    case 3: toast("MAIN MENU DEBUG READY"); break;
    case 4: if (gfx::exportAtlas()) toast("ATLAS SAVED: debug_spritesheet.pam"); else toast("ATLAS EXPORT FAILED"); break;
  }
}
static bool updMainDebug(const Input& in) {
  if (g.scr == SC_GAME) return false;
  const u32 secretMask = plat::BTN_L | plat::BTN_R | plat::BTN_X | plat::BTN_Y;
  if (pressed(in, plat::BTN_SELECT) && (in.held & secretMask) == secretMask) g_debugUnlock = DEBUG_MAGIC;
  if (pressed(in, plat::BTN_SELECT) && g_debugUnlock == DEBUG_MAGIC) {
    g.debugMenu = !g.debugMenu;
    return true;
  }
  if (!g.debugMenu) return false;
  if (pressed(in, plat::BTN_B)) { g.debugMenu = false; return true; }
  if (pressed(in, plat::BTN_DOWN)) g.dbgMainSel = (g.dbgMainSel + 1) % 5;
  if (pressed(in, plat::BTN_UP)) g.dbgMainSel = (g.dbgMainSel + 4) % 5;
  for (int i = 0; i < 5; ++i) if (inR(in, 14, 27 + i * 35, 292, 32)) { g.dbgMainSel = i; runMainDebug(i); return true; }
  if (pressed(in, plat::BTN_A)) runMainDebug(g.dbgMainSel);
  return true;
}
void updMenu(const Input& in) {
  if (pressed(in, plat::BTN_DOWN)) g.menuSel = (g.menuSel + 1) % 8;
  if (pressed(in, plat::BTN_UP)) g.menuSel = (g.menuSel + 7) % 8;
  if (pressed(in, plat::BTN_LEFT) && g.menuSel == 1) g.menuSel = 0;
  if (pressed(in, plat::BTN_RIGHT) && g.menuSel == 0) g.menuSel = 1;
  int act = -1;
  if (inR(in, 8, 28, 148, 34)) { g.menuSel = 0; act = 0; }
  if (inR(in, 164, 28, 148, 34)) { g.menuSel = 1; act = 1; }
  if (inR(in, 8, 70, 304, 30)) { g.menuSel = 2; act = 2; }
  if (inR(in, 8, 106, 304, 30)) { g.menuSel = 7; act = 7; }
  if (inR(in, 8, 150, 72, 48)) { g.menuSel = 3; act = 3; }
  if (inR(in, 88, 150, 72, 48)) { g.menuSel = 4; }
  if (inR(in, 168, 150, 72, 48)) { g.menuSel = 5; act = 5; }
  if (inR(in, 248, 150, 64, 48)) { g.menuSel = 6; act = 6; }
  if (pressed(in, plat::BTN_A)) act = g.menuSel;
  if (act < 0) return;
  audio::sfx(audio::SFX_UI);
  switch (act) {
    case 0: g.endless = false; g.allSeasons = false; g.level = 0; g.campaignSel = 0; g.campaignContentSel = 0; g.campaignContentFocus = false; go(SC_CAMPAIGNS); break;
    case 1: g.endless = true; g.allSeasons = true; g.endlessMask = 0x1F; g.endlessSel = 0; g.back = SC_MENU; go(SC_CAMPAIGN); break;
    case 2: g.catSel = 0; g.catScroll = 0; go(SC_CATALOG); break;
    case 3: g.achSel = 0; g.achScroll = 0; go(SC_ACHIEVEMENTS); break;
    case 4: toast("SHOP COMING SOON"); break;
    case 5: g.notesScroll = 0; go(SC_NOTES); break;
    case 6: g.optSel = 0; go(SC_OPTIONS); break;
    case 7: g.custRow = 0; go(SC_CUSTOM); break;
  }
}
void drawMenuTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  for (int i = 0; i < 5; ++i) gfx::discC(i % 8, 60.0f + i * 70, 190, 30.0f + 6 * (i & 1), 70);
  gfx::textC("ABYSSAL", 200, 26, C_GOLD, 4);
  gfx::textC("ECLIPSE", 200, 66, C_ECLIPSE, 4);
  gfx::textC(T(S_tagline), 200, 112, C_DIM);
  gfx::sprC(gfx::knight(), 200, 160, 1.0f);
  gfx::textC(T(S_version), 200, 224, C_BORDER);
  char coins[24]; snprintf(coins, sizeof coins, "%u", (unsigned)saveCoinBalance(g.save));
  (void)coins;
}
void drawMenuBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  button(8, 28, 148, 34, T(S_nav_campaign), g.menuSel == 0);
  button(164, 28, 148, 34, T(S_nav_endless), g.menuSel == 1);
  button(8, 70, 304, 30, T(S_nav_catalog), g.menuSel == 2);
  button(8, 106, 304, 30, T(S_nav_custom), g.menuSel == 4);
  const int bx[4] = {8, 88, 168, 248}; const int bw[4] = {72, 72, 72, 64};
  const int icons[4] = {IC_TROPHY, IC_COIN, IC_BOOK, IC_GEAR};
  const StrId labels[4] = {S_nav_achievements, S_nav_achievements, S_nav_notes, S_nav_options};
  for (int i = 0; i < 4; ++i) {
    const bool shop = i == 1, selected = g.menuSel == i + 3;
    const u32 border = shop ? (u32)C_BORDER : (selected ? (u32)C_GOLD : (u32)C_BORDER);
    const u32 fill = selected && !shop ? (u32)0x3A2F5C : (u32)C_PANEL;
    gfx::panel(bx[i], 150, bw[i], 48, fill, border);
    gfx::sprC(gfx::icon(icons[i]), bx[i] + bw[i] * 0.5f, 166, 1.25f, shop ? (u32)C_DIM : (u32)C_TEXT);
    textFitC(shop ? "SHOP" : T(labels[i]), bx[i] + bw[i] * 0.5f, 184, bw[i] - 8, shop ? (u32)C_DIM : (u32)C_TEXT, 1);
  }
  char coins[24]; snprintf(coins, sizeof coins, "%u", (unsigned)saveCoinBalance(g.save)); gfx::sprC(gfx::icon(IC_COIN), 137, 205, 1.0f); gfx::text(coins, 150, 200, C_GOLD);
  gfx::textC(T(S_hint_dpad), 160, 216, C_DIM);
}

// ---------------------------------------------------------------------------------------------- campaign / endless
void updCampaign(const Input& in) {
  if (!g.endless) {
    if (pressed(in, plat::BTN_L) || pressed(in, plat::BTN_LEFT) || inR(in, 0, 0, 60, 30)) { g.season = (g.season + 4) % 5; g.level = 0; audio::sfx(audio::SFX_UI); }
    if (pressed(in, plat::BTN_R) || pressed(in, plat::BTN_RIGHT) || inR(in, 260, 0, 60, 30)) { g.season = (g.season + 1) % 5; g.level = 0; audio::sfx(audio::SFX_UI); }
    if (pressed(in, plat::BTN_DOWN)) g.level = (g.level + 1) % 3;
    if (pressed(in, plat::BTN_UP)) g.level = (g.level + 2) % 3;
    bool act = pressed(in, plat::BTN_A);
    for (int i = 0; i < 3; ++i) if (inR(in, 20, 50 + i * 56, 280, 50)) { g.level = i; act = true; }
    if (pressed(in, plat::BTN_B)) { go(g.back); return; }
    if (act) { if (!levelUnlocked(g.save, g.season, g.level)) { toast(T(g.level == 0 ? S_sea_locked_hint : S_lvl_locked_hint)); return; } g.allSeasons = false; g.weaponSel = g.save.lastWeapon; go(SC_WEAPON); }
    return;
  }
  const int optionCount = 21; // 5x4 future-proof level grid plus rules popup
  if (pressed(in, plat::BTN_DOWN)) g.endlessSel = (g.endlessSel + 1) % optionCount;
  if (pressed(in, plat::BTN_UP)) g.endlessSel = (g.endlessSel + optionCount - 1) % optionCount;
  if (pressed(in, plat::BTN_Y)) {
    const bool clear = g.endlessMask == (u8)((1u << SEA_COUNT) - 1u);
    for (int i = 0; i < SEA_COUNT; ++i) g.endlessLevelMask[i] = clear ? 0 : 7;
    g.endlessMask = clear ? 0 : (u8)((1u << SEA_COUNT) - 1u); audio::sfx(audio::SFX_UI);
  }
  int adjust = pressed(in, plat::BTN_LEFT) || pressed(in, plat::BTN_L) ? -1 : (pressed(in, plat::BTN_RIGHT) || pressed(in, plat::BTN_R) ? 1 : 0);
  bool activate = pressed(in, plat::BTN_A);
  if (g.endlessRulesOpen) {
    if (pressed(in, plat::BTN_B) || pressed(in, plat::BTN_A)) { g.endlessRulesOpen = false; return; }
    if (inR(in, 30, 70, 260, 20)) g.endlessRandomSpells = !g.endlessRandomSpells;
    if (inR(in, 30, 92, 260, 20)) { static const int lives[] = {1, 2, 3, 5}; int idx = 0; for (int i = 0; i < 4; ++i) if (lives[i] == g.endlessLives) idx = i; g.endlessLives = lives[(idx + 1) % 4]; }
    return;
  }
  if (inR(in, 10, 22, 300, 90)) { int row = clampT((in.ty - 22) / 18, 0, 4); int col = clampT((in.tx - 10) / 75, 0, 3); g.endlessSel = row * 4 + col; activate = true; }
  if (inR(in, 10, 128, 300, 42)) { g.endlessSel = 20; activate = true; }
  if (pressed(in, plat::BTN_B)) { go(g.back); return; }
  if (!endlessSpellEligible(g.endlessSpell)) stepEndlessSpell(1);
  if (adjust || activate) {
    if (g.endlessSel < 20) {
      const int sea = g.endlessSel / 4, level = g.endlessSel % 4;
      if (level < 3 && activate) { g.endlessLevelMask[sea] ^= (u8)(1u << level); g.endlessMask = 0; for (int i = 0; i < SEA_COUNT; ++i) if (g.endlessLevelMask[i]) g.endlessMask |= (u8)(1u << i); }
    } else if (g.endlessSel == 20) g.endlessRulesOpen = true;
    audio::sfx(audio::SFX_UI);
  }
  if (pressed(in, plat::BTN_START) || inR(in, 194, 207, 116, 22)) {
    if (!g.endlessMask) { toast("SELECT AT LEAST ONE SEASON"); return; }
    g.allSeasons = __builtin_popcount((unsigned)g.endlessMask) > 1;
    for (g.season = 0; g.season < SEA_COUNT && !(g.endlessMask & (1u << g.season)); ++g.season) {}
    g.weaponSel = g.save.lastWeapon; go(SC_WEAPON);
  }
}
void drawCampaignTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  if (g.endless) {
    static const StrId names[SEA_COUNT] = {S_s_forest, S_s_desert, S_s_snow, S_s_mountain, S_s_eclipse};
    gfx::panel(20, 18, 360, 204, C_PANEL, C_GOLD);
    textFitC(T(S_endless_season_set), 200, 36, 350, C_GOLD, 2);
    int selectedCount = 0;
    for (int i = 0; i < SEA_COUNT; ++i) if (g.endlessMask & (1u << i)) ++selectedCount;
    char summary[48]; snprintf(summary, sizeof summary, "%d %s", selectedCount, T(selectedCount == 1 ? S_season : S_seasons));
    gfx::textC(summary, 200, 70, C_TEXT, 2);
    int row = 0;
    for (int i = 0; i < SEA_COUNT; ++i) if (g.endlessMask & (1u << i)) {
      gfx::sprC(gfx::icon(seasonIcon(i)), 95 + (row % 3) * 105, 120 + (row / 3) * 45, 1.6f);
      textFitC(T(names[i]), 95 + (row % 3) * 105, 140 + (row / 3) * 45, 92, SEASON_COLOR[i], 1);
      ++row;
    }
    textFitC(T(S_toggle_season), 100, 195, 150, C_DIM, 1);
    textFitC(T(S_select_clear), 290, 195, 190, C_DIM, 1);
    return;
  }
  const u32 c = SEASON_COLOR[g.season]; gfx::panel(20, 14, 360, 212, C_PANEL, c); gfx::spr(gfx::icon(seasonIcon(g.season)), 40, 34, 5.0f); textFit(T(SEASON_NAME[g.season]), 140, 40, 230, c, 3); gfx::text(T(S_elements), 140, 76, C_DIM);
  for (int k = 0; k < 2; ++k) { int e = SEASON_ELEM[g.season][k]; gfx::rect(140 + k * 110, 90, 10, 10, ELEM_COLOR[e]); gfx::text(T(ELEM_NAME[e]), 154 + k * 110, 91, C_TEXT); }
  gfx::textWrap(T(gimmick(g.season)), 36, 134, 330, C_TEXT, 1, 4); char b[32]; int done = 0; for (int l = 0; l < 3; ++l) if (g.save.levelCleared[g.season] & (1 << l)) ++done; snprintf(b, sizeof b, "%s %d/3", T(S_cleared), done); gfx::text(b, 36, 180, C_GOLD, 2);
  if (!seasonUnlocked(g.save, g.season)) { gfx::rect(20, 14, 360, 212, C_VOID, 190); gfx::sprC(gfx::icon(IC_LOCK), 200, 100, 4.0f); gfx::textC(T(S_season_locked), 200, 150, C_RED, 2); }
}
void drawCampaignBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  if (g.endless) {
    static const StrId names[SEA_COUNT] = {S_s_forest, S_s_desert, S_s_snow, S_s_mountain, S_s_eclipse};
    static const int icons[SEA_COUNT] = {IC_LEAF, IC_PYR, IC_SNOW, IC_PEAK, IC_ECL};
    header(T(S_endless_title));
    for (int sea = 0; sea < SEA_COUNT; ++sea) {
      const int y = 22 + sea * 18; gfx::spr(gfx::icon(icons[sea]), 12, y + 1, 0.7f);
      gfx::text(T(names[sea]), 27, y + 4, C_DIM);
      for (int level = 0; level < 4; ++level) {
        const int x = 95 + level * 52; const bool made = level < 3; const bool selected = made && (g.endlessLevelMask[sea] & (1u << level)); const int cell = sea * 4 + level;
        gfx::panel(x, y, 44, 15, cell == g.endlessSel ? (u32)0x3A2F5C : (u32)C_PANEL, cell == g.endlessSel ? (u32)C_GOLD : (u32)C_BORDER);
        gfx::textC(made ? (selected ? "X" : "-") : "LOCK", x + 22, y + 4, made ? (selected ? (u32)C_GREEN : (u32)C_DIM) : (u32)C_BORDER, 1);
      }
    }
    auto ruleRow = [](int y, int sel, const char* label, const char* value) {
      const int x = 30, w = 260;
      gfx::panel(x, y, w, 15, g.endlessSel == sel ? (u32)0x3A2F5C : (u32)C_PANEL, g.endlessSel == sel ? (u32)C_GOLD : (u32)C_BORDER);
      gfx::text(label, x + 7, y + 5, C_DIM); gfx::textR(value, x + w - 7, y + 5, C_GOLD);
    };
    button(10, 128, 300, 42, T(S_endless_rules), g.endlessSel == 20);
    button(194, 207, 116, 22, T(S_start), true);
    textFitC(T(S_endless_controls), 160, 190, 300, C_DIM, 1);
    if (g.endlessRulesOpen) { gfx::rect(18, 42, 284, 142, C_VOID, 245); gfx::panel(18, 42, 284, 142, C_PANEL, C_GOLD); textFitC(T(S_endless_rules), 160, 50, 268, C_GOLD, 1); ruleRow(76, 21, T(S_endless_random), g.endlessRandomSpells ? T(S_on) : T(S_off)); char lives[24]; snprintf(lives, sizeof lives, "%d", g.endlessLives); ruleRow(98, 22, T(S_lives), lives); gfx::textC(T(S_btn_back), 160, 166, C_DIM); }
    return;
  }
  gfx::rect(0, 0, BOT_W, 30, C_PANEL); gfx::text("<", 20, 10, C_GOLD, 2); gfx::text(">", 288, 10, C_GOLD, 2); gfx::textC(T(SEASON_NAME[g.season]), 160, 11, SEASON_COLOR[g.season], 1); char b[64];
  for (int i = 0; i < 3; ++i) { bool un = levelUnlocked(g.save, g.season, i), sel = g.level == i, done = g.save.levelCleared[g.season] & (1 << i); gfx::panel(20, 50 + i * 56, 280, 50, sel ? (u32)0x3A2F5C : (u32)C_PANEL, sel ? (u32)C_GOLD : (u32)C_BORDER); snprintf(b, sizeof b, "%d-%d", g.season + 1, i + 1); gfx::text(b, 30, 58 + i * 56, un ? (u32)C_GOLD : (u32)C_DIM, 2); gfx::text(T(LEVEL_NAME[g.season][i]), 76, 58 + i * 56, un ? C_TEXT : C_DIM); snprintf(b, sizeof b, "%s", Tn(S_waves_n, LEVELS[i].waves)); gfx::text(b, 76, 72 + i * 56, C_DIM); if (!un) gfx::sprC(gfx::icon(IC_LOCK), 280, 75 + i * 56, 1.5f); else if (done) gfx::sprC(gfx::icon(IC_TROPHY), 280, 75 + i * 56, 1.5f); }
  gfx::textC(T(S_btn_back), 160, 222, C_DIM);
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
  gfx::sprC(gfx::knight(), 130, 110, 1.5f);
  gfx::sprC(gfx::icon(weaponIcon(g.weaponSel)), 250, 110, 6.0f);
  textFitC(T(WEAPON_NAME[g.weaponSel]), 250, 168, 190, weaponUnlocked(g.weaponSel) ? (u32)C_TEXT : (u32)C_RED, 2);
  gfx::textC(T(S_choose_spell_in_game), 200, 210, C_DIM);
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
  static const StrId dmgW[W_COUNT] = {S_lvm_low, S_lvm_normal, S_lvm_high, S_lvm_very_high, S_lvm_normal, S_lvm_high, S_lvm_high, S_lvm_normal, S_lvm_normal, S_lvm_very_high};
  static const StrId atkW[W_COUNT] = {S_lvf_very_high, S_lvf_normal, S_lvf_low, S_lvf_very_low, S_lvf_low, S_lvf_fast, S_lvf_low, S_lvf_fast, S_lvf_very_fast, S_lvf_normal};
  static const StrId rngW[W_COUNT] = {S_lvm_low, S_lvm_very_high, S_lvm_normal, S_lvm_high, S_lvm_normal, S_lvm_high, S_lvm_very_high, S_lvm_low, S_lvm_high, S_lvm_high};
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
  gfx::sprC(gfx::knight(), 200, 126, 2.0f);
}
void drawCustomBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  const StrId lab[3] = {S_hat, S_face, S_color};
  const StrId val[3] = {CLOTH_NAME[clampT((int)g.save.hat, 0, 9)], FACE_NAME[g.save.face], COLOR_NAME[g.save.color]};
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
// Catalog entries include separator rows so each collection can be browsed by themed sections.
enum { CAT_COLS = 12, CAT_ROWS = 6, CAT_CELL = 24, CAT_TAB_COUNT = 5, CAT_TAB_W = 64, CAT_MAX_ENTRIES = 384 };
struct CatEntry { int id, section; bool header; };
static CatEntry g_catEntries[CAT_MAX_ENTRIES]; static int g_catCount = 0, g_catSections = 0;
static void catAdd(int id, int section, bool header = false) { if (g_catCount < CAT_MAX_ENTRIES) g_catEntries[g_catCount++] = CatEntry{id, section, header}; }
static void catAddSection(int section, const int* ids, int count) {
  while (g_catCount < CAT_MAX_ENTRIES && g_catCount % CAT_COLS) catAdd(-1, -1);
  if (g_catCount >= CAT_MAX_ENTRIES) return;
  catAdd(-1, section, true);
  while (g_catCount < CAT_MAX_ENTRIES && g_catCount % CAT_COLS) catAdd(-1, -1);
  for (int i = 0; i < count && g_catCount < CAT_MAX_ENTRIES; ++i) catAdd(ids[i], section);
  ++g_catSections;
}
static void buildCatalog() {
  g_catCount = 0; g_catSections = 0;
  if (g.catTab == 0) {
    int ids[NUM_SPELLS], n = 0;
    for (int el = 0; el < EL_COUNT; ++el) { n = 0; for (int i = 0; i < NUM_SPELLS; ++i) if (SPELLS[i].sc == SC_ELEMENT && SPELLS[i].el == el) ids[n++] = i; if (n) catAddSection(100 + el, ids, n); }
    n = 0; for (int i = 0; i < NUM_SPELLS; ++i) if (SPELLS[i].sc == SC_GLOBAL) ids[n++] = i; if (n) catAddSection(110, ids, n);
    for (int sea = 0; sea < SEA_COUNT; ++sea) { n = 0; for (int i = 0; i < NUM_SPELLS; ++i) if (SPELLS[i].sc == SC_SEASON && SPELLS[i].season == sea) ids[n++] = i; if (n) catAddSection(200 + sea, ids, n); }
  } else if (g.catTab == 1) {
    int ids[NUM_ENEMIES], n; for (int sea = 0; sea < SEA_COUNT; ++sea) { n = 0; for (int i = 0; i < NUM_ENEMIES; ++i) if (ENEMIES[i].season == sea) ids[n++] = i; if (n) catAddSection(300 + sea, ids, n); }
  } else if (g.catTab == 2) {
    int normal[] = {0,1,2,3,4}, rewards[] = {5,6,7,8,9}; catAddSection(400, normal, 5); catAddSection(401, rewards, 5);
  } else if (g.catTab == 3) {
    for (int i = 0; i < 5; ++i) catAdd(i, 500);
    g_catSections = 1;
  } else {
    int normal[] = {0,1,2,3,4,5,6,7}, rewards[] = {8,9}; catAddSection(600, normal, 8); catAddSection(601, rewards, 2);
  }
}
static int catCount() { buildCatalog(); return g_catCount; }
static bool catSelectable(int i) { return i >= 0 && i < g_catCount && !g_catEntries[i].header && g_catEntries[i].id >= 0; }
static int catFirstSelectable() { for (int i = 0; i < g_catCount; ++i) if (catSelectable(i)) return i; return 0; }
static int catMove(int from, int delta) { int i = from; do { i = (i + delta + g_catCount) % g_catCount; } while (!catSelectable(i) && i != from); return i; }
static int catSectionJump(int from, int dir) {
  static int sections[32]; int sectionCount = 0;
  for (int i = 0; i < g_catCount; ++i) if (catSelectable(i)) {
    const int section = g_catEntries[i].section; bool known = false;
    for (int j = 0; j < sectionCount; ++j) if (sections[j] == section) { known = true; break; }
    if (!known && sectionCount < (int)(sizeof sections / sizeof sections[0])) sections[sectionCount++] = section;
  }
  if (sectionCount <= 1) return from;
  const int current = catSelectable(from) ? g_catEntries[from].section : sections[0];
  int currentIndex = 0;
  for (int i = 0; i < sectionCount; ++i) if (sections[i] == current) { currentIndex = i; break; }
  const int target = (currentIndex + (dir < 0 ? sectionCount - 1 : 1)) % sectionCount;
  for (int i = 0; i < g_catCount; ++i)
    if (catSelectable(i) && g_catEntries[i].section == sections[target]) return i;
  return from;
}
static const char* catSectionLabel(int section) {
  static char label[48];
  if (section >= 100 && section < 106) snprintf(label, sizeof label, T(S_cat_element), T(ELEM_NAME[section - 100]));
  else if (section == 110) snprintf(label, sizeof label, "%s", T(S_cat_general_spells));
  else if (section >= 200 && section < 205) snprintf(label, sizeof label, T(S_cat_season), T(SEASON_NAME[section - 200]));
  else if (section >= 300 && section < 305) snprintf(label, sizeof label, T(S_cat_season), T(SEASON_NAME[section - 300]));
  else if (section == 400) snprintf(label, sizeof label, "%s", T(S_cat_normal_clothes));
  else if (section == 401) snprintf(label, sizeof label, "%s", T(S_cat_reward_clothes));
  else if (section == 600) snprintf(label, sizeof label, "%s", T(S_cat_normal_weapons));
  else if (section == 601) snprintf(label, sizeof label, "%s", T(S_cat_reward_weapons));
  else label[0] = 0;
  return label;
}
static bool catItemUnlocked(int id) {
  switch (g.catTab) { case 0: return spellFound(id); case 1: return enemyFound(id); case 2: return clothUnlocked(id); case 3: return musicUnlocked(id); default: return weaponUnlocked(id); }
}
void updCatalog(const Input& in) {
  catCount();
  if (!catSelectable(g.catSel)) g.catSel = catFirstSelectable();
  int old = g.catSel;
  int touchTab = -1; for (int i = 0; i < CAT_TAB_COUNT; ++i) if (inR(in, i * CAT_TAB_W, 0, CAT_TAB_W, 20)) { touchTab = i; break; }
  int nextTab = g.catTab; if (touchTab >= 0) nextTab = touchTab; else if (pressed(in, plat::BTN_L)) nextTab = (g.catTab + CAT_TAB_COUNT - 1) % CAT_TAB_COUNT; else if (pressed(in, plat::BTN_R)) nextTab = (g.catTab + 1) % CAT_TAB_COUNT;
  if (nextTab != g.catTab) { g.catTab = nextTab; g.catSel = 0; g.catScroll = 0; g.loreScroll = 0; audio::sfx(audio::SFX_UI); }
  const int count = catCount(); if (!catSelectable(g.catSel)) g.catSel = catFirstSelectable();
  if (pressed(in, plat::BTN_RIGHT)) g.catSel = catMove(g.catSel, 1);
  if (pressed(in, plat::BTN_LEFT)) g.catSel = catMove(g.catSel, -1);
  if (pressed(in, plat::BTN_DOWN)) { int target = g.catSel + CAT_COLS; while (target < count && !catSelectable(target)) ++target; if (target < count) g.catSel = target; }
  if (pressed(in, plat::BTN_UP)) { int target = g.catSel - CAT_COLS; while (target >= 0 && !catSelectable(target)) --target; if (target >= 0) g.catSel = target; }
  for (int i = 0; i < count; ++i) { int r = i / CAT_COLS - g.catScroll; if (catSelectable(i) && r >= 0 && r < CAT_ROWS && inR(in, 16 + (i % CAT_COLS) * CAT_CELL, 24 + r * CAT_CELL, CAT_CELL, CAT_CELL)) g.catSel = i; }
  // Touch the footer arrows to skip directly to the previous/next separation.
  if (inR(in, 4, 204, 88, 28)) g.catSel = catSectionJump(g.catSel, -1);
  if (inR(in, 228, 204, 88, 28)) g.catSel = catSectionJump(g.catSel, 1);
  if (pressed(in, plat::BTN_X)) g.loreScroll++;
  if (pressed(in, plat::BTN_Y)) g.loreScroll = maxT(0, g.loreScroll - 1);
  if (g.catTab == 3 && (pressed(in, plat::BTN_A) || inR(in, 16 + g.catSel * CAT_CELL, 24, CAT_CELL, CAT_CELL)) && musicUnlocked(g_catEntries[g.catSel].id)) { audio::musicPreview(g_catEntries[g.catSel].id); audio::sfx(audio::SFX_UI); }
  if (g.catSel != old) g.loreScroll = 0;
  int row = g.catSel / CAT_COLS;
  if (row < g.catScroll) g.catScroll = row;
  if (row >= g.catScroll + CAT_ROWS) g.catScroll = row - CAT_ROWS + 1;
  if (pressed(in, plat::BTN_B)) { g.secretCatalog = false; g.droneClicks = 0; go(SC_MENU); }
}
void drawCatalogTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID); buildCatalog(); const int i = g_catEntries[g.catSel].id; char b[96];
  if (g.catTab == 0) { const SpellDef& d = SPELLS[i]; if (!spellFound(i)) { gfx::sprC(gfx::icon(IC_LOCK), 200, 90, 4.0f); gfx::textC(T(S_not_found), 200, 140, C_DIM, 2); return; } gfx::panel(8, 8, 384, 224, C_PANEL, RARITY_COLOR[d.rar]); gfx::spr(gfx::spellIcon(d.ty, d.el, d.act), 20, 20, 3.0f); textFit(spellName(i), 76, 20, 316, RARITY_COLOR[d.rar], 2); snprintf(b, sizeof b, "%s / %s / %s / %s", T(RARITY_NAME[d.rar]), T(ELEM_NAME[d.el]), T(TYPE_NAME[d.ty]), T(ACT_NAME[d.act])); gfx::text(b, 76, 42, C_DIM); gfx::textWrap(spellDesc(i), 20, 76, 360, C_TEXT, 1, 3); gfx::textWrap(spellStats(i), 20, 106, 360, C_GOLD, 1, 2); snprintf(b, sizeof b, "%s %d", T(S_found), g.save.spellFound[i]); gfx::text(b, 20, 214, C_DIM); gfx::text(T(S_lore), 20, 128, C_EPIC); gfx::textWrap(spellLore(i), 20, 140, 360, C_DIM, 1, 7, g.loreScroll);
  } else if (g.catTab == 1) { const EnemyDef& d = ENEMIES[i]; if (!enemyFound(i)) { gfx::sprC(gfx::icon(IC_LOCK), 200, 90, 4.0f); gfx::textC(T(S_not_found), 200, 140, C_DIM, 2); return; } gfx::panel(8, 8, 384, 224, C_PANEL, SEASON_COLOR[d.season]); gfx::spr(gfx::enemy(i), 20, 20, 3.0f); textFit(enemyName(i), 76, 20, 316, SEASON_COLOR[d.season], 2); snprintf(b, sizeof b, "%s %d  %s %s  %s %s", T(S_hp), d.hp, T(S_spd), T(speedWord(d.spd)), T(S_range), T(rangeWord(d.range))); gfx::text(b, 76, 42, C_DIM); gfx::textWrap(enemyDesc(i), 20, 76, 360, C_TEXT, 1, 3); snprintf(b, sizeof b, "%s: %s", T(S_special), enemySpecial(i)); gfx::textWrap(b, 20, 106, 360, C_GOLD, 1, 2); gfx::text(T(S_lore), 20, 128, C_EPIC); gfx::textWrap(enemyLore(i), 20, 140, 360, C_DIM, 1, 7, g.loreScroll); snprintf(b, sizeof b, "%s %d", T(S_kills_s), g.save.enemyKills[i]); gfx::text(b, 20, 214, C_DIM);
  } else if (g.catTab == 2) { gfx::panel(8, 8, 384, 224, C_PANEL, C_GOLD); gfx::sprC(gfx::icon(IC_CHEST), 70, 74, 5.0f); gfx::text(T(CLOTH_NAME[i]), 120, 28, C_GOLD, 2); if (!clothUnlocked(i)) { gfx::sprC(gfx::icon(IC_LOCK), 200, 96, 3.0f); gfx::textC(T(S_not_found), 200, 144, C_DIM, 2); } else { gfx::sprC(gfx::clothingPreview(i), 200, 100, 2.0f); gfx::textC(T(S_ach_unlocked), 200, 164, C_GREEN); }
  } else if (g.catTab == 3) { gfx::panel(8, 8, 384, 224, C_PANEL, SEASON_COLOR[i]); gfx::sprC(gfx::icon(seasonIcon(i)), 60, 70, 4.0f); gfx::text(T(MUSIC_NAME[i]), 100, 28, SEASON_COLOR[i], 2); if (!musicUnlocked(i)) { gfx::sprC(gfx::icon(IC_LOCK), 200, 92, 3.0f); gfx::textWrap(T(S_music_locked), 100, 140, 260, C_DIM, 1, 3); } else { gfx::textC(T(S_music_play), 200, 156, C_GREEN); gfx::textC(T(S_music_stop), 200, 180, C_DIM); }
  } else { const bal::WeaponStat& w = bal::WEAPON[i]; const bool unlocked = weaponUnlocked(i); const float range = i == W_KATANA ? bal::KATANA_RANGE : w.range; gfx::panel(8, 8, 384, 224, C_PANEL, unlocked ? C_GOLD : C_BORDER); gfx::sprC(gfx::icon(weaponIcon(i)), 58, 78, 4.0f, unlocked ? 0xFFFFFFFFu : HEX(C_BORDER), unlocked ? plat::BLIT_PLAIN : plat::BLIT_TINT); textFitC(T(WEAPON_NAME[i]), 112, 22, 270, unlocked ? (u32)C_GOLD : (u32)C_DIM, 2); gfx::textWrap(T(WEAPON_FIRE[i]), 108, 52, 270, C_TEXT, 1, 4); snprintf(b, sizeof b, "%s: %.1f", T(S_dmg), w.dmg); gfx::text(b, 108, 112, C_GOLD); snprintf(b, sizeof b, "%s: %.2f s", T(S_atk_speed), w.interval); gfx::text(b, 108, 130, C_TEXT); snprintf(b, sizeof b, "%s: %.0f px", T(S_range), range); gfx::text(b, 108, 148, C_TEXT); if (!unlocked) { gfx::sprC(gfx::icon(IC_LOCK), 58, 166, 1.5f); gfx::text(T(S_locked), 108, 176, C_RED); } }
}
void drawCatalogBot() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID); buildCatalog(); const StrId tabName[CAT_TAB_COUNT] = {S_sub_spells, S_sub_bestiary, S_cat_clothes, S_cat_music, S_cat_weapons}; for (int i = 0; i < CAT_TAB_COUNT; ++i) { const float x = (float)(i * CAT_TAB_W); const bool active = i == g.catTab; gfx::panel(x + 1, 1, CAT_TAB_W - 2, 19, active ? (u32)0x3A2F5C : (u32)C_PANEL, active ? (u32)C_GOLD : (u32)C_BORDER); textFitC(T(tabName[i]), x + CAT_TAB_W * 0.5f, 6, CAT_TAB_W - 6, active ? (u32)C_GOLD : (u32)C_DIM, 1); }
  const int n = g_catCount; for (int i = g.catScroll * CAT_COLS; i < n && i < (g.catScroll + CAT_ROWS) * CAT_COLS; ++i) { int r = i / CAT_COLS - g.catScroll, c = i % CAT_COLS; float x = 16.0f + c * CAT_CELL, y = 24.0f + r * CAT_CELL; if (g_catEntries[i].header) { gfx::text(catSectionLabel(g_catEntries[i].section), 16, y + 5, C_GOLD); gfx::rect(16, y + 19, 296, 1, C_BORDER); continue; } if (!catSelectable(i)) continue; if (i == g.catSel) gfx::rect(x - 1, y - 1, CAT_CELL - 2, CAT_CELL - 2, C_GOLD); const int id = g_catEntries[i].id; if (g.catTab == 0) { if (spellFound(id)) gfx::spr(gfx::spellIcon(SPELLS[id].ty, SPELLS[id].el, SPELLS[id].act), x + 2, y + 2, 1.0f); else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT); } else if (g.catTab == 1) { if (enemyFound(id)) gfx::spr(gfx::enemy(id), x + 2, y + 2, 1.0f); else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT); } else if (g.catTab == 2) { if (clothUnlocked(id)) gfx::spr(gfx::icon(IC_CHEST), x + 2, y + 2, 1.0f); else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT); } else if (g.catTab == 3) { if (musicUnlocked(id)) gfx::spr(gfx::icon(seasonIcon(id)), x + 2, y + 2, 1.0f); else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT); } else { if (weaponUnlocked(id)) gfx::spr(gfx::icon(weaponIcon(id)), x + 2, y + 2, 1.0f); else gfx::spr(gfx::icon(IC_LOCK), x + 2, y + 2, 1.0f, HEX(C_BORDER), plat::BLIT_TINT); } }
  int found = 0, total = 0; for (int i = 0; i < n; ++i) if (catSelectable(i)) { ++total; found += catItemUnlocked(g_catEntries[i].id) ? 1 : 0; } char b[48]; snprintf(b, sizeof b, "%d/%d", found, total); gfx::textC(b, 160, 176, C_DIM); gfx::panel(4, 204, 88, 20, C_PANEL, C_BORDER); gfx::textC(T(S_cat_prev), 48, 210, C_DIM); gfx::panel(228, 204, 88, 20, C_PANEL, C_BORDER); gfx::textC(T(S_cat_next), 272, 210, C_DIM); gfx::textC(g.catTab == 3 ? T(S_music_play) : T(S_cat_controls), 160, 222, C_DIM);
}
// ---------------------------------------------------------------------------------------------- achievements / lifetime stats
static const StrId ACH_NAME[ACH_COUNT] = {
  S_ach_season_forest, S_ach_season_desert, S_ach_season_snow, S_ach_season_mountain, S_ach_season_eclipse,
  S_ach_kills_1000, S_ach_kills_10000, S_ach_kills_100000,
  S_ach_catalog_forest, S_ach_catalog_desert, S_ach_catalog_snow, S_ach_catalog_mountain, S_ach_catalog_eclipse,
  S_ach_full_catalog, S_ach_little_dummy, S_ach_abyss_veteran, S_ach_touch_grass,
  S_ach_elementalist, S_ach_start, S_ach_infinity, S_ach_unstoppable,
};
static const StrId ACH_DESC[ACH_COUNT] = {
  S_ach_season_forest_desc, S_ach_season_desert_desc, S_ach_season_snow_desc, S_ach_season_mountain_desc, S_ach_season_eclipse_desc,
  S_ach_kills_1000_desc, S_ach_kills_10000_desc, S_ach_kills_100000_desc,
  S_ach_catalog_forest_desc, S_ach_catalog_desert_desc, S_ach_catalog_snow_desc, S_ach_catalog_mountain_desc, S_ach_catalog_eclipse_desc,
  S_ach_full_catalog_desc, S_ach_little_dummy_desc, S_ach_abyss_veteran_desc, S_ach_touch_grass_desc,
  S_ach_elementalist_desc, S_ach_start_desc, S_ach_infinity_desc, S_ach_unstoppable_desc,
};
void updAchievements(const Input& in) {
  if (pressed(in, plat::BTN_R)) g.achSel = minT(ACH_COUNT - 1, g.achSel + 6);
  if (pressed(in, plat::BTN_L)) g.achSel = maxT(0, g.achSel - 6);
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
  int rewardIcon = -1; const char* reward = nullptr;
  if (id >= ACH_SEASON_FOREST && id <= ACH_SEASON_ECLIPSE) { rewardIcon = seasonIcon(id - ACH_SEASON_FOREST); reward = T(MUSIC_NAME[id - ACH_SEASON_FOREST]); }
  else if (id == ACH_LITTLE_DUMMY) { rewardIcon = IC_CHEST; reward = T(S_cloth_nightwarden); }
  else if (id == ACH_ABYSS_VETERAN) { rewardIcon = IC_CHEST; reward = T(S_cloth_grinmask); }
  else if (id == ACH_KILLS_1000) { rewardIcon = IC_CHEST; reward = T(S_cloth_redcap); }
  else if (id == ACH_FULL_CATALOG) { rewardIcon = IC_CHEST; reward = T(S_cloth_arcwarden); }
  else if (id == ACH_ELEMENTALIST) { rewardIcon = IC_CHEST; reward = T(S_cloth_elementalist); }
  else if (id == ACH_UNSTOPPABLE) { rewardIcon = IC_INFINITY; reward = T(S_w_infinity_glove); }
  if (rewardIcon >= 0) {
    gfx::panel(8, 210, 384, 28, C_PANEL, achievementUnlocked(g.save, id) ? (u32)C_GREEN : (u32)C_BORDER);
    gfx::spr(gfx::icon(rewardIcon), 14, 216, 1.0f);
    gfx::text("REWARD", 36, 213, C_DIM, 1); textFit(reward, 88, 213, 292, achievementUnlocked(g.save, id) ? (u32)C_GREEN : (u32)C_GOLD, 1);
    snprintf(val, sizeof val, "%s  %lu/%lu", T(achievementUnlocked(g.save, id) ? S_ach_unlocked : S_ach_locked), (unsigned long)current, (unsigned long)target);
    gfx::text(val, 36, 224, C_TEXT, 1);
  } else {
    snprintf(val, sizeof val, "%s  %lu/%lu", T(achievementUnlocked(g.save, id) ? S_ach_unlocked : S_ach_locked), (unsigned long)current, (unsigned long)target);
    gfx::text(val, 12, 228, C_DIM);
  }
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
  const int n = 8;
  if (pressed(in, plat::BTN_DOWN)) g.optSel = (g.optSel + 1) % n;
  if (pressed(in, plat::BTN_UP)) g.optSel = (g.optSel + n - 1) % n;
  int d = 0; bool act = pressed(in, plat::BTN_A);
  if (pressed(in, plat::BTN_RIGHT)) d = 1;
  if (pressed(in, plat::BTN_LEFT)) d = -1;
  for (int i = 0; i < n; ++i) if (inR(in, 20, 8 + i * 26, 280, 24)) { g.optSel = i; act = true; d = (in.tx < 110 ? -1 : (in.tx > 210 ? 1 : 0)); }
  if (pressed(in, plat::BTN_B)) { persist(); go(SC_MENU); return; }
  switch (g.optSel) {
    case 0: if (act || d) { g.save.lang = (u8)((g.save.lang + 1) % LANG_COUNT); applyOptions(); audio::sfx(audio::SFX_UI); } break;
    case 1: if (d || act) { int v = g.save.musicVol + (d ? d * 10 : 10); if (v > 100) v = 0; g.save.musicVol = (u8)clampT(v, 0, 100); applyOptions(); } break;
    case 2: if (d || act) { int v = g.save.sfxVol + (d ? d * 10 : 10); if (v > 100) v = 0; g.save.sfxVol = (u8)clampT(v, 0, 100); applyOptions(); audio::sfx(audio::SFX_PICKUP); } break;
    case 3: if (act || d) { g.save.fps30 ^= 1; applyOptions(); } break;
    case 4: if (act || d) { g.save.showFps ^= 1; } break;
    case 5: if (act || d) { g.save.shakeOff ^= 1; persist(); } break;
    case 6: if (act) go(SC_CREDITS); break;
    case 7: if (act) { persist(); go(SC_MENU); } break;
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
  const char* labels[8];
  labels[0] = g.save.lang == LANG_EN ? T(S_lang_en) : T(S_lang_pt);
  snprintf(b, sizeof b, "%s: %d%%", T(S_volume_music), g.save.musicVol); static char b1[64]; snprintf(b1, sizeof b1, "%s", b);
  snprintf(b, sizeof b, "%s: %d%%", T(S_volume_sfx), g.save.sfxVol); static char b2[64]; snprintf(b2, sizeof b2, "%s", b);
  snprintf(b, sizeof b, "%s: %s", T(S_opt_fps), g.save.fps30 ? T(S_on) : T(S_off)); static char b3[64]; snprintf(b3, sizeof b3, "%s", b);
  labels[1] = b1; labels[2] = b2; labels[3] = b3;
  snprintf(b, sizeof b, "%s: %s", T(S_opt_showfps), g.save.showFps ? T(S_on) : T(S_off)); static char b4[64]; snprintf(b4, sizeof b4, "%s", b);
  labels[4] = b4;
  static char b5[64]; snprintf(b5, sizeof b5, "SCREEN SHAKE: %s", g.save.shakeOff ? T(S_off) : T(S_on));
  labels[5] = b5; labels[6] = T(S_credits); labels[7] = T(S_back_menu);
  for (int i = 0; i < 8; ++i) button(20, 8 + i * 26, 280, 24, labels[i], g.optSel == i);
  gfx::textC(T(S_btn_back), 160, 222, C_DIM);
}
void drawCreditsTop() {
  gfx::rect(0, 0, TOP_W, TOP_H, C_VOID);
  gfx::textC(T(S_credits), 200, 14, C_GOLD, 3);
  gfx::textWrap("Game design, Some Arts and programming: MrJuaumBR", 24, 48, 352, C_TEXT, 1, 2);
  gfx::textWrap("AI assistants: Claude, DeepSeek, Manus", 24, 76, 352, C_TEXT, 1, 2);
  gfx::textWrap("Some Arts come from: disven (https://disven.itch.io/pixel-swords-assets-16x16), scrython (https://scrython.itch.io/pixelart-guns)", 24, 104, 352, C_TEXT, 1, 3);
  gfx::textWrap("AI Art Concepts: https://perchance.org/ai-pixel-art-generator", 24, 148, 352, C_TEXT, 1, 2);
  gfx::textC("CREDITS: README.md#Credits", 200, 216, C_DIM, 1);
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
  int progress = 0, target = 0; float cooldown = 0.0f, cooldownMax = 0.0f;
  if (simKillTriggerInfo(s, slot, progress, target, cooldown, cooldownMax)) {
    snprintf(b, sizeof b, "%d/%dK", progress, target); gfx::textC(b, x + 36, y + 3, C_GOLD);
    if (cooldownMax > 0.0f) {
      const float ready = cooldown <= 0.0f ? 1.0f : clampT(1.0f - cooldown / cooldownMax, 0.0f, 1.0f);
      gfx::rect(x + 3, y + 10, 66, 2, C_VOID); gfx::rect(x + 3, y + 10, 66 * ready, 2, ready >= 1.0f ? C_GREEN : C_EPIC);
    }
    return;
  }
  const int triggerEvery = simSpellTriggerInterval(s, slot);
  if (triggerEvery > 0) { snprintf(b, sizeof b, "%d/%d", (int)(s.p.throws % triggerEvery), triggerEvery); gfx::textC(b, x + 36, y + 3, C_GOLD); return; }
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

enum { RUN_INFO_ROW_COUNT = 27, RUN_INFO_VISIBLE_ROWS = 11 };
static void drawRunSection(StrId title, float y) {
  gfx::text(T(title), 8, y, C_GOLD);
  gfx::rect(8, y + 11, 300, 1, C_BORDER);
}
static void drawRunValue(StrId label, const char* value, float y, u32 valueColor = C_TEXT) {
  textFit(T(label), 8, y, 178, C_TEXT, 1);
  gfx::textR(value, 306, y, valueColor);
}
static void drawRunStat(StrId label, float base, float current, const char* unit, float y) {
  char value[24], change[40];
  const float delta = current - base;
  snprintf(value, sizeof value, "%.1f%s", current, unit);
  if (base > 0.001f) snprintf(change, sizeof change, "%+.1f%s (%+.0f%%)", delta, unit, delta * 100.0f / base);
  else snprintf(change, sizeof change, "%+.1f%s", delta, unit);
  const u32 changeColor = delta > 0.001f ? (u32)C_GREEN : (delta < -0.001f ? (u32)C_RED : (u32)C_DIM);
  textFit(T(label), 8, y, 112, C_TEXT, 1);
  gfx::textR(value, 194, y, C_TEXT);
  gfx::textR(change, 307, y, changeColor);
}

void drawGameBot() {
  Sim& s = g_sim;
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
  char b[96];
  if (g.debug) {
    if (g.dbgSpellPicker) {
      const int pageSize = 20, cols = 4;
      const int page = g.dbgSpellSel / pageSize, start = page * pageSize;
      const int end = minT((int)NUM_SPELLS, start + pageSize);
      snprintf(b, sizeof b, "SPELL PICKER  %d/%d", page + 1, (NUM_SPELLS + pageSize - 1) / pageSize);
      header(b, C_RED);
      for (int id = start; id < end; ++id) {
        const int j = id - start, x = 4 + (j % cols) * 78, y = 26 + (j / cols) * 23;
        const bool selected = id == g.dbgSpellSel;
        gfx::panel(x, y, 75, 21, selected ? (u32)0x3A2F5C : (u32)C_PANEL,
                   selected ? (u32)C_GOLD : (u32)C_BORDER);
        char label[32]; compactSlotLabel(spellName(id), label, sizeof label);
        textFitC(label, x + 37, y + 3, 70, RARITY_COLOR[SPELLS[id].rar], 1);
        snprintf(b, sizeof b, "#%03d  R%d", id + 1, SPELLS[id].maxLevel);
        gfx::textC(b, x + 37, y + 12, C_DIM, 1);
      }
      const SpellDef& selected = SPELLS[g.dbgSpellSel];
      gfx::panel(4, 145, 312, 66, C_PANEL, C_BORDER);
      snprintf(b, sizeof b, "%s  R1/%d", spellName(g.dbgSpellSel), selected.maxLevel);
      textFitC(b, 160, 149, 296, RARITY_COLOR[selected.rar], 1);
      gfx::textWrap(spellDesc(g.dbgSpellSel), 9, 161, 302, C_TEXT, 1, 2);
      gfx::textWrap(spellStats(g.dbgSpellSel), 9, 185, 302, C_GOLD, 1, 2);
      gfx::textC("A / TAP FOOTER: GRANT OR UPGRADE   B: BACK   L/R: PAGE", 160, 220, C_DIM, 1);
      return;
    }
    header("DEVELOPMENT WATCH  /  IN-GAME", C_RED);
    static const char* labels[14] = {"END WAVE", "SPAWN BOSS", "FORCE SPELL OFFER", "CLEAR RUN SPELLS", "SIMULATE DEATH", "GIVE LEVEL", "TOGGLE GOD MODE", "TOGGLE ALL CONTENT", "RESET RUN", "SPELL PICKER", "SPAWN GOLEM", "COMPLETE LEVEL", "+1 MINUTE", "FORCE ACHIEVEMENTS"};
    const int visible = 7, first = clampT(g.dbgSel - visible / 2, 0, 14 - visible);
    for (int row = 0; row < visible; ++row) {
      const int idx = first + row; const bool selected = idx == g.dbgSel;
      button(12, 34 + row * 24, 296, 22, labels[idx], selected);
      if (idx == 6 && s.godMode) gfx::textR("ON", 299, 40 + row * 24, C_GREEN);
      if (idx == 7 && g.save.unlockAll) gfx::textR("ON", 299, 40 + row * 24, C_GREEN);
    }
    gfx::textC("UP/DOWN SCROLL  A RUN  B/SELECT CLOSE", 160, 218, C_DIM, 1);
    return;
  }
  if (g.paused) {
    header(T(S_paused));
    const char* ptabs[3] = {"MENU", "RUN", "SPELLS"};
    for (int i = 0; i < 3; ++i) { gfx::panel(4 + i * 104, 22, 100, 18, g.pauseTab == i ? (u32)0x3A2F5C : (u32)C_PANEL, g.pauseTab == i ? (u32)C_GOLD : (u32)C_BORDER); gfx::textC(ptabs[i], 54 + i * 104, 27, g.pauseTab == i ? (u32)C_GOLD : (u32)C_DIM, 1); }
    if (g.pauseTab == 1) {
      // Pause RUN uses the same full stats list, columns and scroll position as the live RUN tab.
      g.paused = false; const int savedTab = g.tab; g.tab = 2;
      // render the shared RUN body below the tabs; pause state is restored before returning.
      g.paused = true; g.tab = savedTab;
      const int scroll = clampT(g.runInfoScroll, 0, RUN_INFO_ROW_COUNT - RUN_INFO_VISIBLE_ROWS);
      gfx::text(T(S_run_info), 8, 46, C_GOLD); gfx::textR(T(S_run_value), 194, 46, C_DIM); gfx::textR(T(S_run_change), 307, 46, C_DIM);
      const bal::WeaponStat& w = bal::WEAPON[s.weapon]; const float attackRate = s.m.atkSpd / maxT(0.01f, w.interval * s.m.cdMul); const float moveSpeed = s.m.move * (1.0f + s.p.moveBonus / 100.0f);
      const float critPower = (bal::CRIT_MUL + (s.m.critDmg + s.m.critBonus) / 100.0f) * 100.0f;
      for (int visible = 0; visible < 9; ++visible) { const int row = scroll + visible; const float y = 58.0f + visible * 16.0f; char value[40];
        switch(row) {
          case 0: drawRunSection(S_run_section,y); break;
          case 1: drawRunValue(S_season,T(SEASON_NAME[s.season]),y,SEASON_COLOR[s.season]); break;
          case 2: snprintf(value,sizeof value,"%s",s.endless?T(S_nav_endless):"CAMPAIGN"); drawRunValue(S_level,value,y); break;
          case 3: snprintf(value,sizeof value,"%d",s.wave); drawRunValue(S_wave,value,y); break;
          case 4: snprintf(value,sizeof value,"%d",s.totalKills); drawRunValue(S_enemies_killed,value,y); break;
          case 5: { int sec=(int)s.runT; snprintf(value,sizeof value,"%d:%02d",sec/60,sec%60); drawRunValue(S_time,value,y); } break;
          case 6: snprintf(value,sizeof value,"%d",g.save.bestWave[s.season]); drawRunValue(S_best_wave,value,y,C_GOLD); break;
          case 7: drawRunSection(S_character_section,y); break;
          case 8: case 9: case 10: case 11: case 12: { int st=row-8; drawRunStat(STAT_NAME[st],(float)bal::BASE_STAT[st],(float)statValue(s,st),"",y); } break;
          case 13: drawRunSection(S_combat_section,y); break;
          case 14: drawRunStat(S_stat_damage,w.dmg,(w.dmg+s.m.flat)*s.m.dmgMul*s.m.dealt,"",y); break;
          case 15: drawRunStat(S_stat_attack_rate,1.0f/w.interval,attackRate,"/s",y); break;
          case 16: drawRunStat(S_stat_range,w.range,w.range*s.m.rangeMul,"px",y); break;
          case 17: drawRunStat(S_stat_move_speed,bal::PLAYER_SPEED,moveSpeed,"px/s",y); break;
          case 18: drawRunStat(S_stat_crit_chance,bal::BASE_CRIT,s.m.crit,"%",y); break;
          case 19: drawRunStat(S_stat_crit_power,bal::CRIT_MUL*100.0f,critPower,"%",y); break;
          case 20: drawRunSection(S_other_section,y); break;
          case 21: drawRunStat(S_stat_max_hp,bal::BASE_HP,s.p.maxhp,"",y); break;
          case 22: drawRunStat(S_stat_damage_reduction,0.0f,(1.0f-s.m.taken)*100.0f,"%",y); break;
          case 23: drawRunStat(S_stat_area,100.0f,s.m.aoeMul*100.0f,"%",y); break;
          case 24: drawRunStat(S_stat_projectile_speed,100.0f,s.m.projSpd*100.0f,"%",y); break;
          case 25: drawRunStat(S_stat_xp_gain,100.0f,s.m.expMul*100.0f,"%",y); break;
          case 26: drawRunStat(S_stat_pierce,(float)w.pierce,(float)(w.pierce+s.m.pierce),"",y); break;
        }
      }
      gfx::rect(314, 58, 2, 144, C_BORDER);
      const int maxPauseScroll = 27 - 9; const int thumbH = 42;
      const int thumbY = 58 + (144 - thumbH) * scroll / maxPauseScroll;
      gfx::rect(314, thumbY, 2, thumbH, C_GOLD);
      gfx::textC(T(S_run_info_controls),160,222,C_BORDER);
    } else if (g.pauseTab == 2) { for (int i = 0; i < s.nSlots && i < bal::MAX_SLOTS; ++i) { const char* nm; const char* ds; const char* st; u32 col; const gfx::Spr* ic; slotText(s, i, &nm, &ds, &st, &col, &ic); gfx::panel(8 + (i % 4) * 76, 52 + (i / 4) * 36, 72, 32, C_PANEL, C_BORDER); gfx::spr(*ic, 12 + (i % 4) * 76, 57 + (i / 4) * 36, 1.0f); textFitC(nm, 48 + (i % 4) * 76, 62 + (i / 4) * 36, 50, col, 1); } }
    else { button(12, 112, 296, 24, T(S_resume), g.pauseSel == 0); button(12, 142, 296, 24, T(S_quit_menu), g.pauseSel == 1); }
    gfx::textC(T(S_run_info_controls), 160, 222, C_DIM, 1); return;
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
    if (!s.randomSpells) { snprintf(b, sizeof b, "L/R: PAGE %d/%d  A: CHOOSE", s.offerPage + 1, maxT(1, (s.offerPoolCount + 2) / 3)); gfx::textC(b, 160, 226, C_DIM, 1); }
    else gfx::textC(T(S_choose_one), 160, 226, C_DIM);
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
        if (s.endless) {
          gfx::textWrap(ds, 8, 174, 304, C_TEXT, 1, 2);
          textFit(b, 8, 194, 304, C_GOLD, 1);
          textFit(st, 8, 205, 304, C_DIM, 1);
          const int reward = simSacrificeReward(s, g.slotSel);
          gfx::panel(8, 218, 304, 20, reward > 0 ? (u32)0x3A2F5C : (u32)C_PANEL,
                     reward > 0 ? (u32)C_GREEN : (u32)C_BORDER);
          if (reward > 0) {
            const float progress = clampT(g.sacrificeHoldT / 3.0f, 0.0f, 1.0f);
            gfx::rect(9, 219, 302.0f * progress, 18, C_GREEN, 85);
            gfx::textC(Tn(S_sacrifice_action, reward), 160, 223, C_TEXT);
          }
          else gfx::textC(T(s.sacrificedThisWave ? S_sacrifice_used : S_sacrifice_locked), 160, 223, C_DIM);
        } else {
          gfx::textWrap(b, 8, 202, 304, C_GOLD, 1, 1);
          snprintf(b, sizeof b, "%s: %s", T(S_spell_base_stats), st);
          gfx::textWrap(b, 8, 212, 304, C_DIM, 1, 3);
          gfx::textWrap(ds, 8, 172, 304, C_TEXT, 1, 3);
        }
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
      const int value = statValue(s, i), delta = value - bal::BASE_STAT[i];
      gfx::text(T(STAT_NAME[i]), 34, y + 5, C_TEXT);
      snprintf(b, sizeof b, "%d", value); gfx::textR(b, 256, y + 5, C_TEXT);
      snprintf(b, sizeof b, "%+d", delta);
      gfx::textR(b, 306, y + 5, delta > 0 ? (u32)C_GREEN : (delta < 0 ? (u32)C_RED : (u32)C_DIM));
      gfx::text(T(STAT_DESC[i]), 34, y + 17, C_DIM);
    }
    snprintf(b, sizeof b, "%s %d   %s %d", T(S_plevel), s.p.level, T(S_pts), s.points); gfx::text(b, 8, 192, C_GOLD);
    snprintf(b, sizeof b, "%s %d/%d", T(S_hp), (int)s.p.hp, (int)s.p.maxhp); gfx::text(b, 8, 206, C_RED);
  } else {
    const int scroll = clampT(g.runInfoScroll, 0, RUN_INFO_ROW_COUNT - RUN_INFO_VISIBLE_ROWS);
    g.runInfoScroll = scroll;
    gfx::text(T(S_run_info), 8, 26, C_GOLD);
    gfx::textR(T(S_run_value), 194, 36, C_DIM); gfx::textR(T(S_run_change), 307, 36, C_DIM);
    const bal::WeaponStat& w = bal::WEAPON[s.weapon];
    const float attackRate = s.m.atkSpd / maxT(0.01f, w.interval * s.m.cdMul);
    const float moveSpeed = s.m.move * (1.0f + s.p.moveBonus / 100.0f);
    const float critMultiplier = (bal::CRIT_MUL + (s.m.critDmg + s.m.critBonus) / 100.0f) * 100.0f;
    const int secs = (int)s.runT;
    for (int visible = 0; visible < RUN_INFO_VISIBLE_ROWS; ++visible) {
      const int row = scroll + visible; const float y = 47.0f + visible * 16.0f;
      char value[40];
      switch (row) {
        case 0: drawRunSection(S_run_section, y); break;
        case 1: drawRunValue(S_season, T(SEASON_NAME[s.season]), y, SEASON_COLOR[s.season]); break;
        case 2:
          if (s.endless) snprintf(value, sizeof value, "%s", T(S_nav_endless));
          else snprintf(value, sizeof value, "%d-%d", s.season + 1, s.levelIdx + 1);
          drawRunValue(S_level, value, y); break;
        case 3: snprintf(value, sizeof value, "%d", s.wave); drawRunValue(S_wave, value, y); break;
        case 4: snprintf(value, sizeof value, "%d", s.totalKills); drawRunValue(S_enemies_killed, value, y); break;
        case 5: snprintf(value, sizeof value, "%d:%02d", secs / 60, secs % 60); drawRunValue(S_time, value, y); break;
        case 6: snprintf(value, sizeof value, "%d", g.save.bestWave[s.season]); drawRunValue(S_best_wave, value, y, C_GOLD); break;
        case 7: drawRunSection(S_character_section, y); break;
        case 8: case 9: case 10: case 11: case 12: {
          const int stat = row - 8;
          drawRunStat(STAT_NAME[stat], (float)bal::BASE_STAT[stat], (float)statValue(s, stat), "", y); break;
        }
        case 13: drawRunSection(S_combat_section, y); break;
        case 14: drawRunStat(S_stat_damage, w.dmg, (w.dmg + s.m.flat) * s.m.dmgMul * s.m.dealt, "", y); break;
        case 15: drawRunStat(S_stat_attack_rate, 1.0f / w.interval, attackRate, "/s", y); break;
        case 16: drawRunStat(S_stat_range, w.range, w.range * s.m.rangeMul, "px", y); break;
        case 17: drawRunStat(S_stat_move_speed, bal::PLAYER_SPEED, moveSpeed, "px/s", y); break;
        case 18: drawRunStat(S_stat_crit_chance, bal::BASE_CRIT, s.m.crit, "%", y); break;
        case 19: drawRunStat(S_stat_crit_power, bal::CRIT_MUL * 100.0f, critMultiplier, "%", y); break;
        case 20: drawRunSection(S_other_section, y); break;
        case 21: drawRunStat(S_stat_max_hp, bal::BASE_HP, s.p.maxhp, "", y); break;
        case 22: drawRunStat(S_stat_damage_reduction, 0.0f, (1.0f - s.m.taken) * 100.0f, "%", y); break;
        case 23: drawRunStat(S_stat_area, 100.0f, s.m.aoeMul * 100.0f, "%", y); break;
        case 24: drawRunStat(S_stat_projectile_speed, 100.0f, s.m.projSpd * 100.0f, "%", y); break;
        case 25: drawRunStat(S_stat_xp_gain, 100.0f, s.m.expMul * 100.0f, "%", y); break;
        case 26: drawRunStat(S_stat_pierce, (float)w.pierce, (float)(w.pierce + s.m.pierce), "", y); break;
      }
    }
    gfx::rect(314, 47, 2, 174, C_BORDER);
    const int thumbH = 70;
    const int thumbY = 47 + (174 - thumbH) * scroll / (RUN_INFO_ROW_COUNT - RUN_INFO_VISIBLE_ROWS);
    gfx::rect(314, thumbY, 2, thumbH, C_GOLD);
    gfx::textC(T(S_run_info_controls), 160, 222, C_BORDER);
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
void updateAchievementPopup(float dt) {
  if (g.achPopupId >= 0) {
    g.achPopupT -= dt;
    if (g.achPopupT <= 0) { g.achPopupId = -1; g.achPopupT = 0; }
  }
  if (g.achPopupId < 0 && g.achQueueCount > 0) {
    g.achPopupId = g.achQueue[g.achQueueHead];
    g.achQueueHead = (g.achQueueHead + 1) % ACH_COUNT; --g.achQueueCount;
    g.achPopupT = 4.0f; audio::sfx(audio::SFX_LEVEL);
  }
}
void drawAchievementPopup() {
  if (g.achPopupId < 0 || g.achPopupT <= 0) return;
  const int id = clampT(g.achPopupId, 0, ACH_COUNT - 1);
  gfx::panel(10, 180, 380, 50, C_PANEL, C_GOLD);
  gfx::spr(gfx::icon(IC_TROPHY), 18, 192, 2.0f, C_GOLD);
  gfx::text(T(S_ach_popup), 58, 186, C_GOLD);
  textFit(T(ACH_NAME[id]), 58, 202, 320, C_TEXT, 2);
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
    case 9: g.dbgSpellPicker = true; g.dbgSpellSel = 0; break;
    case 10: dbgSpawnGolem(s); break;
    case 11:
      if (s.phase != PH_PLAY) { dbgCompleteLevel(s); break; }
      dbgCompleteLevel(s);
      for (int i = 0; i < NUM_ENEMIES; ++i) {
        const bool belongsToLevel = s.allSeasons ? s.seenEnemy[i] != 0 : ENEMIES[i].season == s.season;
        if (belongsToLevel) g.save.enemySeen[i] = 1;
      }
      for (int i = 0; i < NUM_SPELLS; ++i)
        if (SPELLS[i].sc == SC_SEASON && SPELLS[i].season == s.season && g.save.spellFound[i] == 0)
          g.save.spellFound[i] = 1;
      persist();
      break;
    case 12: dbgAddMinute(s); addLifetime(g.save.playTimeSeconds, 60); persist(); break;
    case 13: forceAchievements(); break;
  }
}

void updGame(const Input& in, float dt) {
  Sim& s = g_sim;
  const u32 secretMask = plat::BTN_L | plat::BTN_R | plat::BTN_X | plat::BTN_Y;
  // Hidden controller unlock: hold L+R+X+Y and press SELECT once. A Luma
  // cheat can perform the same unlock by writing DEBUG_MAGIC to g_debugUnlock.
  if (pressed(in, plat::BTN_SELECT) && (in.held & secretMask) == secretMask) {
    g_debugUnlock = DEBUG_MAGIC;
    plat::debugLog("debug: controller unlock armed");
  }
  if (pressed(in, plat::BTN_SELECT) && g_debugUnlock == DEBUG_MAGIC) { g.debug = !g.debug; g.paused = false; g.dbgSpellPicker = false; }
  if (g.debug) {
    if (g.dbgSpellPicker) {
      const int pageSize = 20, pageStart = (g.dbgSpellSel / pageSize) * pageSize;
      const int pageEnd = minT((int)NUM_SPELLS, pageStart + pageSize);
      const int local = g.dbgSpellSel - pageStart, col = local % 4, row = local / 4;
      if (pressed(in, plat::BTN_LEFT) && col > 0) --g.dbgSpellSel;
      if (pressed(in, plat::BTN_RIGHT) && col < 3 && g.dbgSpellSel + 1 < pageEnd) ++g.dbgSpellSel;
      if (pressed(in, plat::BTN_UP) && row > 0) g.dbgSpellSel -= 4;
      if (pressed(in, plat::BTN_DOWN) && g.dbgSpellSel + 4 < pageEnd) g.dbgSpellSel += 4;
      if (pressed(in, plat::BTN_L) && pageStart > 0) g.dbgSpellSel = maxT(0, g.dbgSpellSel - pageSize);
      if (pressed(in, plat::BTN_R) && pageEnd < NUM_SPELLS) g.dbgSpellSel = minT((int)NUM_SPELLS - 1, g.dbgSpellSel + pageSize);
      for (int id = pageStart; id < pageEnd; ++id) {
        const int j = id - pageStart, x = 4 + (j % 4) * 78, y = 26 + (j / 4) * 23;
        if (inR(in, x, y, 75, 21)) g.dbgSpellSel = id;
      }
      if (pressed(in, plat::BTN_A) || inR(in, 4, 211, 312, 29)) dbgGrantSpell(s, g.dbgSpellSel);
      if (pressed(in, plat::BTN_B)) g.dbgSpellPicker = false;
      return;
    }
    static const int actionCount = 14, visible = 7;
    if (pressed(in, plat::BTN_DOWN)) g.dbgSel = (g.dbgSel + 1) % actionCount;
    if (pressed(in, plat::BTN_UP)) g.dbgSel = (g.dbgSel + actionCount - 1) % actionCount;
    const int first = clampT(g.dbgSel - visible / 2, 0, actionCount - visible);
    for (int row = 0; row < visible; ++row) if (inR(in, 12, 34 + row * 24, 296, 22)) { g.dbgSel = first + row; runDebug(g.dbgSel); return; }
    if (pressed(in, plat::BTN_A)) runDebug(g.dbgSel);
    if (pressed(in, plat::BTN_B)) { g.debug = false; g.dbgSpellPicker = false; }
    return;
  }
  if (pressed(in, plat::BTN_START) && s.phase == PH_PLAY) { g.paused = !g.paused; g.pauseSel = 0; g.pauseTab = 0; }
  if (g.paused) {
    if (pressed(in, plat::BTN_L)) g.pauseTab = (g.pauseTab + 2) % 3;
    if (pressed(in, plat::BTN_R)) g.pauseTab = (g.pauseTab + 1) % 3;
    for (int i = 0; i < 3; ++i) if (inR(in, 4 + i * 104, 22, 100, 18)) g.pauseTab = i;
    if (g.pauseTab == 1) {
      const int maxPauseScroll = 27 - 9;
      if (pressed(in, plat::BTN_DOWN) || pressed(in, plat::BTN_Y)) g.runInfoScroll = minT(maxPauseScroll, g.runInfoScroll + 1);
      if (pressed(in, plat::BTN_UP) || pressed(in, plat::BTN_X)) g.runInfoScroll = maxT(0, g.runInfoScroll - 1);
      if (in.touchDown && in.ty >= 48 && in.ty < 214) { g.runInfoTouching = true; g.runInfoTouchY = in.ty; g.runInfoDragPx = 0; }
      if (in.touchHeld && g.runInfoTouching) {
        g.runInfoDragPx += g.runInfoTouchY - in.ty; g.runInfoTouchY = in.ty;
        while (g.runInfoDragPx >= 12) { g.runInfoScroll = minT(maxPauseScroll, g.runInfoScroll + 1); g.runInfoDragPx -= 12; }
        while (g.runInfoDragPx <= -12) { g.runInfoScroll = maxT(0, g.runInfoScroll - 1); g.runInfoDragPx += 12; }
      }
      if (in.touchUp || !in.touchHeld) g.runInfoTouching = false;
      return;
    }
    if (g.pauseTab == 2) return;
    if (pressed(in, plat::BTN_DOWN) || pressed(in, plat::BTN_UP)) g.pauseSel ^= 1;
    int act = -1;
    if (inR(in, 12, 112, 296, 24)) act = 0;
    if (inR(in, 12, 142, 296, 24)) act = 1;
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
      if (g.tab == 2) {
        const int maxScroll = 27 - 11;
        if (pressed(in, plat::BTN_X)) g.runInfoScroll = maxT(0, g.runInfoScroll - 5);
        if (pressed(in, plat::BTN_Y)) g.runInfoScroll = minT(maxScroll, g.runInfoScroll + 5);
        if (in.touchDown) { g.runInfoTouching = true; g.runInfoTouchY = in.ty; g.runInfoDragPx = 0; }
        if (in.touchHeld && g.runInfoTouching) {
          g.runInfoDragPx += g.runInfoTouchY - in.ty; g.runInfoTouchY = in.ty;
          while (g.runInfoDragPx >= 12) { g.runInfoScroll = minT(maxScroll, g.runInfoScroll + 1); g.runInfoDragPx -= 12; }
          while (g.runInfoDragPx <= -12) { g.runInfoScroll = maxT(0, g.runInfoScroll - 1); g.runInfoDragPx += 12; }
        }
        if (in.touchUp || !in.touchHeld) g.runInfoTouching = false;
      } else {
        g.runInfoTouching = false;
      }
      if (g.tab == 0) {
        for (int i = 0; i < bal::MAX_SLOTS; ++i) if (inR(in, 8 + (i % 4) * 76, 26 + (i / 4) * 44, 72, 40)) g.slotSel = i;
        if (pressed(in, plat::BTN_X)) g.slotSel = (g.slotSel + 1) % bal::MAX_SLOTS;
        if (pressed(in, plat::BTN_Y)) g.slotSel = (g.slotSel + bal::MAX_SLOTS - 1) % bal::MAX_SLOTS;
      }
      const bool touchSacrifice = in.touchHeld && in.tx >= 8 && in.tx < 312 && in.ty >= 218 && in.ty < 238;
      const bool canSacrifice = g.tab == 0 && s.endless && simSacrificeReward(s, g.slotSel) > 0;
      const bool holdingSacrifice = canSacrifice && (((in.held & plat::BTN_A) != 0) || touchSacrifice);
      if (holdingSacrifice) {
        if (g.sacrificeHoldSlot != g.slotSel) { g.sacrificeHoldSlot = g.slotSel; g.sacrificeHoldT = 0.0f; }
        g.sacrificeHoldT = minT(3.0f, g.sacrificeHoldT + dt);
        if (g.sacrificeHoldT >= 3.0f && simSacrificeSpell(s, g.slotSel)) {
          g.slotSel = minT(g.slotSel, maxT(0, s.nSlots - 1)); g.sacrificeHoldT = 0.0f; g.sacrificeHoldSlot = -1;
        }
      } else { g.sacrificeHoldT = 0.0f; g.sacrificeHoldSlot = -1; }
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
      if (!s.randomSpells && (pressed(in, plat::BTN_L) || pressed(in, plat::BTN_LEFT))) { simCycleOffers(s, -1); g.offerSel = 0; }
      if (!s.randomSpells && (pressed(in, plat::BTN_R) || pressed(in, plat::BTN_RIGHT))) { simCycleOffers(s, 1); g.offerSel = 0; }
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
  if (!plat::init()) { plat::debugLog("game: platform init FAILED"); return false; }
  plat::debugLog("game: platform init OK");
  const SaveLoadStatus loadStatus = saveLoadEx(g.save);
  g.saveWriteBlocked = loadStatus == SAVE_LOAD_RECOVERY_REQUIRED;
  memset(g.achKnown, 0, sizeof g.achKnown); g.achQueueHead = 0; g.achQueueCount = 0; g.achPopupId = -1; g.achPopupT = 0;
  scanAchievementUnlocks(false);
  const char* statusText = loadStatus == SAVE_LOAD_OK ? "loaded" :
                           (loadStatus == SAVE_LOAD_MIGRATED ? "migrated" :
                           (loadStatus == SAVE_LOAD_FRESH ? "new" : "recovery-required"));
  plat::debugLog("save: load=%s slots=sdmc:/3ds/AbyssalEclipse/save.0|save.1 legacy=save.bin", statusText);
  g.inited = true;
  bool notesLoaded = plat::romfsRead("note.txt", g.notes, sizeof(g.notes)-1, &g.notesLen);
  const char* notesPath = "note.txt";
  if (notesLoaded) {
      // Normalize line endings: CRLF and stray CR both become LF, so textWrap
      // (which only understands '\n') doesn't try to render '\r' as '?'.
      size_t w = 0;
      for (size_t i = 0; i < g.notesLen; ++i) {
          const char c = g.notes[i];
          if (c == '\r') {
              g.notes[w++] = '\n';
              if (i + 1 < g.notesLen && g.notes[i + 1] == '\n') ++i;  // skip the LF of a CRLF
          } else {
              g.notes[w++] = c;
          }
      }
      g.notesLen = w;
      g.notes[g.notesLen < sizeof(g.notes) ? g.notesLen : sizeof(g.notes)-1] = 0;
  }
  plat::debugLog("romfs: %s %s (%u bytes)", notesPath, notesLoaded ? "loaded" : "NOT FOUND", (unsigned)g.notesLen);
  applyOptions();
  plat::debugLog("settings: language=%s economy_mode=%s fps=%u music_volume=%u sfx_volume=%u show_fps=%u",
                 g.save.lang == LANG_PT ? "pt-BR" : "en", g.save.fps30 ? "on" : "off",
                 g.save.fps30 ? 30u : 60u, (unsigned)g.save.musicVol, (unsigned)g.save.sfxVol,
                 (unsigned)g.save.showFps);
  if (g.saveWriteBlocked) toast(T(S_save_recovery));
  audio::init();
  g.silent = !plat::audioInit(audio::mix);
  plat::debugLog("game: audio stage complete silent=%d", g.silent ? 1 : 0);
  if (!gfx::init(cosm())) { plat::debugLog("game: gfx init FAILED"); return false; }
  plat::debugLog("game: gfx init OK");
  g.weaponSel = g.save.lastWeapon;
  go(SC_MENU);
  return true;
}
void gameShutdown() { if (g.inited) persist(); plat::audioShutdown(); plat::shutdown(); }
int gameTargetFps() { return g.save.fps30 ? 30 : 60; }
Sim& gameSim() { return g_sim; }
void gameStartRunForTest(int season, int level, bool endless, int weapon, u32 seed) {
  g.season = season; g.level = level; g.endless = endless; g.allSeasons = false; g.weaponSel = weapon;
  g.back = SC_MENU; g.debug = false; g.dbgSpellPicker = false; g.dbgSel = 0; g.tab = 0; g.slotSel = 0;
  g.runInfoScroll = 0; g.runInfoTouching = false;
  simStart(g_sim, season, level, endless, weapon, seed, false); g.scr = SC_GAME;
}
int gameCatalogJumpForTest(int tab, int currentSection, int direction) {
  g.catTab = clampT(tab, 0, CAT_TAB_COUNT - 1); catCount();
  int from = catFirstSelectable();
  for (int i = 0; i < g_catCount; ++i)
    if (catSelectable(i) && g_catEntries[i].section == currentSection) { from = i; break; }
  const int next = catSectionJump(from, direction);
  return catSelectable(next) ? g_catEntries[next].section : currentSection;
}
int gameAchievementPopupForTest() { return g.achPopupId; }
void gameCampaignHubForTest() { g.scr = SC_CAMPAIGNS; g.campaignSel = 0; g.campaignContentSel = 0; g.campaignContentFocus = false; }
int gameCampaignSelectionForTest() { return g.campaignSel; }
int gameCampaignFocusForTest() { return g.campaignContentFocus ? 1 : 0; }
int gameRunInfoScrollForTest() { return g.runInfoScroll; }
bool gameDebugMenuForTest() { return g.debugMenu; }

bool gameFrame(const Input& in, float dt) {
  if (dt > 0.0f) {
    g.fpsElapsed += dt; ++g.fpsFrames;
    if (g.fpsElapsed >= 1.0f) {
      g.fps = (int)(g.fpsFrames / g.fpsElapsed + 0.5f);
      g.fpsFrames = 0; g.fpsElapsed = 0.0f;
    }
  }
  if (g.toastT > 0) g.toastT -= dt;
  updateAchievementPopup(dt);
  if (updMainDebug(in)) return true;
  switch (g.scr) {
    case SC_MENU: updMenu(in); break;
    case SC_CAMPAIGNS: updCampaignHub(in); break;
    case SC_CAMPAIGN: updCampaign(in); break;
    case SC_WEAPON: updWeapon(in); break;
    case SC_CUSTOM: updCustom(in); break;
    case SC_GAME: updGame(in, dt); break;
    case SC_CATALOG: updCatalog(in); break;
    case SC_ACHIEVEMENTS: updAchievements(in); break;
    case SC_OPTIONS: updOptions(in); break;
    case SC_NOTES: if (pressed(in, plat::BTN_B)) go(SC_MENU); else if (pressed(in, plat::BTN_DOWN)) ++g.notesScroll; else if (pressed(in, plat::BTN_UP)) g.notesScroll = maxT(0, g.notesScroll - 1); break;
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

static void drawMainDebugOverlay() {
  gfx::rect(0, 0, BOT_W, BOT_H, C_VOID, 225);
  gfx::panel(7, 8, 306, 224, C_PANEL, C_RED);
  header("DEVELOPMENT WATCH", C_RED);
  static const char* labels[5] = {"FORCE UNLOCK ACHIEVEMENTS", "TOGGLE ALL CONTENT UNLOCKS", "COMPLETE CATALOG DISCOVERY", "DEBUG STATUS", "EXPORT RAM SPRITESHEET (RGBA)"};
  for (int i = 0; i < 5; ++i) {
    const int y = 27 + i * 35;
    button(14, y, 292, 30, labels[i], g.dbgMainSel == i);
    if (i == 1) gfx::textR(g.save.unlockAll ? "ON" : "OFF", 296, y + 10, g.save.unlockAll ? C_GREEN : C_DIM);
  }
  gfx::textC("UP/DOWN SCROLL  A SELECT  B/SELECT CLOSE", 160, 218, C_DIM, 1);
}
void gameDraw() {
  plat::beginScreen(true);
  switch (g.scr) {
    case SC_MENU: drawMenuTop(); break;
    case SC_CAMPAIGNS: drawCampaignHubTop(); break;
    case SC_CAMPAIGN: drawCampaignTop(); break;
    case SC_WEAPON: drawWeaponTop(); break;
    case SC_CUSTOM: drawCustomTop(); break;
    case SC_GAME: drawGameTop(); break;
    case SC_CATALOG: drawCatalogTop(); break;
    case SC_ACHIEVEMENTS: drawAchievementsTop(); break;
    case SC_OPTIONS: drawOptionsTop(); break;
    case SC_NOTES: gfx::rect(0,0,TOP_W,TOP_H,C_VOID); gfx::textC(T(S_nav_notes),200,40,C_GOLD,3); gfx::textC(T(S_version),200,100,C_DIM,1); break;
    case SC_CREDITS: drawCreditsTop(); break;
  }
  if (g.toastT > 0 && g.scr != SC_GAME) gfx::textC(g.toast, 200, 224, C_RED);
  drawAchievementPopup();
  plat::beginScreen(false);
  switch (g.scr) {
    case SC_MENU: drawMenuBot(); break;
    case SC_CAMPAIGNS: drawCampaignHubBottom(); break;
    case SC_CAMPAIGN: drawCampaignBot(); break;
    case SC_WEAPON: drawWeaponBot(); break;
    case SC_CUSTOM: drawCustomBot(); break;
    case SC_GAME: drawGameBot(); break;
    case SC_CATALOG: drawCatalogBot(); break;
    case SC_ACHIEVEMENTS: drawAchievementsBot(); break;
    case SC_OPTIONS: drawOptionsBot(); break;
    case SC_NOTES: { gfx::rect(0,0,BOT_W,BOT_H,C_VOID); header(T(S_nav_notes)); int total=0; gfx::textWrap(g.notes[0]?g.notes:T(S_notes_missing),8,28,300,C_TEXT,1,20,g.notesScroll,&total); g.notesScroll=clampT(g.notesScroll,0,maxT(0,total-20)); gfx::textC(T(S_notes_controls),160,222,C_DIM); break; }
    case SC_CREDITS:
      gfx::rect(0, 0, BOT_W, BOT_H, C_VOID);
      gfx::textC(T(S_btn_back), 160, 100, C_DIM);
      gfx::sprC(gfx::drone(), 160, 180, 4.0f);       // the easter-egg drone
      if (g.secretCatalog) gfx::textC("SECRET UNLOCKED!", 160, 220, C_GOLD);
      break;
  }
  if (g.scr != SC_GAME && g.debugMenu) drawMainDebugOverlay();
}
