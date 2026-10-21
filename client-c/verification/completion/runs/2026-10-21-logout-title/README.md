# Logout returns to the original title screen; in-client login — October 21, 2026

Native SHA-256: `0510EADD01B31BC64212ABAA78D6A71B62FFE62A4095F6356ED4CA0BCFF597E4`. Installed `client-c/bin/rs2client.exe`; the launcher update `rs2client_bridge.exe` and `rs2client_decor.exe` match. The previous build `CD6A014A…7C4A` is preserved as `before/rs2client.exe` (binaries.sha256). The live-tested `candidate/rs2client.exe` differs from the installed build only in the 4 PE link-timestamp bytes (offsets 129–130 and 345605–345606); its code is identical.

## Problem (reproduced first)

Disposable account `sollogb21`, `before.input` (F10, click "Click here to logout"): the client sent IF_BUTTON 2458. Server opcode 142 then closed the socket and set `live=0`, but the world stayed on screen with a synthetic "You have logged out." chat line (`before.log`, `before-after-logout.png`). It was impossible to log in again without restarting.

## Change (bundled revision-225 `deob/client.java` is the source)

- **`src/gfx/jpeg.c`** is a baseline JFIF decoder for `title.dat`, following the arithmetic of libjpeg 6b, which Java's `Toolkit`/`PixelGrabber` use: islow IDCT, h2v2 fancy upsampling, table-driven YCbCr. All 210,140 pixels match `Pix32(byte[], Component)` exactly.
- **`src/ui/title_screen.c`** ports these Java methods:
  - `loadTitle`/`loadTitleBackground`: nine PixMaps with the original offsets, a mirrored second blit, and the logo at `screenWidth/2 - w/2 - 128, 18`.
  - `loadTitleImages`: titlebox, titlebutton, 12 runes, three flame gradients, and the flame-buffer seed.
  - `updateTitle`: button rectangles, field rows [266,281) and [281,296), CHARSET filter, 12/20 length caps, backspace, Tab/Enter/13 field toggles, and the per-update key discard.
  - `drawTitleScreen`: states 0/2/3, the blinking `@yel@|` cursor, asterisks, and one- or two-line messages.
  - `updateFlames`/`updateFlameBuffer`/`drawFlames`, including Java's signed `>> 8` blend.
  - `unloadTitle`.
  - Every `login()` reply message, including code 11's double assignment and unknown codes leaving "Connecting to server...".
- **`platform/sdl2_main.c`** gains a `client.ingame` state. While not in game, the title updates and draws, and all input goes to it (GameShell key queue and one latched click per update).
  - **Login** shows "Connecting to server..." for one frame, then calls `protocol_login_ex`.
  - **Success (2/18)** applies the `login()` reset for every state the native client has: idle timeout, hint arrow, menu, chat history, obj/spell selection, sceneState, orbit yaw, destination flag, all players/NPCs and a new local player, local pid, projectiles/spotanims, merged/spawned locations, sticky/chat/viewport/sidebar interfaces, selected tab 3, flashing tab, and design. It then unloads the title (`prepareGameScreen`).
  - **Other replies** show the original two-line message.
  - **`logout()`** closes the stream, sets title state 0, clears username and password, clears loc/actor model caches, resets the scene and all four collision maps, stops MIDI, and reloads the title.
  - **Callers** follow the Java paths:
    - Opcode 142 and every malformed/unknown/oversize packet (T1/T2 `reporterror` → logout) call `logout()`.
    - A read or keepalive I/O failure calls `tryReconnect`. That logs out if `idleTimeout > 0`. Otherwise it draws "Connection lost" / "Please wait - attempting to reestablish" with plain12 shadow at viewport (256,143)/(256,158), clears the flag, and sends a reconnect login (opcode 18). Reply 15 resumes; any failure logs out.
  - The synthetic chat lines "Connected to LostCity 2004 Server.", "You have logged out." and "Connection lost." are removed. The original has none of them.
- **`src/net/protocol.c`**:
  - `protocol_login_ex(..., reconnect)` sends 18 or 16, retries reply 1 after 2 s as `login()` does, and treats 15 as in-game.
  - `protocol_login` is kept as a wrapper.
  - `GameSession.protocol_error` separates framing failures (unknown opcode, oversize payload) from I/O failures.
- **`src/ui/client_actions.c`**: clientcode 205 (logout button `try_logout`, component 2458) sets `idleTimeout = 250` and still sends IF_BUTTON. `idleTimeout` is decremented each game update.
- **Launcher compatibility:** when `RS2_USERNAME`/`RS2_PASSWORD` are set (both `play_client` launchers set them), the title opens on the login box with them filled in and presses Login once. Without them the raw executable now shows the title and waits for input instead of exiting. `--offline` is unchanged.

