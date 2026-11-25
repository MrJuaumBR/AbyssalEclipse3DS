// Run simulation: pools, enemies, projectiles, spell engine. No platform calls except audio::sfx.
#pragma once
#include "types.h"
#include "data.h"
#include "balance.h"

enum Phase : u8 { PH_PLAY, PH_CLEARED, PH_ALLOC, PH_SPELL, PH_DEAD, PH_VICTORY };
enum StatId { ST_STR, ST_RES, ST_SPD, ST_LUCK, ST_INT, ST_COUNT };
enum ZoneType : u8 { Z_NONE, Z_FIRE, Z_POISON, Z_BRAMBLE, Z_VORTEX, Z_CURSE, Z_WARN, Z_FROST, Z_CLOUD, Z_SPARK, Z_BOMB };
enum FxKind : u8 { FX_NONE, FX_TEXT, FX_RING, FX_DISC, FX_BOLT, FX_SLASH, FX_CRACK };
enum PickupKind : u8 { PICKUP_XP, PICKUP_COIN, PICKUP_POTION };

struct Ev { u8 kind; float a, b, c; s16 owner; };
enum OrbitVisual : u8 { ORBIT_FIRE, ORBIT_LIGHTNING, ORBIT_AIR };
struct Mods {
  float maxHp, move, atkSpd, cdMul, dmgMul, flat, crit, critDmg, critBonus, sizeMul, aoeMul, expMul, rareBonus;
  float rangeMul, projSpd, taken, rangedTaken, debuff, lifesteal, reflect, dodge, mirror, healMul, dealt, attackerDmg;
  float killHeal, killHealCd, regenHp, regenInt, vsSlowed, vsBurning, vsWounded, vsFrozen, lowHpBonus, lowHpThresh, lightning;
  float explodeDmg, explodeR, splitDmg, bounceDmg, shieldWave, revivePct, warmth, hpPerWave, burnTouchDps, burnTouchDur, whirlDps, whirlDur, fairyPower;
  float knock, armorBonus, dronePower, sentinelPower, allNumPct;
  int pierce, bounceE, bounceW, split, blockWave, chainPlus, homingLevel;
  bool homing, armorIgnore, immuneSlow, hasDrone, hasSentinel, hasFairy;
  int nEv; Ev ev[bal::MAX_EV];
  int nOrbit; float orbitDps; OrbitVisual orbitVisual = ORBIT_AIR;
};
struct SpellSlot { s16 id; u8 lv; };        // id >= 0: spell index; id < 0: weapon pseudo-spell (-1 - weaponId)

struct Enemy {
  bool alive; u8 type, state, phase; bool small, revived, split, telegraph;
  float x, y, vx, vy, kx, ky, hp, maxhp, dmg, spd, atkT, t0, t1, t2, dirx, diry, face;
  float slowPct, slowT, freezeT, stunT, rootT, burnDps, burnT, bleedDps, bleedT, markT, markPct, flash;
};
struct Proj {
  bool alive, hostile, lob, crit, homing, retrievable, stuck; u8 spr, kind, bounceE, bounceW; s16 hit[4]; u8 nhit; int pierce;
  float x, y, vx, vy, dmg, r, life, tx, ty, aoe, rot, homingRate;
};
struct Gem { bool alive; float x, y, v; u8 kind; };
struct Zone { u8 type; float x, y, r, t, dps, a; };
struct Fx { u8 kind, col; float x, y, t, life, r, ang; char txt[8]; };
struct Orbiter { float ang; };

struct Player {
  float x, y, hp, maxhp, iframes, slowT, slowPct, burnT, shield, lsBudget, critHealT, killHealT, regenT, flareT, trailT, auraT, moveBonus, maxHpBonus, hurtFlash;
  int level, kills, throws, blocks; float exp; bool usedRevive; float atkT; float face;
  float droneAngle, droneFireT, bombT, timeStopT, timeStopCd; bool tridentOut; u8 infinityElement = 0;
};

struct Sentinel { bool active; float x, y, hp, maxhp, fireT, iframes; };
struct Fairy { bool active; float x, y, healT, phase; };

struct SimInput { float mx, my; };   // movement vector, each -1..1

