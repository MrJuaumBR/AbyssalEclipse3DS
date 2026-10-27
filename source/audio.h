// Procedural audio: software mixer producing 22.05 kHz mono. Platform pulls samples via audio::mix.
#pragma once
#include "types.h"
namespace audio {
enum Sfx { SFX_SHOOT, SFX_HIT, SFX_KILL, SFX_HURT, SFX_PICKUP, SFX_LEVEL, SFX_UI, SFX_WAVE, SFX_BOSS, SFX_DEATH, SFX_BLAST, SFX_COUNT };
enum { RATE = 22050 };
void init();
void sfx(int id);
void musicSet(int season, float intensity);   // season 0..4, or -1 for the menu theme; intensity 0..1
void setVolumes(int music, int sfx);          // 0..100
void mix(s16* out, int frames);               // called by the platform backend
}
