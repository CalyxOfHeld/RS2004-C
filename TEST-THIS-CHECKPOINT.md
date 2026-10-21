# Windows client checkpoint — October 21, 2026

**Latest update:** server song packets now use the original byte10 delimiter with checked metadata framing. Latest cumulative package: song-packet-client-update.zip. Build passed with zero warnings; tests/live audio checks were not run.

**Latest update:** operation approaches now update the shared minimap destination flag after a successful route send. Latest cumulative package: approach-flag-client-update.zip. Build passed with zero warnings; tests/live checks were not run.

**Latest update:** scene-click walking now sets the minimap destination flag; arrival/server clear removes it, and region rebases retain its world destination. Latest cumulative package: destination-flag-client-update.zip. Build passed with zero warnings; tests/live checks were not run for this change. Approach-route flags and minimap clicking remain open.

**Latest update:** ordinary visible-player minimap dots are implemented. Latest cumulative ZIP: player-dots-client-update.zip (includes MIDI/offset fixes). Build passed with zero warnings; tests and two-client visual checks were not run. Friend colouring remains open.

**Latest small update:** the installed executable also corrects minimap dot quantization to match revision225. Build passed with zero warnings; tests/live visual checks were not rerun. Latest update: minimap-offset-client-update.zip.

**Subsequent small update:** the installed executable now includes MIDI failure reporting. It built with zero warnings but has not had tests or audible/live checks rerun. The 37-program/live results below apply to the prior binary retained in world-loops-test-checkpoint.zip. See client-c/verification/completion/runs/2026-10-21-midi-status/README.md.

Feature work is paused for your testing. This is a working checkpoint; the full native client is still incomplete.

## Launch

The current executable is already installed in this checkout. Double-click `play_client.bat` in `2004scape C Rewrite`. The existing local server was listening on port 43595 when packaged. If it is no longer running, run `start_local_server.bat` and wait for World ready first.

The launcher automatically uses the existing local test account and keeps its progress. For another account, see MANUAL-TESTING.md. Close another client using that account before launching; login code 5 means already logged in or pending logout. Close the client window to exit.

The ZIP is an update for this existing checkout: extract its contents directly into `2004scape C Rewrite`, allowing replacement of matching client/source/document files. It requires this checkout's existing engine, content and cache. It contains no player saves or .env files. Do not use older checkpoint ZIPs: one historical package contained a withdrawn save-purge launcher.

## Suggested checks

1. Walk on the main scene; rotate the camera with arrow keys. Confirm inventory and equipment match the account.
2. Drop and take an item you can spare; check inventory quantities and the ground pile.
3. Talk to a banker and open the bank. Deposit/withdraw an item and coins using X, drag a bank slot, close and reopen. Confirm quantities and the normal inventory sidebar return.
4. Trade with a shopkeeper, buy and sell one ordinary item; check coins, inventory and stock.
5. Use available stairs up and down; check the scene and minimap change together. Try normal doors and dialogue choices.

Tutorial restrictions, available items and NPC dialogue depend on the real account and server state. No fixture commands are required for ordinary testing. Minimap click navigation, public chat and full combat/skilling coverage remain unfinished. Escape closes active menus/interfaces and otherwise selects the inventory tab; use window close to exit.

For a bug report, provide the account name (never password), location, action, expected result, actual result, and a screenshot or console log. Do not delete saves to reproduce a problem.

## Evidence

The packaged executable passed the recorded 37-program suite with zero compiler warnings, plus separate instrumented live SDL scenarios for ground quantity/reveal, bank X/drag, natural bank dialogue, shop buy/sell and stairs. These are bounded checks, not full gameplay certification. See the world-loops run README for exact scope and failed probes. Audio matches the original Java output for 435 sounds at two loop counts with a shared noise input; device timing and MIDI remain unverified.

See NEXT-STEPS-AND-GEMINI.md for the continuation plan and GEMINI-AUDIT-TASK.md for a ready-to-copy bounded assignment. The ZIP checksum is in world-loops-test-checkpoint.sha256.
