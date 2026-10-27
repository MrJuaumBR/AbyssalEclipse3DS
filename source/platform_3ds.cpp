// Nintendo 3DS backend: libctru + citro3d + citro2d + ndsp.
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "platform.h"

namespace plat {
namespace {
const int AW = 256, AH = 256;
C3D_RenderTarget *g_top = nullptr, *g_bot = nullptr;
C3D_Tex g_tex;
u32 g_atlasCpu[AW * AH];
u8* g_texUpload = nullptr;
bool g_texReady = false, g_susp = false, g_gfx = false;
aptHookCookie g_hook;
const char* DIR1 = "sdmc:/3ds";
const char* DIR2 = "sdmc:/3ds/AbyssalEclipse";

const int BUF_FRAMES = 735, NBUF = 3;
ndspWaveBuf g_wb[NBUF];
s16* g_abuf = nullptr;
MixCallback g_mix = nullptr;
bool g_audio = false;

void aptHookFn(APT_HookType h, void*) {
  if (h == APTHOOK_ONSUSPEND || h == APTHOOK_ONSLEEP) g_susp = true;
  if (h == APTHOOK_ONRESTORE || h == APTHOOK_ONWAKEUP) g_susp = false;
}
}  // namespace

bool init() {
  gfxInitDefault();
  gfxSet3D(false);
  if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) return false;
  if (!C2D_Init(1536)) return false;
  C2D_Prepare();
  g_top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
  g_bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
  g_gfx = true;
  aptHook(&g_hook, aptHookFn, nullptr);
  romfsInit();
  mkdir(DIR1, 0777); mkdir(DIR2, 0777);
  return g_top && g_bot;
}

void shutdown() {
  if (g_texReady) { C3D_TexDelete(&g_tex); g_texReady = false; }
  if (g_texUpload) { linearFree(g_texUpload); g_texUpload = nullptr; }
  if (g_gfx) { C2D_Fini(); C3D_Fini(); gfxExit(); }
  aptUnhook(&g_hook);
}

bool beginFrame(Input& in) {
  if (!aptMainLoop()) return false;
  hidScanInput();
  const u32 d = hidKeysDown(), h = hidKeysHeld(), u = hidKeysUp();
  auto map = [](u32 k) {
    u32 r = 0;
    if (k & KEY_A) r |= BTN_A;
    if (k & KEY_B) r |= BTN_B;
    if (k & KEY_SELECT) r |= BTN_SELECT;
    if (k & KEY_START) r |= BTN_START;
    if (k & KEY_DRIGHT) r |= BTN_RIGHT;
    if (k & KEY_DLEFT) r |= BTN_LEFT;
    if (k & KEY_DUP) r |= BTN_UP;
    if (k & KEY_DDOWN) r |= BTN_DOWN;
    if (k & KEY_R) r |= BTN_R;
    if (k & KEY_L) r |= BTN_L;
    if (k & KEY_X) r |= BTN_X;
    if (k & KEY_Y) r |= BTN_Y;
    return r;
  };
  in.down = map(d); in.held = map(h); in.up = map(u);
  circlePosition cp; hidCircleRead(&cp);
  in.cx = cp.dx / 156.0f; in.cy = -cp.dy / 156.0f;
  if (in.cx > -0.15f && in.cx < 0.15f) in.cx = 0;
  if (in.cy > -0.15f && in.cy < 0.15f) in.cy = 0;
  in.touchDown = (d & KEY_TOUCH) != 0; in.touchHeld = (h & KEY_TOUCH) != 0; in.touchUp = (u & KEY_TOUCH) != 0;
  if (in.touchHeld || in.touchDown) { touchPosition tp; hidTouchRead(&tp); in.tx = tp.px; in.ty = tp.py; }
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
  return true;
}

bool suspended() { return g_susp; }

