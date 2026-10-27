# Handoff: 2004scape C Rewrite � Revision 225

**Updated:** October 20, 2026  
**Delivered milestone:** Scenery depth ordering and cache textures, verified in the native SDL client.  
**Next milestone:** Terrain lighting/blending, bridge/roof scene semantics, and remaining reference parity.

## Active completion roadmap

The active forward plan is `2004scape C Rewrite/AUTONOMOUS_PLAN.md`. It covers
complete live-client functionality and revision-225 authenticity in dependency
order. The delivery below is historical evidence for limited rendering checks;
it does not establish complete gameplay, ground/minimap parity or usable interfaces.
The next execution starts with phase 0, a capability audit and end-to-end scenarios,
then continues through the roadmap without stopping for routine phase approvals.
Planning only was performed for this update; no gameplay code or binary changed.

## Delivered build

Run `2004scape C Rewrite/play_client.bat --offline` for the verified Lumbridge
scene, or `play_client.bat --port 43595` with a server listening on that port.
The launcher uses the updated `client-c/bin/rs2client.exe` and adjacent SDL2.dll.
The existing `engine/data/pack/client` cache is required. The update ZIP can be
extracted into the existing rewrite folder; it contains the launcher and binaries.

Executable SHA-256: `D66AAC9B608F01184AC303C9160C5F2759663220793433988DF0CF2D0E97E3AA`

## What changed

- **Shared scene depth.** Terrain, scenery, NPCs and the player now use reciprocal
  camera depth per pixel. Later ground/scenery cannot overwrite nearer opaque
  surfaces merely because their tile coordinates are visited later. Multi-tile
  scenery is included when its footprint intersects the camera's existing +/-25
  tile range. A regression reverses world tile submission and checks identical pixels.
- **Cache textures.** The client loads all 50 indexed textures from the existing
  cache, expands cropped sprites, applies brightness, and preserves transparent
  palette entries. Stone, wooden doors/floors, foliage and water are visible in
  the actual client captures.
- **Model texture axes.** `ob_axis.dat` is decoded using the Java reference's
  six-byte axis entries. Texture IDs stay separate from vertex shade values.
  Axes survive deep model copies and composite vertex/index remapping. Texture
  coordinates are interpolated with perspective correction.
- **Terrain texture wiring.** Floor definitions now supply their texture IDs.
  Shaped overlays texture only their overlay triangles; underlay triangles retain
  their ground colors. Transparent magenta overlay definitions skip drawing.
- **Visibility and lighting.** Faces crossing z=50 are clipped with UV/shade
  interpolation. Transparent texels do not write depth. Alpha faces are queued
  across the complete scene, sorted by average depth, then composited after opaque
  terrain/entities. Flat face lighting and textured lightness follow the bundled
  Java model rules. UI/icon rendering keeps its separate rasterizer path.
- **Capture storage.** Verification retains lossless PNG client/world captures and
  removes each run's temporary BMPs after checking them. A disk-space error during
  this task was resolved by removing only this task's disposable captures inside
  the project. Compiler temporary files were subsequently directed inside the project.

Principal sources changed: `src/gfx/pix3d.c`, `src/gfx/model.c`,
`src/dash3d/world3d.c`, `src/dash3d/world.c`, their headers, and
`platform/sdl2_main.c`, all under `client-c`.

## Verification and evidence

Evidence lives in `client-c/verification/`:

- `scenery-build.log`: final native client and model viewer build, no warnings.
- `scenery-tests.log`: **28/28 test programs passed**, including the new scene rasterizer
  program and expanded real-cache model and world-order regressions.
- `scenery-world.log`: explicit world test with overlapping scenery, reversed
  submission, four Lumbridge camera angles, viewport clipping and rebuild identity.
- `scenery/client-verification.txt`: six successful executions of the actual SDL
  client: default/east/south/west, rebuilt scene, and walking to tile (33,32).
- `scenery/after-*.png`: final SDL-renderer readback images; `*.terrain.png`
  captures the opaque world pass before entities/UI and deferred alpha drawing.
  Full-client captures include the complete scene and UI.
- `scenery/live.log`, `scenery/live.png`, `scenery/server.log`: live server login,
  packet-237 rebuild of nine mapsquares at base (3040,3056), and rendering at the
  account's saved location. All 50 cache textures loaded. The temporary server
  used NODE_PORT=43595, WEB_PORT=8889 and WEB_MANAGEMENT_PORT=8899, with no .env
  changes, and was stopped afterward. The suite's separate default-port login
  test may report its documented offline skip.

Reproduce from `client-c`:

```powershell
make all
make test
./tests/verify_terrain.ps1 -OutputDirectory verification/scenery
```

The new rasterizer checks reversed opaque submission, alpha ordering and opaque
occlusion, transparent texture holes, perspective-correct UV sampling, near-plane
clipping, viewport protection, and the texture-axis basis. Model tests use actual
cache texture axes and verify copy/composite retention. World tests exercise the
real scene submission path. Captures come from the running SDL client, not a mock.

## Retained work and remaining limits

The previous terrain fixes, heap label allocation, overlay shapes/rotations,
viewport clipping, asset resolution, minimap rebuilding, character bone labels,
sequence delay inheritance and 50 Hz loop remain. The startup scene is still
mapsquare 50_50 with base (3200,3200). Do not change the camera-centered +/-25
range on the basis of the older handoff; it matches the reference.

This is an opaque-visibility and texture implementation, not bit-for-bit Java
rasterizer parity. The native renderer uses per-pixel depth instead of Java's full
tile dependency/occluder traversal. Java priority-specific layered effects,
intersecting alpha surfaces, and equal-depth surfaces need further parity work.
Texture animation, original quantized texture brightness, terrain lighting and
underlay blending are not complete. Roof/bridge/upper-plane scene behavior needs
its own reference audit. Mouse picking still needs visibility-aware occlusion.
The item-icon/standalone model-viewer rasterizer path has not been migrated to the
new scene texture path. No claim of exhaustive location/model coverage is made.

Repeated map rebuilds still leak existing heap-backed scenery; model ownership is
an outstanding audit. Earlier networking findings remain: NPC opcode 134 versus 1,
walking offsets, packet capacity checks and partial-packet blocking. Some old test
fixtures retain incorrect mapsquare labels; this delivery's Lumbridge checks use 50_50.

Both `2004scape C Rewrite/HANDOFF.md` and `Server-main/HANDOFF.md` contain this
handoff. The previous terrain handoff is saved at
`client-c/verification/handoff.terrain.md`; earlier evidence remains alongside it.
No Git commits, identities, remotes or nested repository histories were changed.
All task file access and cleanup stayed inside `2004scape-LostCityRS`.
