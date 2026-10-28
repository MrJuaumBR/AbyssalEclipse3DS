// The ONLY interface between game code and the console. Implemented by
// platform_3ds.cpp (libctru + citro2d + ndsp) and platform_null.cpp (host tests).
#pragma once
#include "types.h"

namespace plat {
enum Button : u32 {
  BTN_A = 1 << 0, BTN_B = 1 << 1, BTN_SELECT = 1 << 2, BTN_START = 1 << 3, BTN_RIGHT = 1 << 4, BTN_LEFT = 1 << 5,
  BTN_UP = 1 << 6, BTN_DOWN = 1 << 7, BTN_R = 1 << 8, BTN_L = 1 << 9, BTN_X = 1 << 10, BTN_Y = 1 << 11
};
struct Input {
  u32 down = 0, held = 0, up = 0;      // button edges / state (D-pad bits included)
  float cx = 0, cy = 0;                // Circle Pad, -1..1 (y up = negative on screen)
  bool touchDown = false, touchHeld = false, touchUp = false;
  int tx = 0, ty = 0;                  // touch position in bottom-screen pixels
};
enum BlitMode { BLIT_PLAIN = 0, BLIT_TINT = 1, BLIT_ALPHA = 2 };  // as-is / silhouette in colour / alpha multiply

bool init();
void shutdown();
bool beginFrame(Input& in);            // false => the OS asked us to quit
bool suspended();                      // true while the app is in the background / lid closed
void beginScreen(bool top);            // top: 400x240, bottom: 320x240
void clear(u32 rgba);
void rect(float x, float y, float w, float h, u32 rgba);
void blit(int sx, int sy, int sw, int sh, float dx, float dy, float dw, float dh, u32 color, int mode, float rot);
void endFrame();
void atlasUpload(const u32* rgba, int w, int h);                         // full upload (256x256)
void atlasUpdate(int x, int y, int w, int h, const u32* rgba, int stride);  // partial update
void setTargetFps(int fps);            // 60 or 30 (frame pacing for battery)

// Audio: platform pulls 22050 Hz mono s16 from the game mixer on the main thread.
typedef void (*MixCallback)(s16* out, int frames);
bool audioInit(MixCallback cb);        // false => silent mode (no DSP firmware)
void audioPump();                      // call once per frame
void audioShutdown();

// Storage (SD card on hardware). name is a bare file name inside the game folder.
bool storageRead(const char* name, void* buf, size_t cap, size_t* got);
bool storageWrite(const char* name, const void* buf, size_t len);  // atomic (tmp + rename)
u64 nowMs();
#ifdef __3DS__
void debugLog(const char* fmt, ...);  // appends to sdmc:/3ds/AbyssalEclipse/boot.log
#else
inline void debugLog(const char*, ...) {}  // host-test no-op
#endif
}  // namespace plat
