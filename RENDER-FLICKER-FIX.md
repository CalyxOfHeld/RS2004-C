# Layered model flicker fix

October 20, 2026. Historical flicker-verification executable SHA-256: `9071CE61356D92E2BBBE75DCBB0B32C2101ED754208E4FCF6DD080A8BE3A294C`.

The white carpet's black ornament is original revision-225 geometry, not a replacement texture. The cache model places the detail and its base on the same plane, with distinct face priorities. The native renderer previously ignored those priorities. Its integer camera-space rounding also warped a flat surface into slightly different triangle planes, and its mixed double/float depth values then rejected parts of the detail as the camera moved.

The shared model face sorter now follows the bundled original Model.draw priority groups and the 10/11 interleaving rules. Combined models retain both per-face and source constant priorities. Scene models and ground use one continuous camera transform with the original trig tables, and reciprocal depth stays in double precision with a relative floating-point roundoff allowance. Geometry, cache colors, actual textures and server rules are unchanged. There are no carpet/model-ID branches or geometric depth offsets in the fix. Item-icon rendering also uses the shared priority ordering.

## Evidence

All evidence is under `client-c/verification/completion/runs/2026-10-20-flicker/`.

- A user-authorized GPT-6 Luna tester wrote `tests/test_coplanar_visibility.c`. It discovers actual cache model1067, compares its complete black-detail mask against full rendering across28 yaw/shift views, checks reversed synthetic base/detail face order, composite priority propagation and separate nearer/farther model occlusion. Fixed source passes; preserved pre-fix source fails the detail-mask assertion. Synthetic test colors are framebuffer assertions, not game assets.
- Its separate cache sweep checks models1110 and1134 across80 additional yaw/translation views and22,383 overlapping pixels. Fixed source passes; preserved pre-fix source fails. These three assets and synthetic cases establish this shared bug class, not an exhaustive test of every asset in the game.
- `baseline-live.png` and `fixed-live-yaw0.png` show native before/after at the same actual world camera/player position; `matched-camera.json` verifies world coordinates despite different scene-region bases. NPC/player poses differ. No exact original-Java image parity is claimed.
- `fixed-reset-walk.log` and `frames/` contain40 actual SDL client frames, with two click-to-walk routes and15 camera positions. `fixed-walk-contact-sheet.png` contains labeled crops of those captures. `comparison.html` provides before/after and an interactive frame slider. Captures sample every10 rendered frames, not every server tick.
- The location fixture uses the unchanged developer teleport command to put a disposable account at the actual tutorial-island carpet, world3094,3107. Production server and original user saves were not reset. Developer teleport is test preparation, not evidence of natural travel/tutorial completion.
- `before-*.c`, `*-review.diff` and `tester/` preserve the reproducible failure, source review and independent testing. The delegated tester reviewed priority grouping/composite propagation against the bundled original Java Model.java and found no material occlusion risk in the reviewed path.

Full suite status: all33 test programs completed successfully with zero compiler warnings. The optional standalone live-login scenario is explicitly skipped without its opt-in credentials; separate current actual-client live captures are recorded above. The parent independently rebuilt the preserved baseline successfully with no warnings and reproduced the detail-mask assertion failure (tester/baseline-reproduction-*). The tester's initial baseline-build.log records an earlier mixed-source link failure; it is historical, not the successful reproduction log.

## Manual check

Close any running native client, then use `play_client.bat` for the rebuilt executable. The local production server remains on43595; reuse it while running. For later sessions start `start_local_server.bat`, wait for World ready, then play. Walk around the starting carpet and rotate the camera with arrow keys: its complete black design should remain attached to the white surface. The full game still has the gaps recorded in AUTONOMOUS_PLAN.md; see MANUAL-TESTING.md for credentials and controls.

This fixes the shared rendering cause for coplanar prioritized model details. Other rendering work such as normal sharing, occlusion/roof semantics, effects and full original-client parity remains on the broader roadmap.
