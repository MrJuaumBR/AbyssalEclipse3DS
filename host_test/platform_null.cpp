// Headless backend for host tests: no window, counts draw calls, stores files in the working dir.
#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace plat {
static u64 g_draws = 0, g_frames = 0, g_maxDraws = 0, g_cur = 0;
static MixCallback g_cb = nullptr;
bool init() { return true; }
void shutdown() {}
bool beginFrame(Input& in) { in = Input(); g_cur = 0; return true; }
bool suspended() { return false; }
void beginScreen(bool) {}
void clear(u32) {}
void rect(float, float, float, float, u32) { ++g_draws; ++g_cur; }
void blit(int, int, int, int, float, float, float, float, u32, int, float) { ++g_draws; ++g_cur; }
void endFrame() { ++g_frames; if (g_cur > g_maxDraws) g_maxDraws = g_cur; }
void atlasUpload(const u32*, int, int) {}
void atlasUpdate(int, int, int, int, const u32*, int) {}
void setTargetFps(int) {}
bool audioInit(MixCallback cb) { g_cb = cb; return true; }
void audioPump() { if (g_cb) { s16 buf[368]; g_cb(buf, 368); } }
void audioShutdown() {}
bool storageRead(const char* name, void* buf, size_t cap, size_t* got) {
  FILE* f = fopen(name, "rb"); if (!f) return false;
  *got = fread(buf, 1, cap, f); fclose(f); return true;
}
bool storageWrite(const char* name, const void* buf, size_t len) {
  char tmp[128]; snprintf(tmp, sizeof tmp, "%s.tmp", name);
  FILE* f = fopen(tmp, "wb"); if (!f) return false;
  size_t w = fwrite(buf, 1, len, f); fclose(f);
  if (w != len) return false;
  return rename(tmp, name) == 0;
}
u64 nowMs() { timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (u64)ts.tv_sec * 1000 + ts.tv_nsec / 1000000; }
u64 statMaxDrawsPerFrame() { return g_maxDraws; }
}  // namespace plat