struct Sim {
  // configuration
  u8 season = 0, levelIdx = 0, weapon = 0, selectedSeasons = 0x1F; bool endless = false, allSeasons = false, randomSpells = true; int totalWaves = 10, difficulty = 1;
  int lives = 1, runCoins = 0, startingSpell = -1;
  float camX = 0.0f, camY = 0.0f;
  Rng rng;
  // run state
  Phase phase = PH_PLAY;
  int wave = 1, points = 0, bankedPoints = 0, bankStores = 0;
  int alloc[ST_COUNT] = {0, 0, 0, 0, 0}; int drain[ST_COUNT] = {0, 0, 0, 0, 0};
  SpellSlot slots[bal::MAX_SLOTS]; int nSlots = 0;
  int offers[3]; int nOffers = 0; bool offersAreUpgrades = false;
  s16 offerPool[NUM_SPELLS] = {}; int offerPoolCount = 0, offerPage = 0;
  u8 killProgress[NUM_SPELLS] = {}; float killCooldown[NUM_SPELLS] = {};
  float waveT = 0, runT = 0, phaseT = 0, spawnAcc = 0, bossTimer = 0, hazardT = 0, heatT = 0, shake = 0; bool shakeEnabled = true;
  int spawned = 0, quota = 0, totalKills = 0, enemiesAlive = 0;
  bool bossWave = false, bossSpawned = false, godMode = false, newBest = false, forced = false;
  bool sacrificedThisWave = false;
  u8 pendingHitProc[bal::MAX_EV] = {};
  Player p; Mods m;
  Sentinel sentinels[2]; float sentinelSpawnT = 0.0f; Fairy fairy;
  Enemy en[bal::MAX_ENEMIES]; Proj pr[bal::MAX_PROJ]; Gem gem[bal::MAX_GEMS]; Zone zn[bal::MAX_ZONES]; Fx fx[bal::MAX_FX];
  Orbiter orb[bal::MAX_ORBIT]; int nOrb = 0;
  u32 frame = 0;
  u32 runSpellsAcquired = 0, runSpellsCasted = 0, runSpellUpgrades = 0;
  // outputs for the meta layer
  u16 runKills[NUM_ENEMIES]; bool seenEnemy[NUM_ENEMIES]; u8 seenSpell[NUM_SPELLS]; u8 usedSpell[NUM_SPELLS];
  char toast[40]; float toastT = 0;
};

void simStart(Sim& s, int season, int levelIdx, bool endless, int weapon, u32 seed, bool allSeasons = false, u8 selectedSeasons = 0x1F, bool randomSpells = true, int lives = 1, int startingSpell = -1, int difficulty = 1);
void simUpdate(Sim& s, const SimInput& in, float dt);
void simDraw(Sim& s, bool showFps, int fps); // draws the battlefield + HUD on the TOP screen
int  statValue(const Sim& s, int stat);     // BASE + allocated - drained
bool simSpend(Sim& s, int stat);            // spend one point (ALLOC phase)
bool simStorePoints(Sim& s);                // defer points; at most three stores per run
bool simUseStoredPoints(Sim& s);            // retrieve deferred points during ALLOC
void simFinishAlloc(Sim& s);                // -> SPELL phase (or next wave)
void simPickOffer(Sim& s, int i);           // choose offer i (spell or upgrade); i<0 skips
void simCycleOffers(Sim& s, int direction); // page through the full list when random offers are disabled
void simResume(Sim& s);                     // starts the next wave from CLEARED/SPELL
int  simSpellLevelOf(const Sim& s, int spellId);
int  simSpellTriggerInterval(const Sim& s, int slotIndex);
bool simKillTriggerInfo(const Sim& s, int slotIndex, int& progress, int& target, float& cooldown, float& cooldownMax);
float simPotionDropChance(const Sim& s, bool critical);
int  simSacrificeReward(const Sim& s, int slotIndex);
bool simSacrificeSpell(Sim& s, int slotIndex);
// debug (deterministic + reversible, design doc "Debug Tools")
void dbgWaveEnd(Sim& s); void dbgBoss(Sim& s); void dbgForceSpell(Sim& s); void dbgClearSpells(Sim& s);
void dbgDeath(Sim& s); void dbgGiveLevel(Sim& s); void dbgGrantSpell(Sim& s, int spellId); void dbgSpawnGolem(Sim& s); void dbgCompleteLevel(Sim& s); void dbgAddMinute(Sim& s);
const char* phaseName(Phase p);
void recompute(Sim& s);