## Verification

- **Full suite:** `make` and `make test`, **53 programs, zero warnings** (`build.log`, `suite.log`, exit 0). The new `test_title_screen` covers:
  - **TITLE-01.** `oracle-src/TitleOracle.java` runs the unchanged client `loadTitle`, `drawTitleScreen` and `drawFlames` on the real title archive. `client.flameActive` is preset so the flame thread is not started, and `screenWidth`/`screenHeight` are 789/532 as `initApplication(532, 789)` sets them. **1,182,628 pixels match exactly** (`tests/fixtures/title-reference.bin`):
    - all eight background/logo PixMaps;
    - five `drawTitleScreen` states (welcome, login box with and without cursor, two-line message, maximum-length fields with `£`, new-user page);
    - seven `drawFlames` gradient branches (cycle0/cycle1 at 900/500/100, and none), on fixed flame and line-offset buffers.

    The test also checks that the composited frame equals every PixMap at its client position, and that a logout reload restores identical artwork.
  - **TITLE-02.** `updateTitle` buttons and edge pixels, right-click ignored, field rows, charset/length/backspace, Tab/10/13 toggles, discarded keys, Login/Cancel, reply messages and the logout reset.
  - **TITLE-03.** A loopback fake server checks login opcode 16, reconnect opcode 18 with reply 15, reply 1 retried after ≥2 s with a second 16, and reply 5 rejected.
  - **`test_protocol_transport`** now asserts that oversize and unknown opcodes are framing errors and that a mid-payload close is an I/O error.
- **Live re-login on the user's running server PID 2352 (port 43595).** Account `sollogc21`, `after.input`, `after.log`, exit 0:
  1. Launcher-style login (reply 18).
  2. F10 → logout button sends IF_BUTTON 2458 → server 142 → `[STATE] logout -> title state=0`.
  3. Click Existing User, type the username, Tab, type the password, click Login → reply 18, in game.
  4. A second button logout returns to the title again.

  Frames: `after/frame-200.png` (title after logout, flames burning), `frame-325.png` (filled login box), `frame-500.png` (back in game), `frame-700.png` (title after the second logout).
- **Normal launcher** `play_client.ps1` with disposable `sollogd21`: automatic login, button logout, title (`launcher-title.png`), `launcher.log`, exit 0. `launcher-harness-quoting.*` is an earlier attempt where my caller failed to quote a path containing spaces. The client rejected the split argument before connecting. It is not acceptance evidence.
- **`--offline` smoke:** `offline.log`/`offline.png`, exit 0.

## Boundaries (not claimed)

- **Flame randomness.** Java's `Math.random()` is unseeded, so a native flame frame cannot equal a Java frame. `updateFlames`/`updateFlameBuffer` are line-for-line ports driven by a xorshift stand-in. Only `drawFlames` is compared, on fixed buffers.
- **Flame thread.** The flame thread is run from the main loop at most every 40 ms (two `updateFlames` plus one `drawFlames`), approximating `runFlames`' adaptive sleep. All title PixMaps are recomposed each frame instead of on `redrawBackground`; the final image is the same.
- **Reconnect.** Reconnect is tested against the fake server only. A live connection loss was not induced, because that would need stopping the user's server.
- **Not ported:**
  - `idleNetCycles > 750` → tryReconnect, and the 4500-cycle AFK idle packet 70 timer: native has no equivalents yet.
  - Login-reset items with no native counterpart: camera/minimap anticheat randoms, wave queue, friend count, InputTracking, oplogic counters.
  - Obj icon/model and spotanim model cache clears: no native clear functions; these are caches only.
  - `drawProgress` loading bar.
- **Tab interfaces** are not reset on login, as in the original. The server resends them.

## Saves and processes

- **Saves:** 35 saves existed at task start (`saves-before.json`). 34 are byte-identical (`saves-after.json`).
- **`antigravity.sav`** changed at 11:40:37. None of this task's runs used that account; every run used `sollogb21`/`sollogc21`/`sollogd21`. That matches earlier handoffs, which record it changing during the user's own sessions. It was not modified or restored.
- **New disposable saves** `sollogb21`, `sollogc21` and `sollogd21` are retained.
- **Processes:** the server PID 2352 was not stopped. No user client was running at the checks.

No cache, content, server or launcher-source edits, commits, delegates or external-audit searches.

`source-review.diff` holds the complete before/after diff of changed files plus all new files. The baseline `before/tests/test_protocol_transport.c` was taken from `wall-decoration-checkpoint.zip`; its `sdl2_main.c` matches `before/sdl2_main.c`.
