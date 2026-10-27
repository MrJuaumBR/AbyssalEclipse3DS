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

// Previous format: FPS counter option and 138 spell discoveries.
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
  memcpy(s.spellFound, old.spellFound, sizeof old.spellFound); // new spell slots remain zero
  memcpy(s.enemyKills, old.enemyKills, sizeof s.enemyKills);
  memcpy(s.enemySeen, old.enemySeen, sizeof s.enemySeen);
}
bool saveLoad(SaveData& s) {
  u8 raw[sizeof(SaveData)] = {}; size_t got = 0;
  if (plat::storageRead("save.bin", raw, sizeof raw, &got)) {
    if (got == sizeof(SaveData)) {
      SaveData t; memcpy(&t, raw, sizeof t);
      if (t.magic == SAVE_MAGIC && t.version == SAVE_VERSION && t.size == sizeof(SaveData) && t.crc == calc(t) &&
          t.lang < LANG_COUNT && t.hat < 5 && t.face < 5 && t.color < 6 && t.lastWeapon < 4) {
        s = t; return true;
      }
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
