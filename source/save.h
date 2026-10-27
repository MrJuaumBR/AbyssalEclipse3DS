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
  u16 enemyKills[NUM_ENEMIES];
  u8 enemySeen[NUM_ENEMIES + 3];
};
enum { SAVE_MAGIC = 0x41453344u /* 'AE3D' */, SAVE_VERSION = 3 };
void saveDefaults(SaveData& s);
bool saveLoad(SaveData& s);          // true if a valid file was read; otherwise defaults are set
bool saveStore(SaveData& s);         // false => SD problem (caller shows a toast, never crashes)
bool seasonUnlocked(const SaveData& s, int season);
bool levelUnlocked(const SaveData& s, int season, int level);
