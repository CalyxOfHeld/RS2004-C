# Current-build frame parity + world tour — Claude, October 27, 2026

Scope: requested by the user after checkpoint commit 9742d72. (1) Re-record one integrated scenario on the current normal build and strict-replay it against the original Java client, comparing the whole screen. (2) Run a "world tour" comparison at non-Lumbridge locations. Not 244. No human-play items.

Tools: existing `tools/record_live.ps1` and `tools/parity.ps1` (ParityOracle on original rs2client.jar + strict rs2replay). Disposable auto-created accounts are listed in each `account.txt`. Passwords were supplied through the process environment only. Every recording reports `saves changed: none`.

## Results

| Run | Binary | Frames | Viewport/screen differing | Result |
|---|---|---|---|---|
| integrated/ | 05046e6d (pre-fix) | 3099 | 144, first at 8 | DIFFERS |
| tour/ | 05046e6d (pre-fix) | 3849 | 473, first at 22 | DIFFERS |
| integrated-fixed/ | fix 1 only | 3099 | 0 | MATCH |
| tour-fixed/ | fix 1 only | 3849 | 345, first at 1319 | DIFFERS |
| **integrated-final/** | **a63f9ca4** | **3099** | **0 (all 25 regions)** | **PARITY MATCH** |
| **tour-final/** | **a63f9ca4** | **3849** | **0 (all 25 regions)** | **PARITY MATCH** |

The integrated scenario reuses the input from `runs/2026-10-23-c5-integrated-sol/release/input.txt`. The tour input (`tour/input.src.txt`) makes nine `::tele` stops, each followed by a minimap-click walk. World tiles are approximately: 3213,3428 (Varrock), 3086,3228 (Draynor), 3293,3174 (Al Kharid), 2918,3176 (Karamja), 2965,3380 (Falador), 3082,3420 (Barbarian Village), 3090,3525 (Wilderness level 1, frame 2800), 3237,9866 (underground, z>6400) and 3023,3208 (Port Sarim). `tour-final/parity/c/replay-frame-002900.bmp` shows the Wilderness skull and "Level: 1" drawn the same as Java.

## Fixes (all sourced from reference/src/deob, the original Java)

1. **Design-screen model relit twice.** Client.updateInterfaceContent lights the design model with `calculateNormals(..., true)`, and applyLighting then clears `faceColor` (no face has faceInfo&2). Component.getModel only relights when `faceColor != null`, so Java draws the model unrelit; C relit it. Fix: new `Model.colourless225` flag set in `player_entity_get_design_model`, honoured by `component_model_colourless225`. This accounted for all 144 integrated differences and the early tour frames.
2. **Wilderness level / Arena indicator missing.** Port of `client.drawWildyLevel` (client.java:1590) and its draw3D overlay (client.java:6639): skull headicons[0] at (472,296), "Level: N" centred at (484,329) in 0xFFFF00, and the Arena icon headicons[6]. New functions `combat_wilderness_level225`, `combat_world_location225` and `combat_location_draw225` are in `src/ui/combat_feedback.c`. The existing multiway fixture control (`tools/reset_multiway_reference_control.h`) now also asserts the Java wildernessLevel/worldLocationState for every sampled tile. This fixed tour frames 2813–3154.
3. **Camera pitch clamp start value.** `client.cameraPitchClamp` (client.java:888) has no initializer (Java 0) and converges in integer /24 and /80 steps (client.java:6721), with draw pitch `max(pitch, clamp/256)` (client.java:2179). C started it at 32768, which caused isolated single-frame camera differences after teleports (tour frames 1319, 1325 and 1362). Fix: `camera_init` sets `pitch_clamp = 0`.

## Build and suite

Final normal build: rs2client.exe SHA256 `a63f9ca468eda929b981d434c91f2ce308ef4eae62959233f140764e692f11b5`, rs2replay.exe `2e3fa17bec7264b355d5ffaa5c2ee01c2063589e69388a958a96ff45146ccf4e`. This replaces the normal client 05046e6d…, so the 2026-10-26 release zip no longer holds the current binary. `suite-final.log`: `make test` ALL TESTS PASSED with 0 warning lines. The protocol "Receive failed" lines are expected negative-test output.

## Limits

Two recordings on one server, on Windows only. These parity runs prove frame equality for these inputs only: no combat in the Wilderness, no Arena, no other players. Human play was not exercised. Pre-fix recordings no longer strict-replay on the new binary; they desync at the first previously differing frame, which is expected. The release package was not rebuilt.
