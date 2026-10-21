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

## Builds

| File | SHA-256 |
|---|---|
| `bin/rs2client.exe` (also `rs2client_bridge.exe`, `rs2client_decor.exe`) | `341AB6F2BF27D4FBF7CDBB991C695C224E2D4EAA3679DF7ED8AC5ABE8B4D6C2D` |
| `bin/rs2replay.exe` | `30D915ACDC8CA13448190BFC0F4B0F62203EDC79F0E4DF881C17C3E65938F3DF` |
| `bin/rs2client.lastgood.exe` (Phase A) | `E931CB6C…FEBE` |
| `bin/rs2client.lastgood-0510.exe` (earlier lastgood) | `0510EADD…97E4` |

## Not yet done (B3/B4)

- **Java oracle:** feed the same recorded inbound stream to the unchanged `client` under `reference/src`, with seeded Math.random and ISAAC from the logged seed. Compare outgoing packets and the composited areas per tick.
- **`make parity`.**

Replay so far proves the C client is deterministic given its inputs. It does not yet prove the C client matches Java.