void beginScreen(bool top) {
  C3D_RenderTarget* t = top ? g_top : g_bot;
  C2D_TargetClear(t, C2D_Color32(11, 10, 18, 255));
  C2D_SceneBegin(t);
}

void clear(u32 rgba) { (void)rgba; }
void rect(float x, float y, float w, float h, u32 rgba) { C2D_DrawRectSolid(x, y, 0.0f, w, h, rgba); }

void blit(int sx, int sy, int sw, int sh, float dx, float dy, float dw, float dh, u32 color, int mode, float rot) {
  if (!g_texReady || sw <= 0 || sh <= 0 || sx < 0 || sy < 0 || sx + sw > AW || sy + sh > AH)
    return;

  Tex3DS_SubTexture st = {};
  st.width = (u16)sw;
  st.height = (u16)sh;
  st.left = (float)sx / AW;
  st.right = (float)(sx + sw) / AW;
  // Tex3DS uses top > bottom for a normal, non-rotated top-down image.
  st.top = 1.0f - (float)sy / AH;
  st.bottom = 1.0f - (float)(sy + sh) / AH;

  C2D_Image img = { &g_tex, &st };
  C2D_DrawParams p = {};
  // dx/dy are the destination's top-left; rotation remains centered.
  p.pos.x = dx + dw * 0.5f;
  p.pos.y = dy + dh * 0.5f;
  p.pos.w = dw;
  p.pos.h = dh;
  p.center.x = dw * 0.5f;
  p.center.y = dh * 0.5f;
  p.depth = 0.0f;
  p.angle = rot;

  if (mode == BLIT_TINT) {
    C2D_ImageTint t;
    C2D_PlainImageTint(&t, color, 1.0f);
    C2D_DrawImage(img, &p, &t);
  } else if (mode == BLIT_ALPHA) {
    C2D_ImageTint t;
    C2D_AlphaImageTint(&t, (color >> 24) / 255.0f);
    C2D_DrawImage(img, &p, &t);
  } else {
    C2D_DrawImage(img, &p, nullptr);
  }
}

void endFrame() { C3D_FrameEnd(0); }

static void makePicaRgba8(const u32* src, u8* dst) {
  // PICA RGBA8: row-major 8x8 tiles, Morton order within each tile.
  // GPU byte order is A, B, G, R; the game's CPU u32 bytes are R, G, B, A.
  for (int ty = 0; ty < AH; ty += 8) {
    for (int tx = 0; tx < AW; tx += 8) {
      const size_t tileBase = (size_t)((ty / 8) * (AW / 8) + (tx / 8)) * 64;
      for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
          const size_t morton = ((x & 1)) | ((y & 1) << 1) |
                                ((x & 2) << 1) | ((y & 2) << 2) |
                                ((x & 4) << 2) | ((y & 4) << 3);
          const u32 c = src[(ty + y) * AW + (tx + x)];
          u8* p = dst + (tileBase + morton) * 4;
          p[0] = (u8)(c >> 24); // A
          p[1] = (u8)(c >> 16); // B
          p[2] = (u8)(c >> 8);  // G
          p[3] = (u8)c;         // R
        }
      }
    }
  }
}

