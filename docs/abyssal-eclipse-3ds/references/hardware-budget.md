# Old 3DS hardware budget (the "very, very capped" rules)

| Resource | Real limit (O3DS) | Our budget | Why |
|---|---|---|---|
| CPU | 2x ARM11 @ 268 MHz (VFP11); app gets core 0 fully, syscore only ~30% | Main thread only, <= 55% of a 16.6 ms frame (~9 ms) | Headroom for OS, audio, sleep wake-ups |
| RAM | 64 MB app region on retail; homebrew via HBL often sees ~24-32 MB usable | <= 8 MB heap+bss, <= 2 MB linear | Survives HBL, applet takeover, other launchers |
| VRAM | 6 MB | 0 (atlas in linear RAM) | Keep VRAM free for framebuffers |
| GPU | PICA200, ~40 Mpx/s practical fill, vertex-bound at ~100k tri/s in citro2d usage | <= 1200 quads/frame, 1 texture, <= 2 full-screen alpha layers | Single batch, no state changes |
| Screens | 400x240 (top, 800 px wide only in 3D), 320x240 touch | 3D disabled | Halves GPU work, saves battery |
| Frame rate | 60 Hz vsync | Fixed 60 Hz logic, auto 30 Hz fallback, 30 FPS option | Battery + stability |
| Audio | DSP (needs sdmc:/3ds/dspfirm.cdc), 24 ch | 1 music stream + 6 SFX voices, 22.05 kHz mono 16-bit, procedural | No asset files; silent fallback |
| Storage | SD, slow writes | < 8 KB save, event-driven, atomic | Avoid stutter and corruption |
| Input | D-pad, Circle Pad, A B X Y L R START SELECT, touch | All screens usable with D-pad + A/B alone; touch is a shortcut | Matches the design doc |

## Rules of thumb
- No `new`/`malloc`/`std::string`/`std::vector` in gameplay code. Use static arrays and `snprintf` into stack buffers.
- No exceptions, no RTTI (`-fno-exceptions -fno-rtti`), `-O2`, `-ffunction-sections -fdata-sections -Wl,--gc-sections`.
- Prefer squared distances; `sqrtf` only when a normalized vector is needed (cap ~150/frame).
- Collision is brute force over fixed pools (48 x 72) only for projectile-vs-enemy; enemy separation uses a stride (each enemy checks 8 neighbors per frame, rotating).
- Texture is `GPU_RGBA8`, nearest filter, 8x8 Morton tiling, V-flipped (origin bottom-left). Bake once; re-bake only the knight cell when cosmetics change.
- Draw order is painter's order by pool (zones, gems, enemies sorted by y bucket of 4, projectiles, player, fx, HUD). No depth buffer.
- Suspend/sleep: pause the sim, flush save, stop music buffers.
