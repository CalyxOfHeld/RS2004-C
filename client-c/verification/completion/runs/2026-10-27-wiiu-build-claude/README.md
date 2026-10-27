# Wii U port step 2: build + first Cemu boot — Claude, October 27, 2026

User: "go ahead with the makefile". Cemu 2.6 (Windows and Ubuntu builds, plus source) was added by the user under `../../../../../wii u/`; the user has a modded Wii U for hardware testing.

## Changes
- `client-c/sources.mk` (new): client source lists moved verbatim from `Makefile`. `make` prints an identical ALL_SRCS/ALL_OBJS before and after.
- `client-c/Makefile.wiiu` (rewritten):
  - uses sources.mk plus client225.o, built with the same `-Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1` flags as the PC normal build
  - devkitPPC, wut and portlibs SDL2; output `bin/wiiu/rs2004c.{elf,rpx,wuhb}`
  - bundles `../engine/data/pack/client` (6.1 MB) as `/vol/content/engine/data/pack/client`
- `client-c/build_wiiu.bat`: wrapper.
- `platform/sdl2_main.c`, under `__WIIU__` only:
  - chdir to `fs:/vol/content`, so the client's existing `engine/data/pack/client/...` fallback paths resolve
  - read an optional `fs:/vol/external01/wiiu/apps/rs2004c/server.txt` ("host [port]")
  - `SDL_RenderSetLogicalSize(765,503)` letterbox
  - no `SDL_StartTextInput()` at startup, because on Wii U it opens the system keyboard immediately
- `client.c`/`client.h`: new `--host`/`server_host` option, default `127.0.0.1`, replacing the hard-coded login host. PC behaviour is unchanged.

## Evidence
- build-wiiu.log: exit 0, 0 warnings, rs2004c.wuhb 5.4 MB.
- Cemu 2.6 `-g rs2004c.wuhb`: boots and renders the revision-225 title screen at about 49.5 FPS, titlebar "RS2004-C".
  - The first boot showed the Wii U software keyboard over the title, caused by SDL_StartTextInput; it is now skipped on Wii U.
  - The second boot was not visually confirmed by Claude: the desktop capture showed another window on top of Cemu. The user was asked to check.
- PC after the changes:
  - rs2replay build exit 0
  - make test ALL TESTS PASSED, 0 warnings (suite.log)
  - integrated recording strict replay vs Java: PARITY MATCH (parity-integrated.log)

## Not done / next
- Text entry: open swkbd when a login/chat field takes input, and route its text to the client.
- Music: needs a software MIDI synth. The Windows-only synth leaves Wii U silent.
- Not yet tried on hardware or for networking/login.
- Release script: build_release.py copies `Makefile` but not the new `sources.mk`; add it when the next release is built.
