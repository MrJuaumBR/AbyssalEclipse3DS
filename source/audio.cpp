#include "audio.h"
#include <math.h>
#include <string.h>

namespace audio {
namespace {
struct SfxDef { float f0, f1, dur, vol; u8 noise; };
const SfxDef SD[SFX_COUNT] = {
    {900, 500, 0.06f, 0.10f, 0}, {300, 200, 0.05f, 0.16f, 1}, {320, 110, 0.12f, 0.18f, 0}, {220, 70, 0.18f, 0.26f, 1},
    {1200, 1900, 0.05f, 0.10f, 0}, {500, 1100, 0.30f, 0.16f, 0}, {800, 800, 0.04f, 0.10f, 0}, {400, 700, 0.20f, 0.14f, 0},
    {140, 60, 0.50f, 0.26f, 1}, {400, 45, 0.80f, 0.26f, 0}, {260, 60, 0.25f, 0.24f, 1}};
struct Voice { bool on; float t, dur, f0, f1, vol, ph; u8 noise; };
Voice V[6];
int musicVol = 50, sfxVol = 70;
u32 nrng = 12345;
float noise() { nrng = nrng * 1664525u + 1013904223u; return ((nrng >> 16) & 0x7FFF) / 16384.0f - 1.0f; }

// --- music: 8th-note step sequencer, lead (25% pulse) + bass (square), noise hat when tense
const int SCALE[4][7] = {{0, 2, 3, 5, 7, 8, 10}, {0, 1, 4, 5, 7, 8, 10}, {0, 2, 3, 5, 7, 8, 11}, {0, 1, 3, 4, 6, 7, 9}};
const int ROOT[6] = {57, 52, 59, 50, 49, 55};                // forest, desert, snow, mountain, eclipse, menu
const int SCALE_OF[6] = {0, 1, 2, 0, 3, 0};
const s8 PAT[6][16] = {{0, -1, 2, 4, -1, 2, 0, -1, 3, -1, 4, 6, 4, -1, 2, -1}, {0, 1, -1, 4, 3, -1, 1, 0, 4, -1, 3, 1, -1, 0, 1, -1},
                       {4, -1, 2, -1, 0, 2, 4, -1, 5, 4, -1, 2, 0, -1, 2, -1}, {0, -1, -1, 3, 0, -1, 4, -1, 0, 3, -1, 2, 0, -1, -1, 1},
                       {0, 1, 3, -1, 1, 3, 4, -1, 6, 4, 3, -1, 1, 0, -1, 1}, {0, -1, 2, -1, 4, -1, 2, -1, 0, -1, 3, -1, 4, 2, 0, -1}};
const int BASS_DEG[4] = {0, 5, 2, 6};
struct Music { int track = 5; float inten = 0.3f; double pos = 0; int step = 0; float lf = 0, bf = 0, lph = 0, bph = 0; } M;
float midiHz(int m) { return 440.0f * powf(2.0f, (m - 69) / 12.0f); }
void stepNote() {
  const int tr = M.track, *sc = SCALE[SCALE_OF[tr]];
  int bar = (M.step / 16) & 3, st = M.step & 15;
  s8 n = PAT[tr][st];
  if (n < 0) M.lf = 0; else M.lf = midiHz(ROOT[tr] + 12 + sc[n % 7] + (n >= 7 ? 12 : 0));
  M.bf = (st & 1) ? M.bf : midiHz(ROOT[tr] - 12 + sc[BASS_DEG[bar] % 7]);
}
}  // namespace

void init() { memset(V, 0, sizeof V); M = Music(); stepNote(); }
void setVolumes(int m, int s) { musicVol = m < 0 ? 0 : (m > 100 ? 100 : m); sfxVol = s < 0 ? 0 : (s > 100 ? 100 : s); }
void musicSet(int season, float inten) {
  int tr = (season >= 0 && season < 5) ? season : 5;
  if (tr != M.track) { M.track = tr; M.step = 0; M.pos = 0; stepNote(); }
  M.inten = inten;
}
void musicPreview(int season) { musicSet(season, 0.75f); }
void sfx(int id) {
  if (id < 0 || id >= SFX_COUNT || sfxVol == 0) return;
  int k = 0;
  for (int i = 0; i < 6; ++i) { if (!V[i].on) { k = i; break; } if (V[i].t > V[k].t) k = i; }
  const SfxDef& d = SD[id];
  V[k] = Voice{true, 0, d.dur, d.f0, d.f1, d.vol, 0, d.noise};
}
void mix(s16* out, int frames) {
  const float sv = sfxVol / 100.0f, mv = musicVol / 100.0f;
  const double stepLen = RATE * 60.0 / (84.0 + 40.0 * M.inten) / 2.0;
  for (int i = 0; i < frames; ++i) {
    float s = 0;
    for (int v = 0; v < 6; ++v) {
      Voice& x = V[v];
      if (!x.on) continue;
      float k = x.t / x.dur, f = x.f0 + (x.f1 - x.f0) * k, env = 1.0f - k;
      x.ph += f / RATE; x.ph -= (int)x.ph;
      float w = x.noise ? noise() : (x.ph < 0.5f ? 1.0f : -1.0f);
      s += w * x.vol * env * sv;
      x.t += 1.0f / RATE;
      if (x.t >= x.dur) x.on = false;
    }
    if (mv > 0) {
      M.pos += 1.0;
      if (M.pos >= stepLen) { M.pos -= stepLen; M.step++; stepNote(); }
      float env = 1.0f - (float)(M.pos / stepLen) * 0.6f, m = 0;
      if (M.lf > 0) { M.lph += M.lf / RATE; M.lph -= (int)M.lph; m += (M.lph < 0.25f ? 1.0f : -1.0f) * 0.07f * env; }
      if (M.bf > 0) { M.bph += M.bf / RATE; M.bph -= (int)M.bph; m += (M.bph < 0.5f ? 1.0f : -1.0f) * 0.06f; }
      if (M.inten > 0.6f && (M.step & 1) && M.pos < stepLen * 0.15) m += noise() * 0.03f;
      s += m * mv;
    }
    if (s > 0.95f) s = 0.95f;
    if (s < -0.95f) s = -0.95f;
    out[i] = (s16)(s * 30000);
  }
}
}  // namespace audio