static bool ensureAtlasTexture() {
  if (g_texReady) return true;
  if (!C3D_TexInit(&g_tex, AW, AH, GPU_RGBA8)) return false;
  g_texUpload = (u8*)linearAlloc((size_t)AW * AH * 4);
  if (!g_texUpload) {
    C3D_TexDelete(&g_tex);
    return false;
  }
  C3D_TexSetFilter(&g_tex, GPU_NEAREST, GPU_NEAREST);
  C3D_TexSetWrap(&g_tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
  g_texReady = true;
  return true;
}

static void uploadWholeAtlas() {
  makePicaRgba8(g_atlasCpu, g_texUpload);
  // Old 3DS can retain CPU cache lines for linearAlloc buffers. Flush the
  // complete staging buffer so partial cosmetic atlas changes are visible.
  GSPGPU_FlushDataCache(g_texUpload, (size_t)AW * AH * 4);
  C3D_TexLoadImage(&g_tex, g_texUpload, GPU_TEXFACE_2D, 0);
  C3D_TexFlush(&g_tex);
}

void atlasUpload(const u32* rgba, int w, int h) {
  if (!rgba || w != AW || h != AH || !ensureAtlasTexture()) return;
  if (rgba != g_atlasCpu)
    memcpy(g_atlasCpu, rgba, (size_t)AW * AH * sizeof(u32));
  uploadWholeAtlas();
}

void atlasUpdate(int x, int y, int w, int h, const u32* rgba, int stride) {
  if (!g_texReady || !rgba || x < 0 || y < 0 || w <= 0 || h <= 0 ||
      x + w > AW || y + h > AH || stride < w)
    return;

  for (int row = 0; row < h; ++row) {
    u32* dst = g_atlasCpu + (y + row) * AW + x;
    const u32* src = rgba + row * stride;
    if (dst != src) memmove(dst, src, (size_t)w * sizeof(u32));
  }
  uploadWholeAtlas();
}

void setTargetFps(int fps) { C3D_FrameRate(fps >= 60 ? 60.0f : 30.0f); }

bool audioInit(MixCallback cb) {
  g_mix = cb;
  if (R_FAILED(ndspInit())) return false;
  ndspSetOutputMode(NDSP_OUTPUT_MONO);
  ndspChnReset(0);
  ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
  ndspChnSetRate(0, 22050.0f);
  ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16);
  float mix[12]; memset(mix, 0, sizeof mix); mix[0] = 1.0f; mix[1] = 1.0f;
  ndspChnSetMix(0, mix);
  g_abuf = (s16*)linearAlloc(sizeof(s16) * BUF_FRAMES * NBUF);
  if (!g_abuf) { ndspExit(); return false; }
  memset(g_wb, 0, sizeof g_wb);
  for (int i = 0; i < NBUF; ++i) { g_wb[i].data_vaddr = g_abuf + i * BUF_FRAMES; g_wb[i].nsamples = BUF_FRAMES; g_wb[i].status = NDSP_WBUF_DONE; }
  g_audio = true;
  return true;
}

void audioPump() {
  if (!g_audio) return;
  for (int i = 0; i < NBUF; ++i) {
    if (g_wb[i].status != NDSP_WBUF_DONE) continue;
    s16* p = (s16*)g_wb[i].data_vaddr;
    g_mix(p, BUF_FRAMES);
    DSP_FlushDataCache(p, sizeof(s16) * BUF_FRAMES);
    ndspChnWaveBufAdd(0, &g_wb[i]);
  }
}

void audioShutdown() {
  if (!g_audio) return;
  ndspChnReset(0); ndspExit(); linearFree(g_abuf); g_audio = false;
}

bool storageRead(const char* name, void* buf, size_t cap, size_t* got) {
  char path[160]; snprintf(path, sizeof path, "%s/%s", DIR2, name);
  FILE* f = fopen(path, "rb");
  if (!f) { char tmp[170]; snprintf(tmp, sizeof tmp, "%s.tmp", path); f = fopen(tmp, "rb"); }
  if (!f) return false;
  *got = fread(buf, 1, cap, f); fclose(f);
  return true;
}

bool storageWrite(const char* name, const void* buf, size_t len) {
  char path[160], tmp[170]; snprintf(path, sizeof path, "%s/%s", DIR2, name); snprintf(tmp, sizeof tmp, "%s.tmp", path);
  FILE* f = fopen(tmp, "wb"); if (!f) return false;
  size_t w = fwrite(buf, 1, len, f); fclose(f);
  if (w != len) return false;
  remove(path);
  return rename(tmp, path) == 0;
}

u64 nowMs() { return osGetTime(); }
}  // namespace plat
