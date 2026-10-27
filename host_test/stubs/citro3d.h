#pragma once
#include "3ds.h"
#define C3D_DEFAULT_CMDBUF_SIZE 0x40000
typedef struct C3D_RenderTarget C3D_RenderTarget;
struct C3D_Tex { void* data; u16 width, height; };
enum { GPU_RGBA8 = 0 }; enum { GPU_NEAREST = 0 }; enum { GPU_CLAMP_TO_EDGE = 0 }; enum { GPU_TEXFACE_2D = 0 };
enum { C3D_FRAME_SYNCDRAW = 1 };
bool C3D_Init(size_t); void C3D_Fini(); bool C3D_FrameBegin(u8); void C3D_FrameEnd(u8); void C3D_FrameRate(float);
bool C3D_TexInit(C3D_Tex*, u16, u16, int); void C3D_TexDelete(C3D_Tex*); void C3D_TexSetFilter(C3D_Tex*, int, int);
void C3D_TexLoadImage(C3D_Tex*, const void*, int, int);
void C3D_TexSetWrap(C3D_Tex*, int, int); void C3D_TexFlush(C3D_Tex*);
