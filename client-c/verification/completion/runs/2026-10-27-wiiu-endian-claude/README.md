# Wii U port step 1: byte-order / PowerPC portability audit — Claude, October 27, 2026

Scope: authorized by the user ("Wii U next, then Wii, then another revision"; "go" on step 1). No system installs were done, no Wii U runtime was exercised, and PC behaviour is unchanged.

## Wii U vs PC: what differs for this code

| Difference | Wii U (Espresso, devkitPPC) | PC build | Risk to this client |
|---|---|---|---|
| Byte order | big-endian | little-endian | Reading bytes as wider types, or wider types as bytes |
| plain `char` | unsigned | signed | Java bytes are signed |
| pointer / `size_t` / `long` | 32-bit | 64 / 64 / 32 | Size or overflow assumptions |
| C library | newlib, strict `-std=c99` | mingw-w64 | POSIX functions without declarations |
| Compiler | GCC 16.2 | llvm-mingw clang 23 | GCC-only warnings |

## Findings

**Byte order: no defects found.**
- Every cache, packet, map, model and sprite read goes through explicit byte-wise `Packet` accessors (`g1/g2/g4/gsmart/g1b`).
- No `uint8_t*` buffer is ever cast to a wider integer pointer, and no wide value is read as bytes.
- The type-punning `memcpy` calls in varp.c, sound_queue.c, input_tracking.c, wave.c and client.c all copy between same-size types, which is endian-neutral.
- Framebuffer pixels are native `uint32_t` with `SDL_PIXELFORMAT_RGB888`. SDL defines packed formats on the native word, so this is endian-neutral.
- Audio uses `AUDIO_F32SYS` (native float), which is endian-neutral.
- MIDI files are parsed byte-wise in midi_sequence225.h.
- `htons`/`htonl` are used correctly for sockets.

**`char` signedness: no defects found.**
- Signed-byte reads use explicit `int8_t` (g1b, effects.c, component_packets.c, the bzip2 interface).
- The `char` arithmetic sites are ASCII text only (jstring base37, wordpack, title input), or guard with `(unsigned char)` before `toupper`/`tolower`.

**Two real portability defects, now fixed (no behaviour change):**
1. `src/ui/chatbox.c` called `strdup` ten times. `strdup` is POSIX, not C99; newlib does not declare it under `-std=c99`, which is a hard compile error on Wii U. Added a file-local `chat_strdup` with identical semantics.
2. Five warnings that only GCC reports:
   - `jpeg.c` `(-1) << s`, rewritten as `-(1 << s)`, which is the same value for every JPEG category.
   - Four `-Wmisleading-indentation` one-liners in player_entity.c and scene_build225.c, split onto separate lines (whitespace only).

## Dynamic evidence (logs in this folder and the scratch copy)

| Check | Result |
|---|---|
| Full `make test` with `-funsigned-char` (x86_64) | ALL TESTS PASSED, 0 warnings |
| Full `make test` as 32-bit i686 with `-funsigned-char` (llvm-mingw + bundled i686 SDL2) | ALL TESTS PASSED, 0 warnings |
| All 80 client sources + client225 object compiled with devkitPPC `powerpc-eabi-gcc -mcpu=750 -meabi -mhard-float -std=c99 -Wall -Wextra`, wut + portlibs/wiiu headers | Before: 1 file failed and 5 warnings. After: 81/81 objects, 0 diagnostics |
| Normal PC `make test` after the fixes | ALL TESTS PASSED, 0 warnings (suite.log) |
| Strict replay of both October 27 recordings vs original Java, on the patched rs2replay.exe (BE9018A9…) | integrated 3099 and tour 3849 frames: PARITY MATCH |
| Patched rs2client.exe link | Blocked: the user's running client (PID 36400) locked bin/rs2client.exe (build.log). It was not stopped. The normal client still runs ff10df0c until the next `test_client.bat`, which links the patched code. |

The variant suites ran in a temporary sibling copy (`../client-c-portcheck`, since deleted), with junctions to lib, tests/fixtures, reference, runs and baseline, so the normal `bin/` and `build/` were never touched. Logs: variant-unsigned-char.log, variant-i686-unsigned-char.log, ppc-compile-after.txt (empty means 0 diagnostics).

**Not yet proven: big-endian runtime.** No PowerPC test execution was possible without installing qemu-user (WSL apt) or an emulator. The first real big-endian run will be the title screen in Cemu or on hardware. A targeted check then is a cache CRC/frame hash compared against PC.

## Remaining Wii U port work (not byte-order; for the next step)

Already installed on this machine: devkitPro with devkitPPC (GCC 16.2), wut, portlibs/wiiu SDL2, elf2rpl, wuhbtool and wiiload. libogc for Wii is also installed. `Makefile.wiiu` and `build_wiiu.bat` date from October 20 and list an old source set, so they must be regenerated from `Makefile`'s ALL_SRCS plus client225/client_log.

1. **Music:** midi_device225.h is Windows-only (winmm). Elsewhere it compiles to stubs, so music is silent. A Wii U path needs a software synth (for example TinySoundFont plus a SoundFont) feeding SDL audio. Sound effects (SDL F32) should work as-is.
2. **Data paths:** client.c/midi.c look for `../engine/data/pack/client/...`. The required client data is 6.1 MB, so bundle it in the .wuhb content (`/vol/content`) or on SD, and add one platform data-root prefix.
3. **Map download cache:** map_streamer.c writes atomically only on Windows. Elsewhere `map_cache_write` returns false. Downloaded regions are then not persisted; check the in-memory behaviour.
4. **Last-login DNS:** Windows-only worker. Elsewhere it is a no-op.
5. **Input-tracking wall clock:** off Windows, sdl2_main.c falls back to `SDL_GetTicks64()`. A real epoch clock is needed if the server compares the times.
6. **Display:** the 765×503 frame must be scaled to the TV (1280×720) and the GamePad (854×480), for example via SDL_RenderSetLogicalSize.
7. **Input:** GamePad touch as the mouse, plus the swkbd software keyboard for login and chat text.
8. **Network:** wut sockets and the server address. The engine must be reachable on the LAN, not only on localhost.
9. **Memory:** about 72 MB of static arenas, which fits Wii U easily. The Wii's 88 MB needs the arena redesign already noted in WII_AND_WIIU_PORT_FEASIBILITY.md.

Suggested next milestone: regenerate `Makefile.wiiu`, add the data-root prefix, link an .rpx/.wuhb, and reach the title screen in Cemu (or on hardware via wiiload). Cemu is not installed here; installing it needs the user's go-ahead.
