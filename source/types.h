// Abyssal Eclipse 3DS - basic types. No platform headers allowed here.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32;
#define ARRAY_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

// Colour packing: bytes in memory are R,G,B,A (same as citro2d C2D_Color32).
static inline u32 RGBA(u32 r, u32 g, u32 b, u32 a = 255) { return r | (g << 8) | (b << 16) | (a << 24); }
static inline u32 HEX(u32 rgb, u32 a = 255) { return RGBA((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255, a); }
static inline u32 withAlpha(u32 c, u32 a) { return (c & 0x00FFFFFFu) | (a << 24); }

template <class T> static inline T clampT(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }
template <class T> static inline T minT(T a, T b) { return a < b ? a : b; }
template <class T> static inline T maxT(T a, T b) { return a > b ? a : b; }

// Screen geometry
enum { TOP_W = 400, TOP_H = 240, BOT_W = 320, BOT_H = 240 };

// Palette from the design document (flat colours only).
enum : u32 {
  C_VOID = 0x0B0A12, C_PANEL = 0x1E1A2E, C_BORDER = 0x3A3352, C_TEXT = 0xF0EAD6, C_DIM = 0x9B92B0,
  C_GOLD = 0xE8B54B, C_RED = 0xC43D3D, C_GREEN = 0x4FA85A, C_EPIC = 0x8B5FBF, C_RARE = 0x4A7FC1, C_LEG = 0xF08030,
  C_FOREST = 0x3F7A3A, C_DESERT = 0xF0C860, C_SNOW = 0x7AB8D9, C_MOUNTAIN = 0x8A8478, C_ECLIPSE = 0xB83A8A
};

// Small deterministic RNG (xorshift32).
struct Rng {
  u32 s = 0x9E3779B9u;
  u32 next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
  float f() { return (next() >> 8) * (1.0f / 16777216.0f); }            // [0,1)
  float range(float a, float b) { return a + (b - a) * f(); }
  int i(int n) { return n <= 0 ? 0 : (int)(next() % (u32)n); }
  bool chance(float pct) { return f() * 100.0f < pct; }
};
