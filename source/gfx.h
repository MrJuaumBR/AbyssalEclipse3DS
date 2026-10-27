// Drawing helpers on top of platform.h: procedural atlas, bitmap font, panels.
#pragma once
#include "types.h"
#include "data.h"
#include "platform.h"

namespace gfx {
struct Spr { u16 x = 0, y = 0, w = 0, h = 0; };
enum { FX_COLORS = 8 };                       // colours baked for discs/rings
extern const u32 FX_PALETTE[FX_COLORS];

struct Cosmetics { u8 hat = 0, face = 4, color = 0; };

bool init(const Cosmetics& c);                // bake + upload the atlas (once at boot)
void setKnight(const Cosmetics& c);           // re-bake the knight cell
const Spr& enemy(int id);
const Spr& icon(int id);
const Spr& spellIcon(int type, int elem, int act);
const Spr& knight();
const Spr& knightPreview(const Cosmetics& c);
const Spr& clothingPreview(int hat);
const Spr& drone();
const Spr& sentinel();
enum ProjSpr { PS_DAGGER, PS_ARROW, PS_AXE, PS_ORB, PS_SHARD, PS_WEB, PS_BOLT, PS_EMBER, PS_GEM, PS_SPARK, PS_KNIFE, PS_TNT, PS_COUNT };
const Spr& proj(int id);
const Spr& disc(int colorIdx);                // 24x24 filled, translucent
const Spr& ring(int colorIdx);                // 24x24 outline
const Spr& shadow();                          // 16x6

// Drawing (all coordinates in pixels of the current screen)
void setOffset(float x, float y); // shared draw offset, used for world camera translation
void rect(float x, float y, float w, float h, u32 rgb, u32 a = 255);
void panel(float x, float y, float w, float h, u32 fill, u32 border);   // 2 quads
void spr(const Spr& s, float x, float y, float scale = 1.0f, u32 color = 0xFFFFFFFFu, int mode = plat::BLIT_PLAIN, float rot = 0.0f);   // top-left
void sprC(const Spr& s, float cx, float cy, float scale = 1.0f, u32 color = 0xFFFFFFFFu, int mode = plat::BLIT_PLAIN, float rot = 0.0f); // centered
void discC(int colorIdx, float cx, float cy, float radius, u32 alpha = 255);  // scales the 24px disc
void ringC(int colorIdx, float cx, float cy, float radius, u32 alpha = 255);

// Text (UTF-8, uppercase glyphs only; lowercase is drawn as capitals)
int textWidth(const char* s, int scale = 1);
void text(const char* s, float x, float y, u32 rgb, int scale = 1, u32 a = 255);
void textC(const char* s, float cx, float y, u32 rgb, int scale = 1, u32 a = 255);
void textR(const char* s, float rx, float y, u32 rgb, int scale = 1, u32 a = 255);
// Word-wrapped text. Returns the number of lines drawn; maxLines<=0 means unlimited. skipLines scrolls.
int textWrap(const char* s, float x, float y, float w, u32 rgb, int scale, int maxLines, int skipLines = 0, int* totalLines = nullptr);
enum { LINE_H = 9, GLYPH_W = 6 };
}  // namespace gfx
