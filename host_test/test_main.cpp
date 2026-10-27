// Headless soak test: runs the real game logic (all screens + simulation) on the host with scripted input.
#include "../source/game.h"
#include "../source/sim.h"
#include "../source/save.h"
#include "../source/audio.h"
#include "../source/data.h"
#include "../source/i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../source/platform.h"
namespace plat { u64 statMaxDrawsPerFrame(); }

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void checkInvariants(Sim& s, const char* tag) {
  int ea = 0, pa = 0;
  for (int i = 0; i < bal::MAX_ENEMIES; ++i) if (s.en[i].alive) {
    ++ea;
    CHECK(s.en[i].hp == s.en[i].hp && s.en[i].x == s.en[i].x, "%s: NaN enemy", tag);
    CHECK(s.en[i].type < NUM_ENEMIES, "%s: bad type", tag);
  }
  for (int i = 0; i < bal::MAX_PROJ; ++i) if (s.pr[i].alive) ++pa;
  CHECK(ea <= bal::MAX_ENEMIES && pa <= bal::MAX_PROJ, "%s: pool overflow", tag);
  CHECK(s.p.x == s.p.x && s.p.y == s.p.y && s.p.hp == s.p.hp, "%s: NaN player", tag);
  CHECK(s.p.hp <= s.p.maxhp + 0.01f, "%s: hp>max", tag);
  CHECK(s.nSlots >= 1 && s.nSlots <= bal::MAX_SLOTS, "%s: slots %d", tag, s.nSlots);
  CHECK(s.points >= 0, "%s: points", tag);
}

// Auto-pilot: kite away from the nearest enemy, spend points, pick offers.
static void autopilot(Sim& s, int frame, SimInput& in) {
  float best = 1e9f, bx = 0, by = 0;
  for (int i = 0; i < bal::MAX_ENEMIES; ++i) if (s.en[i].alive) {
    float dx = s.en[i].x - s.p.x, dy = s.en[i].y - s.p.y, d = dx * dx + dy * dy;
    if (d < best) { best = d; bx = dx; by = dy; }
  }
  in.mx = in.my = 0;
  if (best < 80 * 80) { float l = sqrtf(best) + 0.01f; in.mx = -bx / l; in.my = -by / l; }
  else { in.mx = (200 - s.p.x) / 200; in.my = (130 - s.p.y) / 130; }
  if (frame % 300 < 40) in.mx += 0.5f;
}

static int runOne(int season, int level, bool endless, int weapon, int maxFrames, bool godMode, double* msPerFrame) {
  gameStartRunForTest(season, level, endless, weapon, 12345u + season * 31 + weapon);
  Sim& s = gameSim();
  s.godMode = godMode;
  u64 t0 = plat::nowMs();
  int f = 0, maxEn = 0, maxPr = 0, picks = 0;
  for (; f < maxFrames; ++f) {
    SimInput in{0, 0};
    autopilot(s, f, in);
    switch (s.phase) {
      case PH_PLAY: simUpdate(s, in, 1.0f / 60); break;
      case PH_CLEARED: simUpdate(s, in, 1.0f / 60); break;
      case PH_ALLOC: while (s.points > 0) simSpend(s, s.rng.i(ST_COUNT)); simFinishAlloc(s); break;
      case PH_SPELL: simPickOffer(s, s.nOffers ? s.rng.i(s.nOffers) : -1); ++picks; break;
      case PH_DEAD: case PH_VICTORY: f = maxFrames; break;
    }
    int ea = 0, pa = 0;
    for (int i = 0; i < bal::MAX_ENEMIES; ++i) ea += s.en[i].alive;
    for (int i = 0; i < bal::MAX_PROJ; ++i) pa += s.pr[i].alive;
    if (ea > maxEn) maxEn = ea;
    if (pa > maxPr) maxPr = pa;
    if ((f & 63) == 0) checkInvariants(s, "run");
    if ((f % 15) == 0) { plat::Input pin; plat::beginFrame(pin); gameDraw(); plat::endFrame(); }
  }
  checkInvariants(s, "end");
  double ms = (double)(plat::nowMs() - t0);
  *msPerFrame = ms / (f > 0 ? f : 1);
  printf("  season %d lv %d %s weapon %d: phase=%s wave=%d kills=%d level=%d spells=%d maxEnemies=%d maxProj=%d picks=%d  %.3f ms/frame\n",
         season, level, endless ? "endless" : "campaign", weapon, phaseName(s.phase), s.wave, s.totalKills, s.p.level, s.nSlots, maxEn, maxPr, picks, *msPerFrame);
  return s.wave;
}

