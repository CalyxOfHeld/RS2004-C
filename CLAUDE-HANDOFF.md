# Claude handoff — 2004scape C Rewrite

Date: 2026-10-21  
Workspace: `C:\Users\USER\Documents\2004scape-LostCityRS\2004scape C Rewrite`

## Current state

The native client is at a verified checkpoint. The latest installed executable is:

`client-c/bin/rs2client.exe`

SHA-256: `0510EADD01B31BC64212ABAA78D6A71B62FFE62A4095F6356ED4CA0BCFF597E4`

`rs2client_bridge.exe` and `rs2client_decor.exe` match this hash. Use `play_client.bat` to launch it. The previous build (`CD6A014A…7C4A`) is preserved in `client-c/verification/completion/runs/2026-10-21-logout-title/before/`.

The existing server is running on ports 43595, 8889 and 8899 under PID 2352. Check ownership before starting another server or client, and do not stop the user's running processes.

## Completed in the latest checkpoint (logout/login recovery)

- Logout now returns to the original revision-225 title screen, and logging in again from it works.
- **Title screen.** The background JPEG decoder (`src/gfx/jpeg.c`) is bit-exact to Java's decode. The title PixMaps, logo, titlebox/buttons, `updateTitle`, `drawTitleScreen` (states 0/2/3) and the flames are ported (`src/ui/title_screen.c`).
- **Login and logout.** `login()` reply messages and the success reset are ported, as is `logout()`.
- **Session-end paths.**
  - Opcode 142 and malformed packets call `logout()`.
  - I/O loss calls `tryReconnect` (opcode 18, reply 15, "Connection lost" overlay).
  - Clientcode 205 sets `idleTimeout = 250`.
  - Reply 1 is retried after 2 s.
- **Launcher.** The launchers' environment credentials prefill the login box and press Login once.
- **Tests.**
  - The full native suite passes: 53 programs, zero warnings.
  - The new `test_title_screen` compares 1,182,628 pixels against `TitleOracle.java`, which runs the unchanged Java `loadTitle`/`drawTitleScreen`/`drawFlames`. It also covers the input rules and a loopback login/reconnect handshake.
- **Live checks.** On PID 2352, a login → logout → title → typed re-login → logout cycle passed with `sollogc21`, and the normal launcher passed with `sollogd21`.
- **Evidence:** `client-c/verification/completion/runs/2026-10-21-logout-title/` (README, logs, PNGs, `source-review.diff`, oracle source), plus the `logout-title-checkpoint.zip` overlay with its SHA-256 sidecar.

## Open boundaries from this task

- Flame frames use a native RNG, because Java's `Math.random` is unseeded. Only `drawFlames` is pixel-compared.
- A live connection loss was not induced; reconnect is tested against a fake server only.
- Not ported:
  - the `idleNetCycles > 750` reconnect;
  - the AFK idle timer (packet 70);
  - camera/minimap anticheat randoms;
  - `drawProgress`.
- `antigravity.sav` changed at 11:40:37 during this task without any run using that account, as in earlier user sessions. It was left untouched; the other 34 saves are byte-identical.

## Authoritative reading order

1. `AGENT.md`
2. `AUTONOMOUS_PLAN.md`
3. The newest section of `HANDOFF.md`
4. `client-c/verification/completion/progress.md`
5. `capabilities-current.md` and `SOURCE-INVENTORY.md`

Use only the bundled revision-225 Java client, the TypeScript port and the engine/content as sources of truth. Do not search for deferred Cursor/Grok audit material. Preserve all saves, caches, content, prior checkpoints and nested repository histories. Do not commit unless explicitly requested.

## Next task

Continue `AUTONOMOUS_PLAN.md` with the remaining gates:

- roofs and occluders;
- texture/palette parity;
- setBridge traversal;
- dynamic relighting;
- the `idleNetCycles`/AFK idle timers;
- the full gameplay audit.

Reproduce before editing, compare against the bundled Java client, add a regression through the real client path, and keep the installed executable as last-known-good until a replacement is verified.

## Operational notes

- Native builds from Git Bash fail with "unable to make temporary file". Build from PowerShell with `C:\llvm-mingw\bin` on PATH and `TMP=$env:TEMP`.
- `make` overwrites `bin/rs2client.exe`. Restore the last-known-good build there until the new one is verified.
- The client changes into its own directory to find caches. Run candidates from `client-c/bin` under another name, and pass paths without spaces (relative to `client-c`) to launcher scripts.
- Disposable accounts `sollogb21`, `sollogc21` and `sollogd21` are retained.
- If interrupted, leave a runnable build, bounded evidence under a new run directory, updated `HANDOFF.md`, this file and `progress.md`.
