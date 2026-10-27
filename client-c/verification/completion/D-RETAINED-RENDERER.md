# Retained synthetic renderer failure resolved — October 26, 2026

The retained `error6/kind3` fixture family has been compared with original revision-225 Java and classified. **No additional production renderer patch is needed.** An incorrect native fatal check was already removed on October25; a later failure from reconstructing the fixture is a synthetic topology error also rejected by Java. The camera regression now retains the populated map instead of clearing the scene.

## Retained evidence and reconstruction

The original failure is recorded in `runs/2026-10-24-receive-batch-sol/camera/candidate2.log`. It failed at the first checked draw of a populated Lumbridge preview after the camera fixture installed ramp heights and synthetic bridge/roof flags. Later accepted camera checks cleared the map, so those passes did not resolve this failure.

The original process state and the initial helper version were not archived in full. This investigation reconstructs the fixture family using the retained camera corpus/cache, the current populated control, and the archived pre-fix renderer component at `2026-10-25-extended-session-sol/renderer-before/src/dash3d/scene_raster225.c`. It is not presented as a byte-identical replay of the old process.

That archived renderer differs from the current production renderer only in its extra fatal condition and the explanatory comment added with the October25 correction:

```c
/* archived */ if (!ok || scene->tiles_remaining != 0) ... error = 6;
/* current  */ if (!ok) ... error = 6;
```

Using the archived component reproduces `error6/kind3` on the first reconstructed draw with109 deferred tiles. The current renderer returns normally for that same frame.

## Original Java comparison

Two actual native scene topologies were exported, including15,742 Ground identities,1,029 Location identities, shared references, bridge links, heightmaps, model-presence/maxY values, walls/decorations, occluders and camera parameters. The Java importer runs the original jar's unchanged `World3D.draw`, `drawTile` and occlusion methods. Underlay/overlay raster callbacks are overridden as explicit observations, and a verifier-only copy of `Model.draw` observes and returns before rasterization. The native source and original Java source are untouched. This comparison proves scheduler/return behavior, not image or triangle equivalence.

| Reconstructed camera sample | Current C | Original Java | Pending tiles | Underlay / overlay / model callbacks |
| --- | --- | --- | --- | --- |
| First draw0 | Normal return | Normal return |109 in both |364 /33 /154 in both |
| Later draw432 | Rejects missing footprint tile | NullPointerException at `World3D.java:1633`, reading the same footprint |480 in both |545 /105 /424 in both |

Thus the old requirement that every tile finish in one frame was a native defect. Java allows the first frame's deferred remainder. That defect is already fixed in the normal client.

The later rejection is different: the synthetic flags caused partial bridge shifts, leaving a location's footprint without a tile on its required plane. Java rejects that same topology. The current C rejection was retained; no guard was weakened to hide it.

## Fixture correction and retained regression

Original Java builds the map topology before the camera fixture changes height/flag arrays. The native camera helper previously let its runtime adapter construct the topology after those arrays had been replaced, creating artificial bridges. Its later workaround emptied the map entirely.

`verification/completion/tools/camera_candidate_control.c` now constructs the runtime scene from the built cache map before applying camera-only synthetic data. It explicitly asserts a populated map and reports3,547 cached placements. It no longer calls `world3d_reset` or `world_reset` to empty the scene.

The corrected control passes all825 framed packet observations,1,552 update ticks and50 full native draws. A diagnostic run records all50 draws with zero renderer errors and zero pending tiles. The existing3,307-state camera reference and camera safety tests also pass. No broad live gameplay or full-suite rerun was necessary for this verification-only change.

## Evidence and completion scope

`runs/2026-10-26-retained-renderer-sol/` contains the original-component copy, reconstructed/frozen controls, topology exporter and scenes, Java importer and verifier-only Model source, original/native comparison logs, `changes.patch`, `identities.json` and requirement-by-requirement `completion-audit.json`.

Key results:

- `archived-forced-before.exit=3`: archived renderer reproduces the first failure.
- `export-final.log`: current C allows draw0 and retains the genuine rejection at draw432.
- `java-0-final.exit=0`: oracle asserts normal Java return with109 pending.
- `java-432-final.exit=0`: oracle asserts Java's expected invalid-footprint exception with480 pending; this is not a valid-scene pass.
- `frozen.exit=0`, `camera-final2.exit=0`, `camera-native-final.exit=0`: populated regression and camera checks pass.

Exploratory setup failures are retained: an absolute compiler include path was unusable in the MSYS toolchain; the initial fixture filename was wrong; strict compilation found misleading indentation in the export helper; the first Java overlay placeholder omitted the source constructor's required10659 sentinel. Each was corrected without changing production rendering or expected reference behavior.

This closes the retained synthetic fixture investigation. It does not establish every scene/camera combination or retroactively qualify historical binaries. The normal executable remains SHA-256 `05046e6dadab75f30351d00ec504dfffbe7bda00c8b11b22fa7c2a73d8acfb56`; no renderer/application rebuild or installation was needed. No server, player save, map, original Java/cache, release archive or live process was modified.