int main() {
  remove("save.bin");
  CHECK(gameInit(), "gameInit failed");
  printf("[data]\n");
  CHECK(SPELL_COUNT == 137 && ENEMY_COUNT == 25, "data counts");
  int perScope[3] = {0, 0, 0}; int per[SEA_COUNT] = {0}; int perEl[EL_COUNT] = {0};
  for (int i = 0; i < NUM_SPELLS; ++i) {
    perScope[SPELLS[i].sc]++;
    if (SPELLS[i].sc == SC_SEASON) per[SPELLS[i].season]++;
    if (SPELLS[i].sc == SC_ELEMENT) perEl[SPELLS[i].el]++;
    CHECK(SPELLS[i].eff[0].kind != E_NONE, "spell %d has no effect", i);
    CHECK(spellName(i)[0] != 0, "spell %d name empty", i);
  }
  CHECK(perScope[0] == 22 && perScope[1] == 90 && perScope[2] == 25, "scope counts %d/%d/%d", perScope[0], perScope[1], perScope[2]);
  for (int e = 0; e < EL_COUNT; ++e) CHECK(perEl[e] == 15, "element %d has %d", e, perEl[e]);
  for (int s = 0; s < SEA_COUNT; ++s) CHECK(per[s] == 5, "season %d has %d exclusives", s, per[s]);
  for (int l = 0; l < LANG_COUNT; ++l) { g_lang = l; for (int i = 0; i < NUM_SPELLS; ++i) CHECK(spellName(i)[0], "lang %d spell %d", l, i); for (int i = 0; i < NUM_ENEMIES; ++i) CHECK(enemyName(i)[0], "lang %d enemy %d", l, i); }
  g_lang = LANG_EN;

  printf("[every spell in isolation: apply + simulate 6 s]\n");
  for (int id = 0; id < NUM_SPELLS; ++id) {
    gameStartRunForTest(SPELLS[id].season == SEA_NONE ? 0 : SPELLS[id].season, 0, true, id % 4, 777u + id);
    Sim& s = gameSim();
    s.slots[s.nSlots++] = SpellSlot{(s16)id, (u8)(1 + id % 3)}; recompute(s);
    for (int f = 0; f < 360; ++f) { SimInput in{0.3f, 0.0f}; s.godMode = true; if (s.phase == PH_PLAY) simUpdate(s, in, 1.0f / 60); if (s.phase == PH_ALLOC || s.phase == PH_SPELL) break; }
    checkInvariants(s, spellName(id));
  }

  printf("[full runs, 60 Hz logic]\n");
  double worst = 0, ms;
  for (int sea = 0; sea < SEA_COUNT; ++sea) { int w = runOne(sea, 0, false, sea % 4, 60 * 60 * 14, true, &ms); CHECK(gameSim().phase == PH_VICTORY, "season %d campaign level 1 should be completable with god mode (wave %d)", sea, w); if (ms > worst) worst = ms; }
  for (int w = 0; w < 4; ++w) { runOne(1, 2, true, w, 60 * 60 * 6, true, &ms); if (ms > worst) worst = ms; }
  printf("[balance probe: mortal autopilot (kites, never aims), campaign level 1]\n");
  for (int sea = 0; sea < SEA_COUNT; ++sea) for (int w = 0; w < 4; w += 1) { runOne(sea, 0, false, w, 60 * 60 * 14, false, &ms); }

  printf("[debug tools + phases]\n");
  gameStartRunForTest(0, 0, false, 0, 99);
  Sim& s = gameSim();
  dbgWaveEnd(s); CHECK(s.phase == PH_CLEARED, "waveEnd phase %s", phaseName(s.phase));
  for (int f = 0; f < 200; ++f) simUpdate(s, SimInput{0, 0}, 1.0f / 60);
  CHECK(s.phase == PH_SPELL || s.phase == PH_ALLOC, "after clear phase %s", phaseName(s.phase));
  while (s.phase == PH_ALLOC) { while (s.points > 0) simSpend(s, 0); simFinishAlloc(s); }
  CHECK(s.phase == PH_SPELL && s.nOffers == 3, "offers %d", s.nOffers);
  int w0 = s.wave; simPickOffer(s, 0); CHECK(s.phase == PH_PLAY && s.wave == w0 + 1 && s.nSlots == 2, "pick -> next wave");
  dbgBoss(s); bool boss = false; for (int i = 0; i < bal::MAX_ENEMIES; ++i) if (s.en[i].alive && (ENEMIES[s.en[i].type].beh & B_BOSS)) boss = true; CHECK(boss, "boss spawned");
  dbgClearSpells(s); CHECK(s.nSlots == 1, "clear spells");
  dbgDeath(s); CHECK(s.phase == PH_DEAD, "death");
  gameStartRunForTest(0, 0, false, 0, 99); Sim& s2 = gameSim();
  for (int i = 0; i < 12; ++i) { dbgForceSpell(s2); if (s2.phase == PH_SPELL) simPickOffer(s2, 0); }
  CHECK(s2.nSlots == bal::MAX_SLOTS, "12/12 slots (%d)", s2.nSlots);
  s2.phase = PH_CLEARED; s2.phaseT = 10; simUpdate(s2, SimInput{0, 0}, 0.016f);
  while (s2.phase == PH_ALLOC) simFinishAlloc(s2);
  CHECK(s2.phase == PH_SPELL && s2.offersAreUpgrades, "12/12 offers upgrades");
  if (s2.phase == PH_SPELL) { int slot = s2.offers[0]; simPickOffer(s2, 0); CHECK(s2.slots[slot].lv == 2, "upgrade applied"); }

  printf("[save round trip]\n");
  SaveData a; saveDefaults(a); a.bestWave[2] = 17; a.levelCleared[0] = 7; a.spellFound[5] = 3; CHECK(saveStore(a), "store");
  SaveData b; CHECK(saveLoad(b) && b.bestWave[2] == 17 && b.spellFound[5] == 3, "load");
  CHECK(seasonUnlocked(b, 1) && !seasonUnlocked(b, 2) && levelUnlocked(b, 0, 2), "unlock chain");
  FILE* f = fopen("save.bin", "r+b"); fseek(f, 40, SEEK_SET); fputc(0x55, f); fclose(f);
  CHECK(!saveLoad(b) && b.bestWave[2] == 0, "corrupt save falls back to defaults");

  printf("[audio mixer]\n");
  s16 buf[2048]; audio::sfx(audio::SFX_BOSS); audio::mix(buf, 2048); int nz = 0; for (int i = 0; i < 2048; ++i) nz += buf[i] != 0; CHECK(nz > 100, "mixer produced sound (%d)", nz);

  printf("[ui frames: all screens draw without crashing]\n");
  for (int lang = 0; lang < 2; ++lang) for (int scr = 0; scr < 6; ++scr) {
    g_lang = lang; plat::Input in; in.down = 0;
    for (int k = 0; k < 4; ++k) { gameFrame(in, 1.0f / 60); gameDraw(); }
    in.down = plat::BTN_A; gameFrame(in, 1.0f / 60); gameDraw();
  }
  printf("max draw calls in a frame (all screens): %llu\n", (unsigned long long)plat::statMaxDrawsPerFrame());
  printf("worst simulation cost: %.3f ms/frame on this host\n", worst);
  printf("%s (%d failures)\n", fails ? "FAILED" : "ALL TESTS PASSED", fails);
  return fails ? 1 : 0;
}
