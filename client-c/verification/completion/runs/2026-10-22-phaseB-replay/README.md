# Phase B checkpoint B1/B2: client loop extraction and session record/replay

C client only. `javaclient/` was not touched.

## B1: the loop moved out of `main()`

The client was moved out of `platform/sdl2_main.c` into `src/client/client.c`. Its API is in `include/client/client.h`:
- `client_init` and `client_shutdown`;
- `client_begin_frame`, then `client_queue_event`/`client_set_held_keys` and `client_process_events`;
- `client_update` (one 20 ms cycle) and `client_draw` (one frame);
- `client_end_frame`.

The client has no SDL dependency. The platform supplies a clock and a BMP writer through `ClientPlatform`. `sdl2_main.c` (about 250 lines) now only does window, input translation, GameShell pacing and presentation. `platform/verification_input.h` moved into `client.c`; the originals are in `before-source/`.

Other changes:
- `title_screen` flames now use `math_random()` (Java's `Math.random()`) instead of a private seed.
- `--seed N` seeds Math.random.
- `--fixed-step` forces exactly one cycle per frame with no sleeping. It is for tests only.

**Equivalence check.** The pre-extraction build (`bin/rs2client_phaseB_before.exe`) and the new build were run through `tools/scenarios.ps1`: scene, rebuild, walk-to 36,29 and a UI input script. All runs used `--offline --seed 225 --fixed-step`.
- **54/54 captured BMPs are byte-identical**, and so are all stdout/stderr logs (`tools/cmplogs.py`).
- I re-checked this after the recording hooks were compiled in.

## B2: record and replay

`rs2client --record PATH` writes a text session log containing:
- the seed, args and username (**never the password**);
- per-frame input events and held-key changes;
- one `U` per update cycle;
- every client clock read;
- every socket result (connect, available, read bytes, write bytes and result, close);
- a framebuffer hash per frame (FNV-1a 64);
- a final `Q` end marker.

`bin/rs2replay.exe LOG [--out DIR] [--dump-every N] [--dump-frame N]` runs the same `client.c` headlessly:
- No window, audio or socket is opened.
- Socket results come from the log.
- **Outgoing bytes are compared with the recording,** and so is every frame hash.
- The first difference prints `[REPLAY] desync at log line N: ...` and dumps that frame.
- `RS2_PASSWORD` must be set in the environment to replay a login.

The login password is RSA-encrypted before it is written (`protocol.c`), and a scan of the log confirmed no plaintext. The log does contain the session's ISAAC-encrypted traffic and seed, so treat recordings of real accounts as private. These use the disposable `claudesm22`.

### Results

- **`offline-record/`:** 150-frame UI script, wall-clock pacing (no `--fixed-step`). PASS. All 6 captures, including terrain debug captures, are byte-identical to the recording.
- **`live-record/`:** live server, PID 2352 (the user's, left running).
  - Autologin as `claudesm22` (reply 18), REBUILD_NORMAL, then the smoke input. F10 and the logout button triggered server opcode 142, and the client returned to title state 0. 440 frames.
  - Replay: `PASS REPLAY: 440 frames reproduced (3187 log lines)`, and all 7 captures are byte-identical (`cmp`).
  - `controls.txt` lists the negative controls, each of which must fail:

    | Control | Result |
    |---|---|
    | One inbound bit flipped | desync at frame 1 (framebuffer hash) |
    | Log truncated at 60% | desync: "log ended, client wanted NR" |
    | Same-length wrong password | desync at the login write: "outgoing bytes differ" |

  - Before the `Q` marker was added, a log cut at a frame boundary passed. That was found by this control and fixed.
- **Saves:** 38/38 pre-existing saves are byte-identical to `live-smoke/saves-before/`. Only `claudesm22.sav` changed.
- **`make test`:** all tests pass, 0 warnings.

`live-smoke/` is the earlier smoke of the extracted loop. Its second run got reply 5 because the previous session was still logged in.

## B3/B4: the Java parity oracle and `make parity`

**What it does.** `reference/src/ParityOracle.java` runs the unchanged `javaclient/build/libs/rs2client.jar` on a recorded C session:
- It drives `update(437)` once per logged `U` and `draw()` once per `F`, synchronously. There is no GameShell thread, and the flame thread is suppressed with `flameActive`.
- It emulates signlink, serving `pack/client/maps`, and runs a local HTTP server for `crc` and the archives, as engine `web.ts` does.
- **Login:** it uses a local ServerSocket. Math.random is synced from the C `[PARITY] login ... random_state=` line, so the ISAAC seeds and the anticheat rolls match.
- **After login,** the inbound stream is released cycle by cycle, exactly as C read it, and the outgoing stream is captured.
- **Every frame,** the outgoing bytes are compared exactly. A mismatch is reported with the decoded opcodes, using the ISAAC seeds and the sizes in `ClientGameProt.ts`.
- **The composited PixMap areas** are hashed (FNV-1a 64 over the RGB). `rs2replay --regions --region-hashes` hashes the same rectangles from the C framebuffer.

**Tools.**
- `tools/record_live.ps1 -OutDir D -InputScript S` records a live session. It snapshots the saves and reports which changed, and reads `RS2_USERNAME`/`RS2_PASSWORD` from the environment.
- `tools/parity.ps1 -Log L -OutDir D [-DumpFrame N]` runs the whole comparison. `tools/parity_compare.py` writes the summary.
- `make parity` (in `client-c`) runs it on `live-record6/session.rec` and needs `RS2_PASSWORD`.

### Results: `live-record6` (login, logout tab click, logout button, logout)

- **Protocol: MATCH.** The login packet is identical (105 bytes), and every outgoing byte matches Java across all 346 in-game frames.
- **2D areas: still differ.**
  - `areaMapback` differs on every frame.
  - `areaChatback` differs on 15 frames, `areaBackbase2`/`areaBackhmid1` on 13, and `areaSidebar` on 27 from frame 320 (the logout tab).
  - The `imageTitle2-8` names that appear on in-game frames are unexplained; they may be a reflection naming artifact in `regionName`.
- **`areaViewport`** differs on every frame, as expected until Phase C.

### C changes that parity found (all from `client.java`, Rev 225)

1. **Wall-clock keepalive removed.**
   - C now sends ANTICHEAT_CYCLELOGIC4 (236) every 111 cycles.
   - It sends NO_TIMEOUT after 50 cycles with no output, and four NO_TIMEOUTs during buildScene.
   - **Output is buffered** like Java's `out` (5000 bytes) and written once at the end of each in-game update. `ClientStream.buffered`, `client_stream_flush` and `client_stream_pending` were added.
2. **At most five packets are read per cycle** (client.java:7665).
3. **REBUILD_NORMAL:**
   - The same centre zone is ignored while a scene exists.
   - REBUILD_GETMAPS (150) lists missing or CRC-mismatched map files and is always sent.
   - The scene is built on the next PLAYER_INFO (sceneState 1→2).
   - TODO: the map data packets that answer 150 are not handled.
4. **Anticheat:**
   - CYCLELOGIC1, 2 (draw time), 3, 5 and 6, plus EVENT_CAMERA_POSITION (189) while an arrow key is held.
   - The login rolls of the camera and minimap offsets, angle and zoom, and the orbit yaw.
   - The 500-cycle drift, and IDLE_TIMER (70) after 4500 idle cycles. Mouse and key events reset the idle count.
5. **Camera and minimap now apply the offsets.**
   - The camera orbit tracks the player plus the offset, and the scene yaw adds the angle.
   - The minimap rotation and dots use yaw plus angle and zoom; the compass uses the plain yaw.
   - The minimap click and the MOVE_MINIMAPCLICK trailer carry the angle and zoom.
6. **Invented mechanics removed.** C switched tabs on F1–F12 and closed interfaces and menus on Escape. Rev 225 has neither: GameShell maps F-keys to 1008+, which `client.java` never reads, and drops Escape.

**Offline equivalence (`scenarios.ps1`).** The scene, walk and rebuild captures are bit-identical to the previous build. Only the `ui` scenario changed, from frame 60, which is where its F2, F4 and Escape presses now do nothing.

**Known ordering gap (TODO in `client.c`).** C acts on clicks when the event arrives. Java acts in `handleMouseInput`, after CYCLELOGIC6, 5 and 3 and the camera packet. A click on the same cycle one of those fires would reorder bytes.

### Pitfalls found while building it
- **Replays need `RS2_PASSWORD` even for the title frames,** because the masked password is drawn. `parity.ps1` now refuses to run without it.
- **Run the C client from `client-c`,** since the cache path is relative. Both scripts pin the working directory.
- **After logout, the Java flame thread draws concurrently.** The oracle snapshots its PixMap map under a lock.

## Builds

| File | SHA-256 |
|---|---|
| `bin/rs2client.exe` (also `rs2client.lastgood.exe`, `rs2client_bridge.exe`, `rs2client_decor.exe`) | `A03BE286C5DCE86FB5949D7EC7079E7C0D5B2F1B4770823529966746AED44CF8` |
| `bin/rs2replay.exe` | `80A1D02D3703175DDA857F68578A1331AB7E75D794FD9574E77ED1984B6DF231` |
| `bin/rs2client.lastgood-phaseA.exe` (Phase A) | `E931CB6C…FEBE` |
| `bin/rs2client.lastgood-0510.exe` (earlier lastgood) | `0510EADD…97E4` |

The B1/B2 build `341AB6F2…6C2D` was superseded by this checkpoint.
