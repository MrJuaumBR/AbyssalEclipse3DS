// Versioned, CRC-checked save file (< 1 KB). Written only on events, atomically.
#pragma once
#include "types.h"
#include "data.h"

struct SaveData {
  u32 magic; u16 version; u16 size; u32 crc;          // header; crc covers everything after the header
  u8 lang, musicVol, sfxVol, fps30;
  u8 showFps;
  u8 hat, face, color, lastWeapon;
  u8 unlockAll, pad0, pad1, pad2;
  u8 levelCleared[SEA_COUNT];                         // bit n = level n+1 cleared
  u8 pad3[3];
  u16 bestWave[SEA_COUNT]; u16 pad4[3];
  u16 spellFound[NUM_SPELLS];
  u8 spellUsed[NUM_SPELLS];
  u16 enemyKills[NUM_ENEMIES];
  u8 enemySeen[NUM_ENEMIES + 3];
  // Lifetime totals; saturate instead of wrapping. Older saves migrate these as zero.
  u32 gamesPlayed, runsWon, runsLost, totalKills;
  u32 spellsAcquired, spellsCasted, spellUpgrades, weaponThrows, playTimeSeconds;
};
enum { SAVE_MAGIC = 0x41453344u /* 'AE3D' */, SAVE_VERSION = 5 };

enum AchievementId {
  ACH_SEASON_FOREST, ACH_SEASON_DESERT, ACH_SEASON_SNOW, ACH_SEASON_MOUNTAIN, ACH_SEASON_ECLIPSE,
  ACH_KILLS_1000, ACH_KILLS_10000, ACH_KILLS_100000,
  ACH_CATALOG_FOREST, ACH_CATALOG_DESERT, ACH_CATALOG_SNOW, ACH_CATALOG_MOUNTAIN, ACH_CATALOG_ECLIPSE,
  ACH_FULL_CATALOG,
  ACH_LITTLE_DUMMY, ACH_ABYSS_VETERAN, ACH_TOUCH_GRASS,
  ACH_CLOTH_NIGHTWARDEN, ACH_CLOTH_GRINMASK, ACH_CLOTH_REDCAP, ACH_CLOTH_ARCWARDEN,
  ACH_ELEMENTALIST,
  ACH_COUNT
};

void saveDefaults(SaveData& s);
bool saveLoad(SaveData& s);          // true if a valid file was read; otherwise defaults are set
bool saveStore(SaveData& s);         // false => SD problem (caller shows a toast, never crashes)
bool seasonUnlocked(const SaveData& s, int season);
bool levelUnlocked(const SaveData& s, int season, int level);
void achievementProgress(const SaveData& s, int id, u32& current, u32& target);
bool achievementUnlocked(const SaveData& s, int id);
