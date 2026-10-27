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

**HOME menu and exit (confirmed on hardware).**
- In-game HOME froze the console right after `entered background` (`hw2-home-freeze-log.txt`). Probable cause: SDL creates its threads with any-core affinity, so the audio threads could keep running on core 2 in the background (wut procui.h: background apps keep only core-2 threads). Fix: on `SDL_APP_WILLENTERBACKGROUND`, `audio_suspend()` stops the wave worker and closes the device before the foreground is released; `SDL_APP_DIDENTERFOREGROUND` calls `audio_resume()`. Queued sounds are kept. Hardware: the HOME menu opens and returning works.
- HOME → Close hung on the Wii U Menu loading screen; the log ended at `quit requested` (`hw3-close-hang-log.txt`). The quit arrives while still in the background, and the rest of that frame (swkbd state query, update, draw) blocked. Fix: `SDL_QUIT` calls `client_stop()`, and the frame breaks out at once. Shutdown calls `SDL_StopTextInput` only if our keyboard is open.
- Hardware: closes cleanly, with every stage logged through `client thread joined` (`hw4-close-ok-log.txt`).
- Synced `[WIIU]` stage lines (fflush and fsync) stay in log.txt for future hardware diagnosis.

## Verification

- `make -f Makefile.wiiu all`: 0 warnings.
- PC `make`: 0 warnings.
- PC `make test`: ALL TESTS PASSED, 0 warning lines (`suite.log`). Re-run on the final code. Shared-code change: `audio_suspend`/`audio_resume` added to wave.c; only the Wii U calls them. Final `.wuhb` SHA256 `8bc0af98…58e0`, uploaded over FTPiiU and checked by reading it back.

## Next

1. Investigate the two in-game logouts during the second run (`hw2-home-freeze-log.txt`): user action, or a server timeout caused by the low frame rate?
2. Performance on Espresso: profile first. Candidates are frame pacing, using the other two cores, and `-O3`/LTO; check each against Cemu's recompiler fault.
3. Input: right-click and camera on touch (GamePad buttons). USB keyboard through SDL's Wii U keyboard driver. Controller later, as the user asked.
