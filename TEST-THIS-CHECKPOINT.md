# Test the current Windows MIDI/action checkpoint — October 21, 2026

Launch play_client.bat and reopen the client to load the new executable. It uses the existing local server on port 43595 and preserves account progress. Reuse the running server. Current native SHA-256: `8D430F47FC59403DB7513A7A1F6F84435CFD8A13BE692DA9EA4298790DB302EF`. The cumulative overlay is midi-actions-checkpoint.zip with a .sha256 sidecar; it requires this checkout's existing engine/cache/content and dependencies. No saves or .env files are packaged.

1. Walk on reachable ground: the original yellow cross animates at the click. Minimap clicks retain the original destination flag behavior.
2. Click a world operation such as Talk-to, Take or an item/spell target: the original red X animates at the click when the action is dispatched.
3. Select an inventory item's Use option, then click a valid item/world target. Cancel clears selection. Spell menus follow the original spell's allowed target types.
4. Listen to server-selected songs and level jingles. Songs now reject malformed metadata/data safely; jingles interrupt ambient music and use the original server delay before restarting it. Automated device commands succeeded, but listening quality and timing need manual assessment.

All 42 programs pass warning-free. Original-Java data/pixel/packet comparisons and live banker/firemaking evidence are recorded in client-c/verification/completion/runs/2026-10-21-midi-actions-sol/README.md. Combat effects, hitmarks/health bars, social/exchange flows and broader visual/gameplay parity remain incomplete. See HANDOFF.md for current details; preserve all existing accounts and old packages.
