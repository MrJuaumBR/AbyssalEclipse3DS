// Menus, screens and the glue between input, simulation and rendering.
#pragma once
#include "platform.h"
bool gameInit();                                   // load save, bake atlas, init audio
bool gameFrame(const plat::Input& in, float dt);   // false => quit requested
void gameDraw();                                   // draws both screens
void gameShutdown();
int gameTargetFps();
// test hooks
struct Sim; Sim& gameSim();
void gameStartRunForTest(int season, int level, bool endless, int weapon, u32 seed);
