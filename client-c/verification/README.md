# Latest delivery: scenery ordering and textures — October 20, 2026

The current build and verification are described in `../.. /HANDOFF.md`
(the rewrite-root `HANDOFF.md`). Both project handoffs are synchronized.

- Executable: `../bin/rs2client.exe`, SHA-256 `D66AAC9B608F01184AC303C9160C5F2759663220793433988DF0CF2D0E97E3AA`.
- Final build: `scenery-build.log`; full tests: `scenery-tests.log`.
- Actual-client runs: `scenery/client-verification.txt` and `scenery/after-*.png`.
- Live login/rebuild: `scenery/live.log`, `scenery/live.png`, `scenery/server.log`.
- Earlier terrain milestone: `handoff.terrain.md`, original `after-*.png` and logs.

From `client-c`, run `make all`, `make test`, then
`./tests/verify_terrain.ps1 -OutputDirectory verification/scenery`.
The script retains client and opaque-world-pass PNGs and removes temporary BMPs.
The full-client images include deferred alpha; the opaque world captures do not.

The client requires the existing adjacent engine cache. Run the rewrite-root
`play_client.bat --offline` or use `--port 43595` for an existing server on that port.
The temporary live verification server has been stopped; no .env file was changed.

Known remaining differences include terrain blending/lighting, animated textures,
Java priority effects, bridge/roof semantics, visibility-aware picking and scenery
ownership on rebuild. Full reference pixel parity is not claimed.
