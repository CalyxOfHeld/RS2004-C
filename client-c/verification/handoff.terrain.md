# Handoff: 2004scape C Rewrite — Revision 225

**Updated:** October 20, 2026  
**Delivered milestone:** Missing startup terrain fixed and verified in the actual native client.  
**Next milestone:** Scenery ordering, occlusion, textures, and remaining visual parity.

## Current result

The working C client now displays Lumbridge grass and paths under the character.
The character, NPC demonstration, UI, minimap, movement animation, and camera still
render. Four camera views, walking, and scene rebuilding were verified through
actual client executions, with saved SDL-renderer images. Live login and a
server-driven rebuild of nine mapsquares were also verified at the existing
account's saved location.

This is a terrain-visibility delivery, not a claim of complete revision-225 parity.
Scenery and texture defects remain visible and are documented below.

## Root causes and changes

1. **Wrong startup mapsquare.** The client called `m47_47/l47_47` Lumbridge.
   That fixture is dominated by water that this renderer does not texture.
   `content/maps/labels.txt` puts Lumbridge at (3239,3233), in mapsquare **50_50**.
   Startup now uses `m50_50/l50_50` with base (3200,3200), and fails clearly if
   either map file is missing.
2. **Scenery allocation crash.** Correctly populated scenery reached
   `model_create_label_references(..., NULL)` through `loc_get_model/model_copy`.
   It dereferenced the NULL arena. Label groups now follow the caller's heap or
   arena allocation policy; a regression test covers the heap case.
3. **Overlay geometry.** `world3d_set_tile` indexed the reference shape tables
   with `shape-1`; both the Java `dash3d/type/TileOverlay.java` and TypeScript
   `dash3d/Ground.ts` use `shape`. Rotated points 15/16 were missing as well.
   All 11 overlay shapes now cover exactly one tile at all four rotations.
4. **Horizontal clipping.** Flat/Gouraud rasterizers clipped to framebuffer
   width instead of the active viewport. Both now respect left/right clip bounds.
   The new regression test failed before this correction and passes afterward.
5. **Rebuilding and launching.** Live map rebuilds refresh the minimap. Assets
   resolve relative to the executable, so launching from Explorer works.
   `play_client.bat` runs the delivered binary; both `test_client.bat` wrappers
   forward command-line options and still rebuild first.
6. **Verification support.** The client can save its actual SDL-rendered frame,
   run offline, set camera angles, and replay a rebuild or walk for verification.
   Scripted walking/rebuilding requires offline capture mode. The unused
   `g_axis` variable/load was removed to eliminate the compiler warning.

**Do not apply the old handoff's camera-range suggestion:** the Java reference
itself uses eyeTileX/eyeTileZ +/-25. That camera-centered range was left unchanged.

## Verification evidence

All evidence is in `2004scape C Rewrite/client-c/verification/`:

- `final-build.log`: native client and model viewer built, zero compiler warnings.
- `final-tests.log`: **27/27 test programs passed**, with expanded world/model
  regressions for real Lumbridge terrain, shapes/rotations, camera views, clipping,
  rebuild identity, and heap-backed label groups.
- `client-verification.txt`: six actual SDL-client executions passed. The scene
  contains grass and paths at yaw/pitch 0/128, 512/256, 1024/383, and 1536/128.
  Viewport borders and UI were checked. Rebuilding the same scene produces an
  identical world-pass image. Walking reaches tile (33,32), sequence 819/frame 6.
- `before.png`: original missing-ground appearance.
- `after-default.png`, `after-east.png`, `after-south.png`, `after-west.png`,
  `after-rebuild.png`, `after-walk.png`: final full-client captures.
- `live.log` / `live.png`: successful live login, packet-237 rebuild of nine
  mapsquares at base (3040,3056), and a rendered client frame after 100 frames.
- `README.md`: reproduction instructions, limitations, source references, and
  delivered executable SHA-256.

Images come from `SDL_RenderReadPixels` in the real client after copying its
framebuffer into the SDL renderer. Windows desktop capture timed out twice.
The `.bmp.terrain.bmp` companion images capture the world pass before entities/UI;
that world pass includes scenery. These are not screenshots of a test mockup.

The default server ports failed to bind (43594 permission error, web port 80 in
use). Live verification used temporary environment overrides: NODE_PORT=43595,
WEB_PORT=8889, WEB_MANAGEMENT_PORT=8899. No `.env` file was changed. The standard
suite's login test still uses 43594 and reports an offline skip; the separate
actual-client live verification succeeded on 43595.
The temporary verification server was stopped after these checks.

## Run and rebuild

From `2004scape C Rewrite`:

```powershell
./play_client.bat --offline
# With a running server on the default port:
./play_client.bat
# With a running server on the verification port:
./play_client.bat --port 43595
```

From `2004scape C Rewrite/client-c`:

```powershell
make all
make test
./tests/verify_terrain.ps1
```

The delivered binary is `client-c/bin/rs2client.exe` with `bin/SDL2.dll`. It needs
the existing adjacent engine cache. Arrow keys orbit the camera, click to walk,
WASD provides the existing movement controls, F1-F12 select tabs, Escape exits.

## Preserved earlier work

Character vertex/face bone labels, frame-delay inheritance in `seq_get_delay`,
and the 50 Hz GameShell-style loop remain intact. The final captures and tests
exercise animated characters and walking without reverting those fixes.

## Remaining work / practical limits

- Scenery painter ordering/occlusion and textures visibly differ from the source
  client. Water/wood textures, terrain lighting and underlay blending need further
  reference-driven work. The standalone startup scene covers one mapsquare.
- Audit scenery model ownership: repeated scene rebuilds still leak existing
  heap-backed models. This task fixed the NULL-arena crash, not all memory ownership.
- Earlier networking findings remain: NPC dispatch uses 134 instead of 1, walking
  offsets differ from the server's expected start-relative offsets, packet reads
  need capacity checks, and polling can block on partial packets.
- Do not equate test PASS banners with bit-for-bit parity. In particular, several
  old synchronization tests directly assign state. The original tests still use
  some inaccurate mapsquare labels; the new Lumbridge regression uses 50_50.
- Git is initialized only inside the rewrite root, with no baseline commit,
  configured author, or remote. Nested reference repositories must be handled
  deliberately before staging. No commits, identity, remotes, or histories were
  changed. Existing user changes were preserved.

Continue against the bundled May-2004 revision-225 Java/TypeScript/server sources.
The previous handoff is preserved at `client-c/verification/handoff.before.md`.
