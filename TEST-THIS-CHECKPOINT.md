# Test the current Windows client — October 21, 2026

## Launch

The current client is already installed in this checkout. Double-click play_client.bat in `2004scape C Rewrite`. The existing server was confirmed listening on43595/8889/8899 in PID3128 at handoff. If it is no longer running, use start_local_server.bat and wait for World ready. The launcher preserves account progress and uses the existing local test account automatically. For another account, see MANUAL-TESTING.md. Login code5 means already logged in or pending logout. Close the client window to exit.

The newest complete update is `handoff-checkpoint-2026-10-21.zip`, with a .sha256 sidecar. Extract its contents directly into the existing `2004scape C Rewrite` folder, allowing matching client/source/doc files to be replaced. Existing engine/content/cache are required. No saves or .env files are included. Older ZIPs are historical snapshots; never restore the withdrawn save-purge launcher from an old package.

## Visible changes to try

1. Close any bank/shop interface, then left-click nearby walkable ground on the minimap. The client should send a route and display its destination flag. Rotate with arrow keys and try another click. Movement remains driven by the real server.
2. Watch the destination flag clear on arrival. Ordinary NPC, scenery and ground-item approaches now set the same flag. Server-clear and region-rebase handling are implemented but still need live verification.
3. Other visible players now have ordinary minimap dots. Friend colouring is unfinished. NPC and ground dots use the original coordinate rounding.
4. Bank Deposit/Withdraw X, bank dragging/close/reopen, shop Buy/Sell, ground Drop/Take and stairs are useful existing checks. Available items, tutorial restrictions and scripts depend on the actual account/server state.

Minimap clicking is live-only. An open context menu, active inventory drag or main modal blocks it. Nonzero minimap zoom/angle drift and wider navigation parity remain unfinished. Escape closes an active menu/interface and otherwise selects Inventory; window close exits.

For a bug report, include account name (never password), location, clicked position/action, camera direction, expected/actual outcome and screenshot or console log. Preserve saves.

## What has actually been checked

The older world-loops executable passed37 programs and separate instrumented live ground, bank, shop, dialogue and stair scenarios. Eight subsequent small changes built with zero warnings, including the current minimap input implementation. Tests and live scenarios were not rerun for those changes. Audible MIDI, friend colouring, complete gameplay and original visual parity are not established.

Current build/evidence scope: client-c/verification/completion/runs/2026-10-21-minimap-click/README.md. HANDOFF.md identifies the latest source, hashes, gaps and continuation order. Cursor/Grok audit recovery is deferred; do not spend more tokens searching for its partial output.
