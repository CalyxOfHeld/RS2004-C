# Current Windows revision-225 release — October 26, 2026

Current package: [2026-10-26-current-225-sol.zip](client-c/verification/completion/releases/2026-10-26-current-225-sol.zip).

This working checkout is already updated. Keep using:

1. `start_local_server.bat` — wait for **World ready**.
2. `test_client.bat` — builds and runs the normal revision-225 client on port43595.

If an older server is running, press Ctrl+C in its window, wait for shutdown, then start it again. `test_client.bat` updates the client, not a running server. Do not start a second server on the same port. Existing characters keep their saves.

The package contains the current normal client/replay and SDL runtime, native build sources, server/content changes and compiled server scripts. It is an update for this existing rewrite workspace, not a standalone server/cache distribution. Its README explains applying the `workspace/` overlay to another copy of this same checkout. It introduces no alternate launch script.

Latest fixes include continuous music volume with direct attenuation, sound-effect gain correction, authentic input phases, the normal build using the verified225 loop, console-only development messages, idle logout configuration, five repaired stair placements and clue-reader failure handling. The retained renderer fixture was compared with Java and its populated regression restored. Quest progress journals were introduced after225 and remain absent correctly.

Physical speaker/headphone confirmation of the latest audio is still pending. Historical external-host instrument tone/fade timing is not established. Recovered stair landings follow bundled map/source patterns; their precise historical tiles are not independently proven. See the package's KNOWN-LIMITS.md and CHECKS.md for evidence and scope.

Normal client SHA256: `05046e6dadab75f30351d00ec504dfffbe7bda00c8b11b22fa7c2a73d8acfb56`.
