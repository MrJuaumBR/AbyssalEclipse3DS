#include "save.h"
#include "platform.h"
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

bool saveLoad(SaveData& s) {
  u8 raw[sizeof(SaveData)] = {}; size_t got = 0;
  if (plat::storageRead("save.bin", raw, sizeof raw, &got)) {
    if (got == sizeof(SaveData)) {
      SaveData t; memcpy(&t, raw, sizeof t);
      if (t.magic == SAVE_MAGIC && t.version == SAVE_VERSION && t.size == sizeof(SaveData) && t.crc == calc(t) &&
          t.lang < LANG_COUNT && t.hat < 10 && t.face < 6 && t.color < 6 && t.lastWeapon < W_COUNT) {
        s = t; return true;
      }
    }
    if (got == sizeof(SaveDataV4)) {
      SaveDataV4 old; memcpy(&old, raw, sizeof old);
      if (validV4(old)) { migrateV4(s, old); (void)saveStore(s); return true; }
    }
    if (got == sizeof(SaveDataV3)) {
      SaveDataV3 old; memcpy(&old, raw, sizeof old);
      if (validV3(old)) { migrateV3(s, old); (void)saveStore(s); return true; }
    }
    if (got == sizeof(SaveDataV2)) {
      SaveDataV2 old; memcpy(&old, raw, sizeof old);
      if (validV2(old)) { migrateV2(s, old); (void)saveStore(s); return true; }
    }
    if (got == sizeof(SaveDataV1)) {
      SaveDataV1 old; memcpy(&old, raw, sizeof old);
      if (validV1(old)) { migrateV1(s, old); (void)saveStore(s); return true; }
    }
  }
  saveDefaults(s);
  return false;
}
bool saveStore(SaveData& s) {
  s.magic = SAVE_MAGIC; s.version = SAVE_VERSION; s.size = sizeof(SaveData); s.crc = calc(s);
  return plat::storageWrite("save.bin", &s, sizeof s);
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
  } else if (id >= ACH_CLOTH_NIGHTWARDEN && id <= ACH_CLOTH_ARCWARDEN) {
    switch (id) {
      case ACH_CLOTH_NIGHTWARDEN: current = s.playTimeSeconds; target = 3600; break;
      case ACH_CLOTH_GRINMASK: current = s.playTimeSeconds; target = 10 * 3600; break;
      case ACH_CLOTH_REDCAP: current = s.totalKills; target = 1000; break;
      default:
        for (int i = 0; i < NUM_SPELLS; ++i) if (s.spellFound[i]) ++current;
        for (int i = 0; i < NUM_ENEMIES; ++i) if (s.enemySeen[i]) ++current;
        target = NUM_SPELLS + NUM_ENEMIES; break;
    }
    if (current > target) current = target;
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
