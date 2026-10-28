// Headless soak test: runs the real game logic (all screens + simulation) on the host with scripted input.
#include "../source/game.h"
#include "../source/sim.h"
#include "../source/save.h"
#include "../source/audio.h"
#include "../source/data.h"
#include "../source/i18n.h"
#include "../source/gfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../source/platform.h"
namespace plat { u64 statMaxDrawsPerFrame(); }

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

struct SaveV3Fixture {
  u32 magic; u16 version; u16 size; u32 crc;
  u8 lang, musicVol, sfxVol, fps30; u8 showFps;
  u8 hat, face, color, lastWeapon; u8 unlockAll, pad0, pad1, pad2;
  u8 levelCleared[SEA_COUNT]; u8 pad3[3];
  u16 bestWave[SEA_COUNT]; u16 pad4[3]; u16 spellFound[141];
  u16 enemyKills[NUM_ENEMIES]; u8 enemySeen[NUM_ENEMIES + 3];
};
static_assert(sizeof(SaveV3Fixture) == 412, "test fixture must match the historical v3 save");
struct SaveV4Fixture {
  u32 magic; u16 version; u16 size; u32 crc;
  u8 lang, musicVol, sfxVol, fps30; u8 showFps;
  u8 hat, face, color, lastWeapon; u8 unlockAll, pad0, pad1, pad2;
  u8 levelCleared[SEA_COUNT]; u8 pad3[3];
  u16 bestWave[SEA_COUNT]; u16 pad4[3]; u16 spellFound[144];
  u16 enemyKills[NUM_ENEMIES]; u8 enemySeen[NUM_ENEMIES + 3];
  u32 gamesPlayed, runsWon, runsLost, totalKills;
  u32 spellsAcquired, spellsCasted, spellUpgrades, weaponThrows, playTimeSeconds;
};
static_assert(sizeof(SaveV4Fixture) == 452, "test fixture must match the historical v4 save");
static u32 testCrc32(const u8* p, size_t n) {
  u32 c = 0xFFFFFFFFu;
  for (size_t i = 0; i < n; ++i) { c ^= p[i]; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
  return ~c;
}

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
  CHECK(SPELL_COUNT == NUM_SPELLS && SPELL_COUNT == 147 && ENEMY_COUNT == 25, "data counts");
  int perScope[3] = {0, 0, 0}; int per[SEA_COUNT] = {0}; int perEl[EL_COUNT] = {0};
  for (int i = 0; i < NUM_SPELLS; ++i) {
    perScope[SPELLS[i].sc]++;
    if (SPELLS[i].sc == SC_SEASON) per[SPELLS[i].season]++;
    if (SPELLS[i].sc == SC_ELEMENT) perEl[SPELLS[i].el]++;
    CHECK(SPELLS[i].eff[0].kind != E_NONE, "spell %d has no effect", i);
    CHECK(spellName(i)[0] != 0, "spell %d name empty", i);
  }
  CHECK(perScope[0] == 25 && perScope[1] == 96 && perScope[2] == 26, "scope counts %d/%d/%d", perScope[0], perScope[1], perScope[2]);
  for (int e = 0; e < EL_COUNT; ++e) CHECK(perEl[e] == 16, "element %d has %d", e, perEl[e]);
  const int expectedSeason[SEA_COUNT] = {5, 5, 5, 5, 6};
  for (int s = 0; s < SEA_COUNT; ++s) CHECK(per[s] == expectedSeason[s], "season %d has %d exclusives", s, per[s]);
  for (int l = 0; l < LANG_COUNT; ++l) { g_lang = l; for (int i = 0; i < NUM_SPELLS; ++i) CHECK(spellName(i)[0], "lang %d spell %d", l, i); for (int i = 0; i < NUM_ENEMIES; ++i) CHECK(enemyName(i)[0], "lang %d enemy %d", l, i); }
  g_lang = LANG_EN;

  printf("[weapon stats + katana melee regression]\n");
  for (int w = 0; w < W_COUNT; ++w)
    CHECK(bal::WEAPON[w].dmg > 0.0f && bal::WEAPON[w].interval > 0.0f && bal::WEAPON[w].range > 0.0f && bal::WEAPON[w].speed > 0.0f,
          "weapon %d has missing combat stats", w);
  CHECK(bal::KATANA_RANGE == 46.0f && bal::WEAPON[W_KATANA].range == 56.0f,
        "katana reach should be reduced to a closer cleave (%0.1f/%0.1f)", bal::KATANA_RANGE, bal::WEAPON[W_KATANA].range);
  gameStartRunForTest(0, 0, true, W_KATANA, 4242u);
  Sim& katana = gameSim();
  Enemy& katanaTarget = katana.en[0];
  katanaTarget.alive = true; katanaTarget.type = 0;
  katanaTarget.x = katana.p.x + 24.0f; katanaTarget.y = katana.p.y;
  katanaTarget.hp = katanaTarget.maxhp = 100.0f; katanaTarget.spd = katanaTarget.dmg = 0.0f;
  katanaTarget.atkT = 100.0f; katana.enemiesAlive = 1; katana.godMode = true;
  katana.p.atkT = 0.001f;
  simUpdate(katana, SimInput{0.0f, 0.0f}, 1.0f / 60.0f);
  CHECK(katanaTarget.hp < 100.0f, "katana should damage a target in front of the player");
  bool hasSlash = false;
  for (int i = 0; i < bal::MAX_FX; ++i) hasSlash |= katana.fx[i].kind == FX_SLASH;
  CHECK(hasSlash, "katana hit should create a curved slash effect");
  plat::Input slashFrame; plat::beginFrame(slashFrame); simDraw(katana, false, 0); plat::endFrame();

  printf("[aura rendering: lightning + Vacuum particles]\n");
  int vacuumId = -1, staticFieldId = -1;
  for (int i = 0; i < NUM_SPELLS; ++i) {
    if (!strcmp(spellName(i), "VACUUM")) vacuumId = i;
    if (!strcmp(spellName(i), "STATIC FIELD")) staticFieldId = i;
  }
  CHECK(vacuumId >= 0 && staticFieldId >= 0, "aura visual fixtures exist in localization data");
  gameStartRunForTest(SEA_FOREST, 0, true, W_DAGGER, 4244u);
  Sim& auraVisual = gameSim();
  auraVisual.nSlots = 3;
  auraVisual.slots[1] = SpellSlot{(s16)vacuumId, 1};
  auraVisual.slots[2] = SpellSlot{(s16)staticFieldId, 1};
  recompute(auraVisual);
  auraVisual.en[0].alive = true; auraVisual.en[0].x = auraVisual.p.x + 25.0f; auraVisual.en[0].y = auraVisual.p.y;
  auraVisual.en[0].hp = auraVisual.en[0].maxhp = 100.0f; auraVisual.enemiesAlive = 1;
  plat::Input auraFrame; plat::beginFrame(auraFrame); simDraw(auraVisual, false, 0); plat::endFrame();
  CHECK(plat::statMaxDrawsPerFrame() < 1200, "animated auras stay under the draw-call budget (%llu)", (unsigned long long)plat::statMaxDrawsPerFrame());

  gameStartRunForTest(0, 0, true, W_MACHINE_GUN, 4243u);
  Sim& machineGun = gameSim();
  Enemy& gunTarget = machineGun.en[0];
  gunTarget.alive = true; gunTarget.type = 0;
  gunTarget.x = machineGun.p.x + 40.0f; gunTarget.y = machineGun.p.y;
  gunTarget.hp = gunTarget.maxhp = 100.0f; gunTarget.spd = gunTarget.dmg = 0.0f;
  gunTarget.atkT = 100.0f; machineGun.enemiesAlive = 1; machineGun.godMode = true;
  machineGun.p.atkT = 0.001f;
  simUpdate(machineGun, SimInput{0.0f, 0.0f}, 1.0f / 60.0f);
  int validShots = 0;
  for (int i = 0; i < bal::MAX_PROJ; ++i) if (machineGun.pr[i].alive) {
    ++validShots;
    CHECK(machineGun.pr[i].spr < gfx::PS_COUNT, "machine gun used invalid projectile sprite %u", machineGun.pr[i].spr);
  }
  CHECK(validShots > 0, "machine gun should create a projectile when a target is in range");

  printf("[every spell in isolation: apply + simulate 6 s]\n");
  for (int id = 0; id < NUM_SPELLS; ++id) {
    gameStartRunForTest(SPELLS[id].season == SEA_NONE ? 0 : SPELLS[id].season, 0, true, id % 4, 777u + id);
    Sim& s = gameSim();
    s.slots[s.nSlots++] = SpellSlot{(s16)id, (u8)(1 + id % 3)}; recompute(s);
    for (int f = 0; f < 360; ++f) { SimInput in{0.3f, 0.0f}; s.godMode = true; if (s.phase == PH_PLAY) simUpdate(s, in, 1.0f / 60); if (s.phase == PH_ALLOC || s.phase == PH_SPELL) break; }
    checkInvariants(s, spellName(id));
  }

  printf("[per-spell caps + every available rank upgrade]\n");
  int restrictedSpell = -1, ordinarySpell = -1, restrictedCount = 0;
  for (int id = 0; id < NUM_SPELLS; ++id) {
    bool entityEffect = false;
    for (int k = 0; k < 3; ++k) {
      const EffKind kind = (EffKind)SPELLS[id].eff[k].kind;
      if (kind == E_ORBIT || kind == E_SENTINEL || kind == E_DRONE_MINIGUN || kind == E_FAIRY) entityEffect = true;
    }
    const int expectedCap = entityEffect ? 5 : (strcmp(spellName(id), "TIME-STOP") == 0 ? 9 : bal::MAX_SPELL_LV);
    CHECK(SPELLS[id].maxLevel == expectedCap,
          "%s cap should be %d, got %d", spellName(id), expectedCap, SPELLS[id].maxLevel);
    if (entityEffect) { ++restrictedCount; if (restrictedSpell < 0) restrictedSpell = id; }
    else if (ordinarySpell < 0) ordinarySpell = id;
    Mods previous; bool havePrevious = false;
    for (int level = 1; level <= SPELLS[id].maxLevel; ++level) {
      const int sea = SPELLS[id].season == SEA_NONE ? (int)SEA_FOREST : (int)SPELLS[id].season;
      gameStartRunForTest(sea, 0, true, id % W_COUNT, 10000u + id);
      Sim& levelSim = gameSim();
      levelSim.slots[levelSim.nSlots++] = SpellSlot{(s16)id, (u8)level}; recompute(levelSim);
      if (havePrevious) CHECK(memcmp(&previous, &levelSim.m, sizeof(Mods)) != 0,
                              "%s level %d has no effective upgrade", spellName(id), level);
      previous = levelSim.m; havePrevious = true;
    }
  }
  CHECK(restrictedCount == 6, "orbit/summon spell category has six capped members, got %d", restrictedCount);

  if (restrictedSpell >= 0 && ordinarySpell >= 0) {
    gameStartRunForTest(SEA_FOREST, 0, true, W_DAGGER, 10101u);
    Sim& caps = gameSim(); caps.nSlots = bal::MAX_SLOTS; caps.slots[0].id = -1;
    caps.slots[1] = SpellSlot{(s16)restrictedSpell, SPELLS[restrictedSpell].maxLevel};
    caps.slots[2] = SpellSlot{(s16)ordinarySpell, (u8)(SPELLS[ordinarySpell].maxLevel - 1)};
    int nextId = 0;
    for (int slot = 3; slot < caps.nSlots; ++slot) {
      while (nextId == restrictedSpell || nextId == ordinarySpell) ++nextId;
      caps.slots[slot] = SpellSlot{(s16)nextId, SPELLS[nextId].maxLevel}; ++nextId;
    }
    caps.phase = PH_ALLOC; simFinishAlloc(caps);
    CHECK(caps.phase == PH_SPELL && caps.offersAreUpgrades && caps.nOffers == 1 && caps.offers[0] == 2,
          "only an under-cap ordinary spell is offered for upgrade");
    simPickOffer(caps, 0);
    CHECK(caps.slots[2].lv == SPELLS[ordinarySpell].maxLevel, "ordinary spell upgrades to its higher cap");
    const u32 upgradesAtCap = caps.runSpellUpgrades;
    caps.phase = PH_SPELL; caps.offersAreUpgrades = true; caps.nOffers = 1; caps.offers[0] = 1;
    simPickOffer(caps, 0);
    CHECK(caps.slots[1].lv == 5 && caps.runSpellUpgrades == upgradesAtCap,
          "a protected spell cannot be upgraded beyond level 5 even with a stale offer");
  }

  int flareSpell = -1;
  for (int id = 0; id < NUM_SPELLS; ++id) if (SPELLS[id].eff[0].kind == E_PERIODIC_FLARE) { flareSpell = id; break; }
  CHECK(flareSpell >= 0, "periodic flare spell exists for cast counter test");
  if (flareSpell >= 0) {
    gameStartRunForTest(SEA_FOREST, 0, true, 0, 4242u); Sim& casts = gameSim(); casts.godMode = true;
    casts.slots[casts.nSlots++] = SpellSlot{(s16)flareSpell, 1}; recompute(casts); casts.p.flareT = 100.0f;
    simUpdate(casts, SimInput{0, 0}, 1.0f / 60.0f);
    CHECK(casts.runSpellsCasted > 0, "periodic spell activation is counted");
  }

  printf("[full runs, 60 Hz logic]\n");
  double worst = 0, ms;
  for (int sea = 0; sea < SEA_COUNT; ++sea) { int w = runOne(sea, 0, false, sea % W_COUNT, 60 * 60 * 18, true, &ms); CHECK(gameSim().phase == PH_VICTORY, "season %d campaign level 1 should be completable with god mode (wave %d)", sea, w); if (ms > worst) worst = ms; }
  for (int w = 0; w < W_COUNT; ++w) { runOne(1, 2, true, w, 60 * 60 * 6, true, &ms); if (ms > worst) worst = ms; }
  printf("[balance probe: mortal autopilot (kites, never aims), campaign level 1]\n");
  for (int sea = 0; sea < SEA_COUNT; ++sea) for (int w = 0; w < W_COUNT; w += 1) { runOne(sea, 0, false, w, 60 * 60 * 18, false, &ms); }

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

  printf("[achievement progression + catalog completion]\n");
  SaveData ach; saveDefaults(ach); u32 current, target;
  achievementProgress(ach, ACH_SEASON_FOREST, current, target);
  CHECK(current == 0 && target == 3 && !achievementUnlocked(ach, ACH_SEASON_FOREST), "season achievement initial progress");
  ach.levelCleared[SEA_FOREST] = 7;
  CHECK(achievementUnlocked(ach, ACH_SEASON_FOREST), "season clear achievement");
  ach.totalKills = 10000;
  CHECK(achievementUnlocked(ach, ACH_KILLS_1000) && achievementUnlocked(ach, ACH_KILLS_10000) &&
        !achievementUnlocked(ach, ACH_KILLS_100000), "kill milestone achievements");
  for (int i = 0; i < NUM_SPELLS; ++i) if (SPELLS[i].sc == SC_SEASON && SPELLS[i].season == SEA_FOREST) ach.spellFound[i] = 1;
  for (int i = 0; i < NUM_ENEMIES; ++i) if (ENEMIES[i].season == SEA_FOREST) ach.enemySeen[i] = 1;
  CHECK(achievementUnlocked(ach, ACH_CATALOG_FOREST), "season catalog achievement");
  for (int i = 0; i < NUM_SPELLS; ++i) ach.spellFound[i] = 1;
  for (int i = 0; i < NUM_ENEMIES; ++i) ach.enemySeen[i] = 1;
  CHECK(achievementUnlocked(ach, ACH_FULL_CATALOG), "full catalog achievement");
  int byType[TY_COUNT] = {0, 0, 0};
  for (int i = 0; i < NUM_SPELLS && byType[TY_DAMAGE] < 2; ++i) if (SPELLS[i].ty == TY_DAMAGE) { ach.spellUsed[i] = 1; ++byType[TY_DAMAGE]; }
  for (int i = 0; i < NUM_SPELLS && byType[TY_STATUS] < 2; ++i) if (SPELLS[i].ty == TY_STATUS) { ach.spellUsed[i] = 1; ++byType[TY_STATUS]; }
  for (int i = 0; i < NUM_SPELLS && byType[TY_WEAPON] < 2; ++i) if (SPELLS[i].ty == TY_WEAPON) { ach.spellUsed[i] = 1; ++byType[TY_WEAPON]; }
  achievementProgress(ach, ACH_ELEMENTALIST, current, target);
  CHECK(current == TY_COUNT * 2 && target == TY_COUNT * 2 && achievementUnlocked(ach, ACH_ELEMENTALIST), "Elementalist achievement progression");

  printf("[version-3 save migration + v5 round trip]\n");
  SaveV3Fixture old = {}; old.magic = SAVE_MAGIC; old.version = 3; old.size = sizeof old;
  old.lang = LANG_EN; old.musicVol = 45; old.sfxVol = 65; old.showFps = 1;
  old.levelCleared[SEA_DESERT] = 7; old.bestWave[2] = 17; old.spellFound[140] = 9;
  old.enemyKills[3] = 22; old.enemySeen[4] = 1;
  old.crc = testCrc32((const u8*)&old + 12, sizeof old - 12);
  FILE* fixture = fopen("save.bin", "wb"); CHECK(fixture != nullptr, "open legacy fixture");
  if (fixture) { fwrite(&old, 1, sizeof old, fixture); fclose(fixture); }
  SaveData migrated; CHECK(saveLoad(migrated), "load legacy v3 save");
  CHECK(migrated.version == SAVE_VERSION && migrated.levelCleared[SEA_DESERT] == 7 && migrated.bestWave[2] == 17 &&
        migrated.spellFound[140] == 9 && migrated.spellFound[143] == 0 && migrated.enemyKills[3] == 22 &&
        migrated.enemySeen[4] && migrated.showFps == 1 && migrated.gamesPlayed == 0, "v3 progress preserved and new stats defaulted");
  SaveV4Fixture old4 = {}; old4.magic = SAVE_MAGIC; old4.version = 4; old4.size = sizeof old4;
  old4.hat = 9; old4.lastWeapon = W_STAFF; old4.spellFound[140] = 7; old4.gamesPlayed = 44; old4.totalKills = 1234;
  old4.crc = testCrc32((const u8*)&old4 + 12, sizeof old4 - 12);
  if (fixture) { fixture = fopen("save.bin", "wb"); CHECK(fixture != nullptr, "reopen v4 legacy fixture"); }
  if (fixture) { fwrite(&old4, 1, sizeof old4, fixture); fclose(fixture); }
  SaveData migrated4; CHECK(saveLoad(migrated4), "load legacy v4 save");
  CHECK(migrated4.version == SAVE_VERSION && migrated4.hat == 9 && migrated4.lastWeapon == W_STAFF &&
        migrated4.spellFound[140] == 7 && migrated4.gamesPlayed == 44 && migrated4.totalKills == 1234 &&
        migrated4.spellUsed[140] == 0, "v4 progress preserved and spell-use history starts empty");
  SaveData a; saveDefaults(a); a.bestWave[2] = 17; a.levelCleared[0] = 7; a.spellFound[5] = 3;
  a.gamesPlayed = 12; a.spellsCasted = 987; a.totalKills = 54321; CHECK(saveStore(a), "store v4 save");
  SaveData b; CHECK(saveLoad(b) && b.bestWave[2] == 17 && b.spellFound[5] == 3 && b.gamesPlayed == 12 &&
                    b.spellsCasted == 987 && b.totalKills == 54321, "load v4 stats");
  CHECK(seasonUnlocked(b, 1) && !seasonUnlocked(b, 2) && levelUnlocked(b, 0, 2), "unlock chain");
  FILE* f = fopen("save.bin", "r+b"); if (f) { fseek(f, 40, SEEK_SET); fputc(0x55, f); fclose(f); }
  CHECK(!saveLoad(b) && b.bestWave[2] == 0, "corrupt save falls back to defaults");

  gameStartRunForTest(SEA_FOREST, 0, true, W_TRIDENT, 7777u);
  Sim& trident = gameSim(); trident.pr[0].alive = true; trident.pr[0].retrievable = true; trident.pr[0].stuck = true;
  trident.pr[0].x = trident.p.x + 1.0f; trident.pr[0].y = trident.p.y; trident.p.tridentOut = true;
  simUpdate(trident, SimInput{0, 0}, 1.0f / 60.0f);
  CHECK(!trident.p.tridentOut && !trident.pr[0].alive, "trident is reclaimed within pickup range");

  printf("[audio mixer + katana swish]\n");
  s16 buf[2048]; audio::init(); audio::setVolumes(0, 100); audio::sfx(audio::SFX_SWING); audio::mix(buf, 2048);
  int nz = 0; for (int i = 0; i < 2048; ++i) nz += buf[i] != 0;
  CHECK(nz > 100, "katana swing mixer produced sound (%d)", nz);

  printf("[ui frames: all screens draw without crashing]\n");
  for (int lang = 0; lang < 2; ++lang) {
    g_lang = lang;
    gameStartRunForTest(SEA_FOREST, 0, false, 0, 888u + (u32)lang);
    CHECK(T(S_cat_weapons)[0] != 0, "catalog weapon tab is localized in language %d", lang);
    char rankLine[96];
    snprintf(rankLine, sizeof rankLine, "%s 8/10 | %s", T(S_spell_rank), Tn(S_spell_rank_gain, 140));
    CHECK(strstr(rankLine, "8/10") != 0 && strstr(rankLine, "140") != 0,
          "current spell rank and cumulative gain are localized in language %d", lang);
    CHECK(gfx::textWidth(rankLine, 1) <= 304,
          "rank summary fits the detail panel in language %d", lang);
    char customLine[96];
    snprintf(customLine, sizeof customLine, "%s 9/9 | %s", T(S_spell_rank), T(S_spell_rank_custom));
    CHECK(strstr(customLine, "9/9") != 0 && gfx::textWidth(customLine, 1) <= 304,
          "custom rank-effect summary fits in language %d", lang);
    const char* takenText = lang == LANG_EN ? "DAMAGE TAKEN" : "DANO SOFRIDO";
    CHECK(strstr(T(S_st_res_e), takenText) != 0 && gfx::textWidth(T(S_st_res_e), 1) <= 230,
          "Resistance effect clearly names damage taken and fits in language %d", lang);
    Sim& ui = gameSim();
    ui.nSlots = 2; ui.slots[1] = SpellSlot{(s16)ordinarySpell, 8}; recompute(ui);
    ui.phase = PH_PLAY;
    plat::Input spellSlotTouch; spellSlotTouch.touchDown = true; spellSlotTouch.tx = 90; spellSlotTouch.ty = 35;
    gameFrame(spellSlotTouch, 1.0f / 60.0f); gameDraw(); // selected high-rank detail fits after the description
    int timeStopId = -1; for (int id = 0; id < NUM_SPELLS; ++id) if (strcmp(spellName(id), "TIME-STOP") == 0) timeStopId = id;
    if (timeStopId >= 0) { ui.slots[1] = SpellSlot{(s16)timeStopId, 9}; recompute(ui); gameDraw(); }
    ui.phase = PH_ALLOC; ui.points = 1; gameDraw(); // stat improvement rows in both locales
    ui.phase = PH_DEAD; ui.phaseT = 1.0f;
    plat::Input nav; nav.down = plat::BTN_A; gameFrame(nav, 1.0f / 60.0f); // return to campaign
    nav.down = plat::BTN_B; gameFrame(nav, 1.0f / 60.0f);                  // return to menu
    nav.down = plat::BTN_DOWN; for (int k = 0; k < 3; ++k) gameFrame(nav, 1.0f / 60.0f);
    nav.down = plat::BTN_A; gameFrame(nav, 1.0f / 60.0f); gameDraw();
  }
  printf("[visible catalog tabs + Weapons page navigation]\n");
  for (int lang = 0; lang < 2; ++lang) {
    g_lang = lang;
    gameStartRunForTest(SEA_FOREST, 0, false, 0, 990u + (u32)lang);
    Sim& cat = gameSim(); cat.phase = PH_DEAD; cat.phaseT = 1.0f;
    plat::Input nav; nav.down = plat::BTN_A; gameFrame(nav, 1.0f / 60.0f);
    nav = plat::Input{}; nav.down = plat::BTN_B; gameFrame(nav, 1.0f / 60.0f);
    nav = plat::Input{}; nav.touchDown = true; nav.tx = 160; nav.ty = 82; gameFrame(nav, 1.0f / 60.0f); // Catalog menu item
    nav = plat::Input{}; nav.touchDown = true; nav.tx = 288; nav.ty = 10; gameFrame(nav, 1.0f / 60.0f); // Weapons tab
    nav = plat::Input{}; nav.touchDown = true; nav.tx = 24; nav.ty = 36; gameFrame(nav, 1.0f / 60.0f); // first weapon
    gameDraw();
  }
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
