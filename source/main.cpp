// Entry point for the 3DS build. Fixed-step-ish loop with automatic 60 -> 30 Hz fallback on slow frames.
#include "game.h"
#include "platform.h"

int main(int, char**) {
  if (!gameInit()) { gameShutdown(); return 1; }
  plat::Input in;
  int wantFps = gameTargetFps(), slow = 0;
  plat::setTargetFps(wantFps);
  u64 last = plat::nowMs();
  while (plat::beginFrame(in)) {
    const u64 now = plat::nowMs();
    float measured = (float)(now - last) / 1000.0f; last = now;
    if (gameTargetFps() != wantFps && gameTargetFps() == 30) { wantFps = 30; slow = 0; plat::setTargetFps(30); }
    if (gameTargetFps() == 60 && wantFps == 30 && slow == 0) { wantFps = 60; plat::setTargetFps(60); }
    float dt = 1.0f / (float)wantFps;
    if (measured > dt) dt = measured > 0.05f ? 0.05f : measured;       // never run the sim in slow motion
    if (wantFps == 60) { if (measured > 0.022f) ++slow; else if (slow > 0) --slow; if (slow > 150) { wantFps = 30; plat::setTargetFps(30); slow = 1; } }
    if (!plat::suspended()) gameFrame(in, dt);
    gameDraw();
    plat::endFrame();
    plat::audioPump();
  }
  gameShutdown();
  return 0;
}
