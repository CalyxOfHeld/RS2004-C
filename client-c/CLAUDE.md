# Claude Developer Guide: 2004scape C Rewrite (RS2 Rev 225)

## Project Objective
Build a clean, high-performance native C99 client for 2004scape (RuneScape 2, Revision 225 - May 18, 2004) targeting **PC (Windows / Linux) using SDL2**.

> Note: Console targets are deferred until the PC client is 100% functional and verified. All development is focused exclusively on PC with SDL2.

## Reference Repositories
- `javaclient/src/main/java/`: The golden deobfuscated Java client reference.
- `webclient/src/`: The secondary typed TypeScript client reference.
- `engine/`: The local server. Run `bun start` in `engine` to test live connections at `localhost:43594`.
- `content/`: 2004 game definitions, 3D models, textures, maps, and cache archives.

## Active Task Handoff
Refer to `../HANDOFF.md` for current milestone progress, verified systems, and open investigations (specifically 3D ground terrain rasterization in `src/dash3d/world3d.c`).

## Architecture & Conventions

### 1. Dialect & Platform
- Pure **C99**. Use `<stdint.h>`, `<stdbool.h>`, `<stddef.h>`.
- Use fixed-width types: `int32_t`, `uint32_t`, `int16_t`, `uint16_t`, `uint8_t`, `int8_t`.
- Graphics & Windowing: SDL2 with software rasterizer (`Pix3D`) outputting to a `uint32_t pixels[765 * 503]` framebuffer blitted via `SDL_UpdateTexture` and `SDL_RenderCopy`.

### 2. Memory Management
- Avoid unnecessary heap churn.
- Use explicit Arena Allocators (`include/core/arena.h`):
  - `g_arena_perm`: Static cache archives, configs, fonts.
  - `g_arena_world`: 3D map meshes and terrain.
  - `g_arena_frame`: Temporary models, rasterizer scanlines, and packets (reset every tick via `arena_reset(&g_arena_frame)`).

### 3. Build & Run Commands
```sh
# Clean and build the PC client
make clean && make

# Run the client
./bin/rs2client.exe

# Run the test suite
make test
```
