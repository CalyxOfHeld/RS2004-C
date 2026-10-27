# Current Windows revision-225 release — October 27, 2026

Current package: [2026-10-27-current-225-claude.zip](client-c/verification/completion/releases/2026-10-27-current-225-claude.zip). It supersedes 2026-10-26-current-225-sol.zip.

This working checkout is already updated. Keep using:

1. `start_local_server.bat` — wait for **World ready**.
2. `test_client.bat` — builds and runs the normal revision-225 client on port43595.

If an older server is running, press Ctrl+C in its window, wait for shutdown, then start it again. `test_client.bat` updates the client, not a running server. Do not start a second server on the same port. Existing characters keep their saves.

The package contains the current normal client/replay and SDL runtime, native build sources, server/content changes and compiled server scripts. It is an update for this existing rewrite workspace, not a standalone server/cache distribution. Its README explains applying the `workspace/` overlay to another copy of this same checkout. It introduces no alternate launch script.

Latest fixes (October 27) are three original-Java frame-parity corrections: the character-design model is no longer relit, the Wilderness "Level: N" skull and Arena indicator are now drawn, and the camera pitch clamp starts at Java's 0. An integrated scenario (3099 frames) and a nine-location world tour (3849 frames) match the original Java client frame for frame. Console log messages and the local website now use RS2004-C branding. Earlier fixes include continuous music volume with direct attenuation, sound-effect gain correction, authentic input phases, the normal build using the verified225 loop, console-only development messages, idle logout configuration, five repaired stair placements and clue-reader failure handling. The retained renderer fixture was compared with Java and its populated regression restored. Quest progress journals were introduced after225 and remain absent correctly.

Physical speaker/headphone confirmation of the latest audio is still pending. Historical external-host instrument tone/fade timing is not established. Recovered stair landings follow bundled map/source patterns; their precise historical tiles are not independently proven. See the package's KNOWN-LIMITS.md and CHECKS.md for evidence and scope.

Normal client SHA256: `ff10df0c1aa457d7ce15a087a6044b316f12a5c0bb5c04e039b8efe29c1a250f`.
