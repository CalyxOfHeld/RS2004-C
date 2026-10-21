Implemented and verified October21,2026. See runs/2026-10-21-shared-lighting-sol/README.md for results and exclusions. The task below is the original implementation brief.

# Implementation task: revision-225 scene shared lighting

## Goal

Port the bundled revision-225 static-scene normal merge and hidden-face behavior into the native renderer. This is the next bounded renderer task, identified by the bridge follow-up research. Apply it to eligible `sharelight` scenery generally; do not special-case Lumbridge IDs or coordinates.

The user reported that the restored bridge side looks wrong. Original-client probe evidence shows why this gap can affect it: for two adjacent real `loc3007` railings, `World3D.buildModels` changes 8 of 10 face shades and hides the 2 matched internal faces, for both rotations 1 and 3. This small flat probe proves an algorithmic difference, but does not prove that lighting alone explains the user's screenshot.

## Reference behavior

- `javaclient/src/main/java/jagex2/dash3d/World.java`, `finishBuild`: after building terrain, calls `World3D.buildModels(ambient=64, light=(−50,−10,−50), contrast=768)` before `setBridge` tile relinking.
- `javaclient/src/main/java/jagex2/config/LocType.java`, `getModel`: calculates each model's face normals with location ambient/contrast and `applyLighting = !sharelight`. For `sharelight` models it preserves the normal accumulators; for hillskew it copies the model and changes vertex heights after normal calculation.
- `javaclient/src/main/java/jagex2/dash3d/World3D.java`, `buildModels`, `mergeLocNormals`, and `mergeNormals`: merges static scenery normals over nearby tiles on the same and next source plane. It compares terrain-relative model heights, adds each model's original vertex normal to the other model's working normal, and hides fully matched faces only when at least three vertices were matched and `hideFaces` is true.
- `javaclient/src/main/java/jagex2/graphics/Model.java`, `calculateNormals` and `applyLighting`: flat faces (face-info bit 0 set) get their location-specific shade while normals are built. Gouraud faces use the merged working normal and the scene-wide light in `World3D.buildModels`. After final lighting, normal arrays are discarded.
- `Model.faceInfo == -1` is the hidden-face sentinel; the native face render-type byte is unsigned and `model_draw` already skips value `255`.

## Native integration points

- `client-c/src/gfx/model.c`: `model_calculate_normals` currently calculates and discards vertex accumulators, so the scene cannot merge them later. Preserve both immutable original and mutable working accumulators for deferred `sharelight` models. Keep ordinary model lighting behavior unchanged.
- `client-c/src/config/loc.c`: `loc_get_model_frame` performs per-location normal calculation, clears Gouraud shades for `sharelight`, then returns an owned copy. Preserve deferred normal state through the cached model and returned model copies. The flat-face shades must retain location ambient/contrast.
- `client-c/include/dash3d/world3d.h` and `client-c/src/dash3d/world3d.c`: `WorldLocation` holds the model, source plane, anchor, and tile footprint; its draw placement is centered from footprint dimensions. Add per-model light metadata only if needed by the final light pass; the original shared-light Gouraud pass uses common scene values (64,768,−50,−10,−50).
- `client-c/src/dash3d/world.c`: `place_location` inserts map locations, currently calls individual lighting as a fallback for `sharelight`, then applies hillskew. `world_build` runs after all region locations have been inserted by `map_streamer_rebuild`, making it the natural whole-scene merge/build point.
- `world_change_location` and `location_anim_tick` can replace scenery models after the static build. If the implementation claims those paths remain parity-safe, refresh neighboring deferred lighting after replacement and ensure removed models no longer contribute. Do not let repeated builds accumulate normals or retain hidden faces from an earlier scene state.

## Required implementation behavior

1. Retain per-vertex original and working normal sums for `sharelight` models through location placement and hill skew. The original sums must remain unchanged across repeated merge passes.
2. Port original same/next-level `mergeLocNormals` candidate traversal with terrain-relative average-height offsets, each candidate's footprint, and the original boundary pruning.
3. Port exact vertex matching and accumulate both models' original normal sums into the opposite working sum.
4. For the original `hideFaces=true` cases, mark each face whose three vertices matched in that merge as hidden using native sentinel `255`. Do not hide faces in `hideFaces=false` pairings.
5. Shade eligible Gouraud faces only after all merges, using original scene ambient/contrast/light values and the original integer arithmetic. Preserve flat-face shades computed with location ambient/contrast. Release or reset temporary normals only after shading.
6. Apply the pass to `sharelight` static scenery generally. Preserve ownership, alpha sharing, cache isolation, hillskew, placement and model cleanup semantics. Keep the separately tracked original `setBridge` relinking/draw-level work out of scope.

## Acceptance checks

- Add native tests for exact shared vertex matching, both-way normal accumulation, the three-matched-vertices threshold, hidden-face marking in both models, `hideFaces=false`, face-info flat faces, repeatable rebuilds, and terrain-height offsets across source planes.
- Extend the revision-225 Java oracle from `client-c/verification/completion/reference/src/BridgeLightingProbe.java` (or add a new oracle) to emit comparable per-vertex normal sums, per-face shades and face-hidden state for adjacent real `loc3007` models in rotations 1 and 3. Compare native values to this independently run oracle; retain source and output in a new run directory.
- Exercise a small real terrain neighborhood with bridge-body and rail models from the bundled cache, including hill skew and source-plane neighbors. Confirm the two end faces are hidden only where the reference hides them and all visible face shades match.
- Run the full native suite with zero warnings. Capture the same Lumbridge view in the native client and bundled Java client using the same cache, camera and scene state; describe any remaining discrepancy without claiming whole-scene parity from unit checks.
- Recheck any dynamic-location replacement or animation path affected by retained normals. Document exact exclusions if a path cannot safely be included.

## Evidence and boundaries

Research probe source: `client-c/verification/completion/reference/src/BridgeLightingProbe.java`.
Research output: `client-c/verification/completion/runs/2026-10-21-bridge-sol/lighting-probe.log` (exit 0); the log reports 8/10 shades changed and 2/10 faces hidden for both tested orientations.
User screenshot: `client-c/verification/completion/runs/2026-10-21-bridge-sol/user-bridge-followup.png`.

Do not alter cache, content, server rules, saves, credentials, launcher behavior, or the existing user session to imitate the screenshot. Keep all work and evidence under this project. The previous bridge checkpoint ZIP is immutable; produce a new cumulative checkpoint only after implementation and verification are complete.
