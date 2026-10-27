#pragma once
#include "citro3d.h"
struct Tex3DS_SubTexture { u16 width, height; float left, top, right, bottom; };
struct C2D_Image { C3D_Tex* tex; const Tex3DS_SubTexture* subtex; };
struct C2D_DrawParams { struct { float x, y, w, h; } pos; struct { float x, y; } center; float depth; float angle; };
struct C2D_ImageTint { u32 dummy[8]; };
#define C2D_Color32(r, g, b, a) ((u32)(r) | ((u32)(g) << 8) | ((u32)(b) << 16) | ((u32)(a) << 24))
bool C2D_Init(size_t); void C2D_Fini(); void C2D_Prepare();
C3D_RenderTarget* C2D_CreateScreenTarget(gfxScreen_t, gfx3dSide_t);
void C2D_TargetClear(C3D_RenderTarget*, u32); void C2D_SceneBegin(C3D_RenderTarget*);
bool C2D_DrawRectSolid(float, float, float, float, float, u32);
bool C2D_DrawImage(C2D_Image, const C2D_DrawParams*, const C2D_ImageTint*);
void C2D_PlainImageTint(C2D_ImageTint*, u32, float); void C2D_AlphaImageTint(C2D_ImageTint*, float);
