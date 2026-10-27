# Handoff Document: 2004scape C Rewrite (RS2 Revision 225)

**Date**: October 20, 2026  
**Target Milestone**: 3D Ground Terrain & Land Rasterization Parity  

---

## 1. Executive Summary & Accomplished Work

### What Has Been Resolved & Confirmed Working
1. **Character Upright Stance & 45-Degree Tilt Fix (`src/gfx/model.c:model_load`)**:
   - Root Cause: `g_point5` (`vertex_label_offset`) and `g_face5` (`face_label_offset`) were skipped during model unpacking, defaulting all vertices to bone 0 (pelvis) and rotating the entire character as a rigid 45° monolith.
   - Fix: Full vertex label and face label unpacking implemented matching `Model.java:452-520`. Player model articulates cleanly across 62 independent bone labels.
2. **Animation Pacing & Delay Inheritance (`src/config/seq.c` & `src/entity/pathing_entity.c`)**:
   - Root Cause: `seq.c` forced `if (delay == 0) delay = 1`, making frames cycle at 20ms instead of inheriting duration from `AnimFrame.delay` (4 to 8 cycles = 80ms to 160ms/frame).
   - Fix: `seq_get_delay` dynamically inherits `AnimFrame.delay`. Stand sequence 808 frame 0 takes 400ms; walk sequence 819 stride takes 800ms.
3. **Authentic 50 Hz Client Loop (`platform/sdl2_main.c`)**:
   - Implemented exact `GameShell.java:150-210` loop with 10-entry circular buffer `otim[10]`, `deltime = 20`, and 50 Hz logic accumulator `while (var6 < 256) { ... var6 += var4; } var6 &= 0xFF;`.
   - Entity updates moved strictly into logic tick; redundant `SDL_Delay(20)` removed from render block.
4. **Build & Launch Scripts**:
   - Both `Server-main/test_client.bat` and `2004scape C Rewrite/test_client.bat` are available and compile with MinGW-w64.
   - All **27/27** automated unit tests pass via `make test`.

---

## 2. Immediate Next Problem: "Still No Land Though"

### The Issue
While the player character, NPCs (Hans), 2D UI components, fonts, chatbox, and minimap all render properly, **3D ground terrain (landscape underlay grass, paths, and overlays) is not visible in the 3D viewport** during live gameplay (`rs2client.exe`).

### Key Code Locations & Investigation Points

1. **`client-c/src/dash3d/world3d.c` (`world3d_render`)**:
   - Line 291: Tile visibility iteration:
     ```c
     int center_tx = eyeX / 128;
     int center_tz = eyeZ / 128;
     int range = 25;
     ```
     - Check: `eyeX / 128` and `eyeZ / 128` are the camera eye position, NOT the player/focus position. If the camera is pitched back behind the player, the tile iteration box may shift away from the visible tiles. Compare with `World3D.java:1113-1120`, which iterates relative to the camera lookat center.
   - Lines 316-320 & 331-342: Triangle projection and backface culling:
     ```c
     project_point(x0, y_sw, z0, eyeX, eyeY, eyeZ, sin_p, cos_p, sin_y, cos_y, &sx[0], &sy[0], &sz[0]);
     ...
     // Triangle 1: NE (2), NW (3), SE (1)
     if ((sx[2] - sx[3]) * (sy[1] - sy[3]) - (sy[2] - sy[3]) * (sx[1] - sx[3]) > 0) { ... }
     // Triangle 2: SW (0), SE (1), NW (3)
     if ((sx[0] - sx[1]) * (sy[3] - sy[1]) - (sy[0] - sy[1]) * (sx[3] - sx[1]) > 0) { ... }
     ```
     - Check: If vertex winding or screen Y direction (`pix3d_center_y + ((y << 9) / z)`) causes the 2D cross-product to be negative when viewed from above (pitch 128-383), ALL ground triangles will fail the `> 0` test and be culled. Compare with `World3D.java:1250-1350`.

2. **`client-c/src/dash3d/world.c` (`world_build`)**:
   - Lines 213-217:
     ```c
     int underlay_id = w->tile_underlays[l][x][z];
     int overlay_id = w->tile_overlays[l][x][z];
     if (underlay_id == 0 && overlay_id == 0) continue;
     ```
     - Check: Verify that `w->tile_underlays[0][x][z]` has non-zero values after `world_load_ground`. In `test_world.c`, `non_zero_underlays` was 4,096 for Lumbridge `m47_47`. Ensure this data is retained when `world_build` runs in `sdl2_main.c`.

3. **`client-c/platform/sdl2_main.c`**:
   - Lines 278-313 (Initial scene build):
     - `m47_47` and `l47_47` are loaded at startup.
   - Lines 965-975 (Packet 237 `REBUILD_NORMAL`):
     - When connected to a live server, the server sends Packet 237, triggering `map_streamer_rebuild`. Ensure `map_streamer_rebuild` decompresses ground maps and rebuilds `world3d` with the player's updated local scene base.

4. **`client-c/tests/test_world.c`**:
   - Reference unit test that renders 3D terrain to an offscreen buffer and validates shaded pixels (`assert(non_zero_pixels > 1000)`). Use this as a baseline comparison against `world3d_render` in `sdl2_main.c`.

---

## 3. Useful Commands
- **Compile client**: `cd "2004scape C Rewrite/client-c" && make bin/rs2client.exe`
- **Run test suite**: `cd "2004scape C Rewrite/client-c" && make test`
- **Run client**: `test_client.bat` (from `Server-main` or `2004scape C Rewrite`)
