// All tunable numbers live here (see skill: "Tune only here").
#pragma once
#include "types.h"

namespace bal {
static const float BASE_HP = 60.0f;
static const int   BASE_STAT[5] = {8, 5, 4, 3, 5};          // STR RES SPD LUCK INT (zero point of the bonus)
static const float PLAYER_SPEED = 62.0f;                    // px/s
static const float PLAYER_RADIUS = 5.0f;
static const float IFRAMES = 0.6f;
static const float WAVE_LEN = 40.0f;
static const int   MAX_SLOTS = 12;
static const int   MAX_SPELL_LV = 5;
static const float LV_BONUS = 0.20f;                        // +20% numeric effects per upgrade
static const float MAGNET_R = 38.0f, PICKUP_R = 9.0f;
static const float EXP_GAIN_MUL = 1.05f;                    // v0.1 release: +5% global XP
static const float BASE_CRIT = 5.0f, CRIT_MUL = 1.5f;
inline int expNeed(int level) { return 10 + (level - 1) * 5; }

// Descriptor words -> numbers (px/s and seconds)
static const float SPEED_WORD[7] = {0, 14, 22, 28, 36, 52, 70};   // STATIONARY..VERY FAST
static const float ATK_WORD[5] = {2.4f, 1.6f, 1.3f, 1.0f, 0.7f};  // VERY SLOW..FAST (seconds between attacks)
static const float RANGE_WORD[3] = {0, 140, 190};                 // TOUCH / HIGH / VERY HIGH
static const float ENEMY_RADIUS = 6.0f;

// Weapons: dmg, interval(s), range(px), speed(px/s), pierce(extra), radius
struct WeaponStat { float dmg, interval, range, speed; int pierce; float radius; };
static const WeaponStat WEAPON[4] = {
    {3.0f, 0.28f, 90.0f, 250.0f, 0, 3.0f},    // DAGGER  low dmg, very high rate, low range
    {6.0f, 0.65f, 210.0f, 310.0f, 0, 3.0f},   // BOW     normal, very high range
    {11.0f, 1.15f, 140.0f, 150.0f, 0, 22.0f}, // AXE     lobbed, AoE on landing (radius = impact radius)
    {24.0f, 1.90f, 160.0f, 90.0f, 1, 8.0f}};  // STAFF   very high dmg, very slow

// Caps (fixed pools)
enum { MAX_ENEMIES = 48, MAX_PROJ = 72, MAX_GEMS = 48, MAX_FX = 32, MAX_ZONES = 14, MAX_ORBIT = 6, MAX_EV = 40 };

// Wave director
inline float hpScale(int wave, int levelTier) { return (1.0f + 0.12f * (wave - 1)) * (1.0f + 0.35f * levelTier); }
inline float dmgScale(int wave, int levelTier) { return (1.0f + 0.04f * (wave - 1)) * (1.0f + 0.15f * levelTier); }
inline int waveQuota(int wave) { return 12 + 4 * wave; }
}  // namespace bal
