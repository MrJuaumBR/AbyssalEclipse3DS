// MINIMAL STUB of libctru, only the symbols platform_3ds.cpp uses. Catches typos, NOT API drift.
#pragma once
#include <stdint.h>
#include <stddef.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64; typedef int32_t s32; typedef int16_t s16; typedef int8_t s8;
typedef s32 Result;
#define R_FAILED(r) ((r) < 0)
enum { KEY_A = 1, KEY_B = 2, KEY_SELECT = 4, KEY_START = 8, KEY_DRIGHT = 16, KEY_DLEFT = 32, KEY_DUP = 64, KEY_DDOWN = 128,
       KEY_R = 256, KEY_L = 512, KEY_X = 1024, KEY_Y = 2048, KEY_TOUCH = 1 << 20 };
struct circlePosition { s16 dx, dy; };
struct touchPosition { u16 px, py; };
typedef enum { GFX_TOP, GFX_BOTTOM } gfxScreen_t;
typedef enum { GFX_LEFT, GFX_RIGHT } gfx3dSide_t;
void gfxInitDefault(); void gfxExit(); void gfxSet3D(bool);
bool aptMainLoop();
void hidScanInput(); u32 hidKeysDown(); u32 hidKeysHeld(); u32 hidKeysUp();
void hidCircleRead(circlePosition*); void hidTouchRead(touchPosition*);
typedef enum { APTHOOK_ONSUSPEND, APTHOOK_ONRESTORE, APTHOOK_ONSLEEP, APTHOOK_ONWAKEUP, APTHOOK_ONEXIT, APTHOOK_COUNT } APT_HookType;
struct aptHookCookie { void* next; };
typedef void (*aptHookFn_t)(APT_HookType, void*);
void aptHook(aptHookCookie*, aptHookFn_t, void*); void aptUnhook(aptHookCookie*);
Result romfsInit();
u64 osGetTime();
void* linearAlloc(size_t); void linearFree(void*);
void GSPGPU_FlushDataCache(const void*, u32);
Result DSP_FlushDataCache(const void*, u32);
typedef enum { NDSP_WBUF_FREE, NDSP_WBUF_QUEUED, NDSP_WBUF_PLAYING, NDSP_WBUF_CANCELLED, NDSP_WBUF_DONE } ndspWaveBufStatus;
struct ndspWaveBuf { const void* data_vaddr; u32 nsamples; void* adpcm_data; u32 offset; bool looping; u16 sequence_id; ndspWaveBufStatus status; void* next; };
enum { NDSP_OUTPUT_MONO, NDSP_OUTPUT_STEREO, NDSP_OUTPUT_SURROUND };
enum { NDSP_INTERP_POLYPHASE, NDSP_INTERP_LINEAR, NDSP_INTERP_NONE };
enum { NDSP_FORMAT_MONO_PCM8, NDSP_FORMAT_MONO_PCM16 };
Result ndspInit(); void ndspExit(); void ndspSetOutputMode(int); void ndspChnReset(int); void ndspChnSetInterp(int, int);
void ndspChnSetRate(int, float); void ndspChnSetFormat(int, u16); void ndspChnSetMix(int, float*); void ndspChnWaveBufAdd(int, ndspWaveBuf*);
