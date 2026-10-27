# Wii U step 4: first real-hardware run — Claude, October 27, 2026

Scope: the user's own Wii U, test requested by the user. Not 244.

## Console setup (user-authorized)

The SD card had Tiramisu (`default.cfg` = `tiramisu`). Tiramisu cannot launch `.wuhb` files.

- The card's original `wiiu` folder was backed up to `Documents\WiiU-SD-backup-2026-10-27`: 972 files, 0 failures.
- Installed Aroma beta-27 from GitHub (SHA256 matched the published digest).
- Installed EnvironmentLoader 0.3.2, PayloadLoaderPayload 0.1.2 and CustomRPXLoader 0.1.3, using the paths from the official aroma.foryour.cafe manifest. The manifest hashes matched.
- Plugins: FTPiiU 0.4.5, Wiiload 0.2.5 and Bloopair 1.0.4.
- SigpatchesModuleWiiU 1.2. It is byte-identical to the Tiramisu copy and is needed for installed titles.
- `default.cfg` set to `aroma`. Tiramisu is left in place as a fallback.
- The game was installed at `wiiu/apps/rs2004c/` together with `server.txt` (`192.168.1.100 43595`).

## Results

1. **First run** (`hw1-log.txt`, `hw1-errors.txt`):
   - Loaded to the title screen.
   - Login failed with `Could not connect to 192.168.1.100:43595`. Cause: no inbound Windows Firewall rule for TCP 43595. The user added one.
   - The username keyboard opened, but the password keyboard never appeared. After that, no field tap would reopen any keyboard.
   - Closing from the HOME menu hung on the Wii U Menu loading screen. The log has no exit lines, and the client did not log quit at the time.
2. **Second run** (build `af82b77d…207b`, uploaded over FTPiiU and checked by reading it back):
   - The user logged in and joined the world.
   - Reported problems: very low frame rate, and touch alone is not enough to control the game.

## Changes (Wii U-only, `platform/sdl2_main.c`)

**Keyboard.** On hardware the swkbd fades in and out; Cemu has no fade.
- Probable cause of the stuck keyboard: the password-keyboard appear was issued during the username keyboard's fade-out and was dropped. `wiiu_kbd_shown` then stayed set forever.
- Fix, part 1: an open request now waits until `SDL_IsScreenKeyboardShown` is false.
- Fix, part 2: a keyboard of ours that has not been visible for 50 frames is treated as closed.
- Confirmed on hardware: the user logged in.

**Exit.**
- `SDL_QUIT` (ProcUI EXITING, from HOME → Close) now calls `client_stop()` directly.
- Exit-path stages are written with `fflush` and `fsync` to `log.txt` (`[WIIU] ...`). They cover quit, background/foreground, loop end, audio closed, client shut down, SDL_Quit and thread joined.
- The HOME → Close retest is pending.

## Verification

- `make -f Makefile.wiiu all`: 0 warnings.
- PC `make`: 0 warnings.
- PC `make test`: ALL TESTS PASSED, 0 warning lines (`suite.log`). The PC code paths are unchanged apart from a no-op `WIIU_STAGE` macro.

## Next

1. Read the logs after HOME → Close. If the exit still hangs, the last `[WIIU]` line names the stuck step.
2. Performance on Espresso: profile first. Candidates are frame pacing, using the other two cores, and `-O3`/LTO; check each against Cemu's recompiler fault.
3. Input: right-click and camera on touch (GamePad buttons). USB keyboard through SDL's Wii U keyboard driver. Controller later, as the user asked.
