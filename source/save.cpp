#include "save.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>

static u32 crc32(const u8* p, size_t n) {
  u32 c = 0xFFFFFFFFu;
  for (size_t i = 0; i < n; ++i) { c ^= p[i]; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
  return ~c;
}
void saveDefaults(SaveData& s) {
  memset(&s, 0, sizeof s);
  s.magic = SAVE_MAGIC; s.version = SAVE_VERSION; s.size = sizeof(SaveData);
  s.musicVol = 50; s.sfxVol = 70; s.face = 4; s.lang = LANG_EN; s.showFps = 0;
}
static u32 calc(const SaveData& s) { return crc32((const u8*)&s + 12, sizeof(SaveData) - 12); }

// Original format: no FPS counter option and 137 spell discoveries.
struct SaveDataV1 {
  u32 magic; u16 version; u16 size; u32 crc;
  u8 lang, musicVol, sfxVol, fps30;
  u8 hat, face, color, lastWeapon;
  u8 unlockAll, pad0, pad1, pad2;
  u8 levelCleared[SEA_COUNT]; u8 pad3[3];
  u16 bestWave[SEA_COUNT]; u16 pad4[3];
  u16 spellFound[137];
  u16 enemyKills[NUM_ENEMIES];
  u8 enemySeen[NUM_ENEMIES + 3];
};
static_assert(sizeof(SaveDataV1) == 400, "version-1 save layout changed");
static u32 calcV1(const SaveDataV1& s) { return crc32((const u8*)&s + 12, sizeof(SaveDataV1) - 12); }
static bool validV1(const SaveDataV1& t) {
  return t.magic == SAVE_MAGIC && t.version == 1 && t.size == sizeof(SaveDataV1) && t.crc == calcV1(t) &&
      t.lang < LANG_COUNT && t.hat < 5 && t.face < 5 && t.color < 6 && t.lastWeapon < 4;
}

// Version 2 added the FPS option and a 138th spell discovery.
struct SaveDataV2 {
  u32 magic; u16 version; u16 size; u32 crc;
  u8 lang, musicVol, sfxVol, fps30;
  u8 showFps;
  u8 hat, face, color, lastWeapon;
  u8 unlockAll, pad0, pad1, pad2;
  u8 levelCleared[SEA_COUNT]; u8 pad3[3];
  u16 bestWave[SEA_COUNT]; u16 pad4[3];
  u16 spellFound[138];
  u16 enemyKills[NUM_ENEMIES];
  u8 enemySeen[NUM_ENEMIES + 3];
};
static_assert(sizeof(SaveDataV2) == 404, "version-2 save layout changed");
static u32 calcV2(const SaveDataV2& s) { return crc32((const u8*)&s + 12, sizeof(SaveDataV2) - 12); }
static bool validV2(const SaveDataV2& t) {
  return t.magic == SAVE_MAGIC && t.version == 2 && t.size == sizeof(SaveDataV2) && t.crc == calcV2(t) &&
      t.lang < LANG_COUNT && t.hat < 5 && t.face < 5 && t.color < 6 && t.lastWeapon < 4;
}

// Version 3 is the current 141-spell record before lifetime counters were added.
struct SaveDataV3 {
  u32 magic; u16 version; u16 size; u32 crc;
  u8 lang, musicVol, sfxVol, fps30;
  u8 showFps;
  u8 hat, face, color, lastWeapon;
  u8 unlockAll, pad0, pad1, pad2;
  u8 levelCleared[SEA_COUNT]; u8 pad3[3];
  u16 bestWave[SEA_COUNT]; u16 pad4[3];
  u16 spellFound[141];
  u16 enemyKills[NUM_ENEMIES];
  u8 enemySeen[NUM_ENEMIES + 3];
};
static_assert(sizeof(SaveDataV3) == 412, "version-3 save layout changed");
static u32 calcV3(const SaveDataV3& s) { return crc32((const u8*)&s + 12, sizeof(SaveDataV3) - 12); }
static bool validV3(const SaveDataV3& t) {
  return t.magic == SAVE_MAGIC && t.version == 3 && t.size == sizeof(SaveDataV3) && t.crc == calcV3(t) &&
      t.lang < LANG_COUNT && t.hat < 5 && t.face < 5 && t.color < 6 && t.lastWeapon < 4;
}

// Version 4 is the previous current record: lifetime totals, but no distinct spell-use history.
struct SaveDataV4 {
  u32 magic; u16 version; u16 size; u32 crc;
  u8 lang, musicVol, sfxVol, fps30;
  u8 showFps;
  u8 hat, face, color, lastWeapon;
  u8 unlockAll, pad0, pad1, pad2;
  u8 levelCleared[SEA_COUNT]; u8 pad3[3];
  u16 bestWave[SEA_COUNT]; u16 pad4[3];
  u16 spellFound[144];
  u16 enemyKills[NUM_ENEMIES];
  u8 enemySeen[NUM_ENEMIES + 3];
  u32 gamesPlayed, runsWon, runsLost, totalKills;
  u32 spellsAcquired, spellsCasted, spellUpgrades, weaponThrows, playTimeSeconds;
};
static_assert(sizeof(SaveDataV4) == 452, "version-4 save layout changed");
static u32 calcV4(const SaveDataV4& s) { return crc32((const u8*)&s + 12, sizeof(SaveDataV4) - 12); }
static bool validV4(const SaveDataV4& t) {
  return t.magic == SAVE_MAGIC && t.version == 4 && t.size == sizeof(SaveDataV4) && t.crc == calcV4(t) &&
      t.lang < LANG_COUNT && t.hat < 10 && t.face < 6 && t.color < 6 && t.lastWeapon < 4;
}

static void migrateV1(SaveData& s, const SaveDataV1& old) {
  saveDefaults(s);
  s.lang = old.lang; s.musicVol = old.musicVol; s.sfxVol = old.sfxVol; s.fps30 = old.fps30;
  s.hat = old.hat; s.face = old.face; s.color = old.color; s.lastWeapon = old.lastWeapon; s.unlockAll = old.unlockAll;
  memcpy(s.levelCleared, old.levelCleared, sizeof s.levelCleared);
  memcpy(s.bestWave, old.bestWave, sizeof s.bestWave);
  memcpy(s.spellFound, old.spellFound, sizeof old.spellFound);
  memcpy(s.enemyKills, old.enemyKills, sizeof s.enemyKills);
  memcpy(s.enemySeen, old.enemySeen, sizeof s.enemySeen);
}
static void migrateV2(SaveData& s, const SaveDataV2& old) {
  saveDefaults(s);
  s.lang = old.lang; s.musicVol = old.musicVol; s.sfxVol = old.sfxVol; s.fps30 = old.fps30; s.showFps = old.showFps;
  s.hat = old.hat; s.face = old.face; s.color = old.color; s.lastWeapon = old.lastWeapon; s.unlockAll = old.unlockAll;
  memcpy(s.levelCleared, old.levelCleared, sizeof s.levelCleared);
  memcpy(s.bestWave, old.bestWave, sizeof s.bestWave);
  memcpy(s.spellFound, old.spellFound, sizeof old.spellFound); // later spell slots remain zero
  memcpy(s.enemyKills, old.enemyKills, sizeof s.enemyKills);
  memcpy(s.enemySeen, old.enemySeen, sizeof s.enemySeen);
}
static void migrateV3(SaveData& s, const SaveDataV3& old) {
  saveDefaults(s);
  s.lang = old.lang; s.musicVol = old.musicVol; s.sfxVol = old.sfxVol; s.fps30 = old.fps30; s.showFps = old.showFps;
  s.hat = old.hat; s.face = old.face; s.color = old.color; s.lastWeapon = old.lastWeapon; s.unlockAll = old.unlockAll;
  memcpy(s.levelCleared, old.levelCleared, sizeof s.levelCleared);
  memcpy(s.bestWave, old.bestWave, sizeof s.bestWave);
  memcpy(s.spellFound, old.spellFound, sizeof old.spellFound);
  memcpy(s.enemyKills, old.enemyKills, sizeof s.enemyKills);
  memcpy(s.enemySeen, old.enemySeen, sizeof s.enemySeen);
}
static void migrateV4(SaveData& s, const SaveDataV4& old) {
  saveDefaults(s);
  s.lang = old.lang; s.musicVol = old.musicVol; s.sfxVol = old.sfxVol; s.fps30 = old.fps30; s.showFps = old.showFps;
  s.hat = old.hat; s.face = old.face; s.color = old.color; s.lastWeapon = old.lastWeapon; s.unlockAll = old.unlockAll;
  memcpy(s.levelCleared, old.levelCleared, sizeof s.levelCleared);
  memcpy(s.bestWave, old.bestWave, sizeof s.bestWave);
  memcpy(s.spellFound, old.spellFound, sizeof old.spellFound);
  memcpy(s.enemyKills, old.enemyKills, sizeof s.enemyKills);
  memcpy(s.enemySeen, old.enemySeen, sizeof s.enemySeen);
  s.gamesPlayed = old.gamesPlayed; s.runsWon = old.runsWon; s.runsLost = old.runsLost; s.totalKills = old.totalKills;
  s.spellsAcquired = old.spellsAcquired; s.spellsCasted = old.spellsCasted; s.spellUpgrades = old.spellUpgrades;
  s.weaponThrows = old.weaponThrows; s.playTimeSeconds = old.playTimeSeconds;
}

// The two alternating records each carry their own generation and checksum.
// A write only replaces the older/inactive slot, leaving the newest valid copy
// untouched if power is lost during the SD operation.
struct SaveSlot { u32 magic, generation, crc; SaveData data; };
static const u32 SAVE_SLOT_MAGIC = 0x534C4F54u; // 'SLOT'
static int g_activeSlot = -1;
static u32 g_saveGeneration = 0;
static u32 slotCalc(const SaveSlot& slot) {
  u8 payload[sizeof(slot.generation) + sizeof(slot.data)];
  memcpy(payload, &slot.generation, sizeof slot.generation);
  memcpy(payload + sizeof slot.generation, &slot.data, sizeof slot.data);
  return crc32(payload, sizeof payload);
}
static bool validCurrent(const SaveData& t) {
  return t.magic == SAVE_MAGIC && t.version == SAVE_VERSION && t.size == sizeof(SaveData) && t.crc == calc(t) &&
      t.lang < LANG_COUNT && t.hat < 10 && t.face < 6 && t.color < 6 && t.lastWeapon < W_COUNT;
}
// Version 5 used the same record size; the four bytes now called `coins` were padding.
static bool validV5(const SaveData& t) {
  return t.magic == SAVE_MAGIC && t.version == 5 && t.size == sizeof(SaveData) && t.crc == calc(t) &&
      t.lang < LANG_COUNT && t.hat < 10 && t.face < 6 && t.color < 6 && t.lastWeapon < W_COUNT;
}
static void migrateV5(SaveData& out, const SaveData& old) {
  out = old; out.coinsLo = out.coinsHi = 0; out.coinSeasonClaimed = 0; out.version = SAVE_VERSION; out.crc = 0;
}
static bool validSlot(const SaveSlot& slot) {
  return slot.magic == SAVE_SLOT_MAGIC && slot.crc == slotCalc(slot) && validCurrent(slot.data);
}
static bool readSlot(int index, SaveSlot& best, bool& found, bool& sawFile) {
  static const char* const names[2] = {"save.0", "save.1"};
  char tmpName[16]; snprintf(tmpName, sizeof tmpName, "%s.tmp", names[index]);
  const char* candidates[2] = {names[index], tmpName};
  bool gotOne = false;
  for (int c = 0; c < 2; ++c) {
    u8 raw[sizeof(SaveSlot)] = {}; size_t got = 0;
    if (!plat::storageRead(candidates[c], raw, sizeof raw, &got)) continue;
    sawFile = true;
    if (got != sizeof(SaveSlot)) continue;
    SaveSlot slot; memcpy(&slot, raw, sizeof slot);
    if (!validSlot(slot)) continue;
    if (!found || slot.generation > best.generation) { best = slot; g_activeSlot = index; found = true; }
    gotOne = true;
  }
  return gotOne;
}
static bool decodeLegacy(const u8* raw, size_t got, SaveData& out) {
  if (got == sizeof(SaveData)) {
    SaveData t; memcpy(&t, raw, sizeof t);
    if (validCurrent(t)) { out = t; return true; }
    if (validV5(t)) { migrateV5(out, t); return true; }
  }
  if (got == sizeof(SaveDataV4)) { SaveDataV4 old; memcpy(&old, raw, sizeof old); if (validV4(old)) { migrateV4(out, old); return true; } }
  if (got == sizeof(SaveDataV3)) { SaveDataV3 old; memcpy(&old, raw, sizeof old); if (validV3(old)) { migrateV3(out, old); return true; } }
  if (got == sizeof(SaveDataV2)) { SaveDataV2 old; memcpy(&old, raw, sizeof old); if (validV2(old)) { migrateV2(out, old); return true; } }
  if (got == sizeof(SaveDataV1)) { SaveDataV1 old; memcpy(&old, raw, sizeof old); if (validV1(old)) { migrateV1(out, old); return true; } }
  return false;
}
SaveLoadStatus saveLoadEx(SaveData& s) {
  SaveSlot best = {}; bool found = false, sawFile = false;
  g_activeSlot = -1; g_saveGeneration = 0;
  readSlot(0, best, found, sawFile); readSlot(1, best, found, sawFile);
  if (found) { s = best.data; g_saveGeneration = best.generation; return SAVE_LOAD_OK; }

  // Read the old single-file format only when no valid slot exists. Keep that
  // source file in place; migration writes a verified new slot separately.
  const char* legacyNames[2] = {"save.bin", "save.bin.tmp"};
  for (int i = 0; i < 2; ++i) {
    u8 raw[sizeof(SaveData)] = {}; size_t got = 0;
    if (!plat::storageRead(legacyNames[i], raw, sizeof raw, &got)) continue;
    sawFile = true;
    if (!decodeLegacy(raw, got, s)) continue;
    (void)saveStore(s);
    return SAVE_LOAD_MIGRATED;
  }
  saveDefaults(s);
  return sawFile ? SAVE_LOAD_RECOVERY_REQUIRED : SAVE_LOAD_FRESH;
}
bool saveLoad(SaveData& s) {
  const SaveLoadStatus result = saveLoadEx(s);
  return result == SAVE_LOAD_OK || result == SAVE_LOAD_MIGRATED;
}
bool saveStore(SaveData& s) {
  s.magic = SAVE_MAGIC; s.version = SAVE_VERSION; s.size = sizeof(SaveData); s.crc = calc(s);
  SaveSlot slot = {};
  slot.magic = SAVE_SLOT_MAGIC;
  slot.generation = g_saveGeneration + 1;
  if (slot.generation == 0) slot.generation = 1;
  slot.data = s;
  slot.crc = slotCalc(slot);
  const int target = g_activeSlot < 0 ? 0 : 1 - g_activeSlot;
  const char* name = target == 0 ? "save.0" : "save.1";
  if (!plat::storageWrite(name, &slot, sizeof slot)) return false;
  u8 checkRaw[sizeof(SaveSlot)] = {}; size_t got = 0;
  if (!plat::storageRead(name, checkRaw, sizeof checkRaw, &got) || got != sizeof(SaveSlot)) return false;
  SaveSlot check; memcpy(&check, checkRaw, sizeof check);
  if (!validSlot(check) || check.generation != slot.generation) return false;
  g_activeSlot = target; g_saveGeneration = slot.generation;
  // Keep a readable v6 recovery copy for tools and for first-launch migration.
  // Failure here is non-fatal: the verified rotating slots are already safe.
  (void)plat::storageWrite("save.bin", &s, sizeof s);
  return true;
}
bool seasonUnlocked(const SaveData& s, int sea) { return s.unlockAll || sea == 0 || (s.levelCleared[sea - 1] & 4); }
bool levelUnlocked(const SaveData& s, int sea, int lv) {
  if (!seasonUnlocked(s, sea)) return false;
  return s.unlockAll || lv == 0 || (s.levelCleared[sea] & (1 << (lv - 1)));
}

static u32 countSeasonCatalog(const SaveData& s, int season, u32& target) {
  u32 found = 0; target = 0;
  for (int i = 0; i < NUM_SPELLS; ++i) if (SPELLS[i].sc == SC_SEASON && SPELLS[i].season == season) {
    ++target; if (s.spellFound[i] > 0) ++found;
  }
  for (int i = 0; i < NUM_ENEMIES; ++i) if (ENEMIES[i].season == season) {
    ++target; if (s.enemySeen[i]) ++found;
  }
  return found;
}
void achievementProgress(const SaveData& s, int id, u32& current, u32& target) {
  current = 0; target = 1;
  if (id >= ACH_SEASON_FOREST && id <= ACH_SEASON_ECLIPSE) {
    const int sea = id - ACH_SEASON_FOREST;
    const u8 bits = s.levelCleared[sea] & 7;
    current = (u32)((bits & 1) != 0) + (u32)((bits & 2) != 0) + (u32)((bits & 4) != 0);
    target = 3;
  } else if (id >= ACH_KILLS_1000 && id <= ACH_KILLS_100000) {
    static const u32 goals[3] = {1000, 10000, 100000};
    target = goals[id - ACH_KILLS_1000]; current = s.totalKills < target ? s.totalKills : target;
  } else if (id >= ACH_CATALOG_FOREST && id <= ACH_CATALOG_ECLIPSE) {
    current = countSeasonCatalog(s, id - ACH_CATALOG_FOREST, target);
  } else if (id == ACH_FULL_CATALOG) {
    for (int i = 0; i < NUM_SPELLS; ++i) if (s.spellFound[i]) ++current;
    for (int i = 0; i < NUM_ENEMIES; ++i) if (s.enemySeen[i]) ++current;
    target = NUM_SPELLS + NUM_ENEMIES;
  } else if (id >= ACH_LITTLE_DUMMY && id <= ACH_TOUCH_GRASS) {
    static const u32 hours[3] = {1, 10, 50};
    target = hours[id - ACH_LITTLE_DUMMY] * 3600;
    current = s.playTimeSeconds < target ? s.playTimeSeconds : target;
  } else if (id == ACH_START) {
    current = s.bestAllSeasonsWave;
    for (int sea = 0; sea < SEA_COUNT; ++sea) current = maxT(current, (u32)s.bestWave[sea]);
    target = 30;
  } else if (id == ACH_INFINITY) {
    current = s.bestAllSeasonsWave; for (int sea = 0; sea < SEA_COUNT; ++sea) current = maxT(current, (u32)s.bestWave[sea]); target = 50;
  } else if (id == ACH_UNSTOPPABLE) {
    current = s.bestAllSeasonsWave; for (int sea = 0; sea < SEA_COUNT; ++sea) current = maxT(current, (u32)s.bestWave[sea]); target = 70;
  } else if (id == ACH_ELEMENTALIST) {
    int byType[TY_COUNT] = {0, 0, 0};
    for (int i = 0; i < NUM_SPELLS; ++i) if (s.spellUsed[i]) ++byType[SPELLS[i].ty];
    for (int t = 0; t < TY_COUNT; ++t) current += minT(byType[t], 2);
    target = TY_COUNT * 2;
  }
}
bool achievementUnlocked(const SaveData& s, int id) {
  u32 current, target; achievementProgress(s, id, current, target); return current >= target;
}
