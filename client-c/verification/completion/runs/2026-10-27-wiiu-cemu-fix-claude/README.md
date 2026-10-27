# Wii U step 3: GamePad input, keyboard, login and world loading in Cemu — Claude, October 27, 2026

Scope: user-authorized ("title screen shows, go ahead with the keyboard"). This run covers the keyboard and input work, the user's Cemu login test, and the fix for the world never loading. Not 244. No real hardware was used.

## Result

In Cemu 2.6 the Wii U build:
1. logs in to the local server (the user confirmed "it works now, logged in");
2. rebuilds the map (`Live REBUILD_NORMAL: 9 mapsquares, base=(3040,3056)`);
3. stays in game (`scene_state=2`, sending regular keepalive and anticheat packets), with no draw failures and no corrupt models.

The logout after about 90 s in unattended runs is the original idle timer (`IDLE_TIMER` after 4500 cycles with no input, as in client.java). It is not a fault.

## Changes (all Wii U-only unless noted)

- **GamePad touch:** SDL's Wii U port only reads touch for an opened joystick, so `sdl2_main.c` opens every joystick, including hot-plugged ones. Touches arrive as synthesized left-button mouse events.
- **swkbd text entry:** tapping a text field opens the system keyboard; OK delivers the text.
  - Shared code: new `client_text_field()` in client.c. It reports the active field for title login, chat, private prompt, amount and report, plus a tap latch from `title_screen_update`. The PC build does not call it.
  - Cemu keeps the swkbd OK state set until the next Appear, so OK is honoured only while our keyboard is shown, and events are pumped once per frame.
- **Default port 43595,** matching `start_local_server.bat`; `sd:/wiiu/apps/rs2004c/server.txt` still overrides it.
- **16 MB client stack:** an RPX main thread has 64 KB. The client now runs on a CPU1 OSThread with a memalign'd 16 MB stack, the same size as the PC `--stack`.
- **SD logs:** stdout and stderr go to `sd:/wiiu/apps/rs2004c/log.txt` and `errors.txt`.
- **Test login:** an optional `autologin.txt` ("user password") next to server.txt logs in like `RS2_USERNAME`/`RS2_PASSWORD` on PC. It is absent by default.
- **Build:** `Makefile.wiiu` adds `-fno-caller-saves`; see the next section.

## The "Loading - please wait" / startup-hang bug

**Symptoms:**
- models were corrupt straight after decode (`Draw failed … kind=3`), so the world never drew;
- startup sometimes hung while parsing sounds.dat (inside `calloc`).

**Ruled out, with evidence:**
- stack overflow: logged addresses show the client on the 16 MB stack, separate from the heap;
- a second client thread: thread and core IDs are identical everywhere;
- GPU or SDL writes: the hang reproduces with presenting disabled, before any frame;
- a use-after-free or double free: a build where `free()` does nothing still corrupts;
- arena OOM: none was reported;
- PC heap errors: an ASan PC run was clean.

**Decisive observations:**
- A `volatile` read right after each store, and after every later loop iteration, saw the correct value. The read right after the loop, and `memcpy`, saw stale memory: old title pixels and malloc free-list pointers.
- The same -O2 binary under Cemu's **interpreter** (game profile `cpuMode = 0`): no hang, no corruption.
- Under the **single-core recompiler**: still corrupt. So the trigger is Cemu's recompiler, not multi-core timing.
- `-O0`, and `-O1`, on the default recompiler: clean.

**Flag bisection of what -O2 adds over -O1:**

| Build | Result |
|---|---|
| `-O2 -fno-ipa-ra` | corrupt |
| -O2 with 17 backend flags off | clean |
| -O2 with schedule, peephole2, ree, caller-saves, lra-remat, store-merging and hoist-adjacent-loads off | clean |
| `-O2 -fno-schedule-insns -fno-schedule-insns2` | startup hang |
| `-O2 -fno-peephole2 -fno-ree -fno-caller-saves` | clean |
| `-O2 -fno-peephole2` | corrupt |
| `-O2 -fno-ree` | corrupt |
| **`-O2 -fno-caller-saves`** | **clean** (twice, including the final build with audio on) |

**Conclusion:** GCC 16's `-fcaller-saves` code shape triggers a Cemu 2.6 PPC recompiler fault that loses stores. The client code is correct, since the interpreter runs the same binary correctly. Real hardware has not been tested; the flag is harmless there.

## Verification

| Check | Result |
|---|---|
| `make -f Makefile.wiiu clean all` | 0 warnings; `rs2004c.wuhb` SHA256 `09cbf7a2…0dddb` |
| Cemu 2.6 final build (audio on, autologin) | logged in, map rebuilt, 0 draw failures (`cemu-final-log.txt`, `cemu-final-errors.txt`) |
| PC `make test` | ALL TESTS PASSED, 0 warning lines (`suite.log`) |
| PC `make` | rs2client.exe relinked, SHA256 `263d32ad…6327` (`build-pc.log`) |
| Strict parity, integrated (3099 frames) and tour (3849 frames), vs original Java | PARITY MATCH, 0 differing frames (`parity-integrated.log`, `parity-tour.log`) |

The rs2replay.exe used for parity has SHA256 `58cd045e…000a`. The earlier keyboard-only logs are in `../2026-10-27-wiiu-keyboard-claude/`.

## Cleanup

- All diagnostics are removed: canary, stage, texture-check, net and live logging, the noaudio and nopresent switches, and the extended Draw-failed message. model.c, model.h, wave.c and client_stream.c are byte-identical to HEAD.
- The bisection build folders, the ASan exe and the temporary Cemu game profile were deleted.
- The SD card still has the user-local `autologin.txt`, `server.txt` and the logs.

## Next

1. Hardware test on the user's Aroma Wii U (LAN server address in server.txt).
2. Software MIDI synth.
3. Map cache write on non-Windows.
4. sources.mk in build_release.py.
