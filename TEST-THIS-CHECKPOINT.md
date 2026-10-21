# Current Windows combat/effects checkpoint — October 21, 2026

Reopen the client with play_client.bat to use the updated executable. It connects to the existing server on43595 and preserves progress. Reuse the running server. Current executable SHA-256: `93CDC5C40FDF7EC2A0B5C2C68447E7246153D6C00BAC4FEB90252028669A6511`. Cumulative overlay: combat-effects-checkpoint.zip with its .sha256 sidecar; requires existing engine/cache/content and dependencies. No saves or .env files are packaged.

New: original spell casting/flight/impact models, numbered hitmarks and green/red health bars, plus original stationary spot animations. Red X action feedback, yellow walking crosses, item/spell targeting and MIDI fixes from the previous checkpoint remain included.

All43 test programs pass warning-free. Actual Wind Strike against chickens produced server-confirmed rune consumption, XP, projectiles and damage. Exact scope, screenshots, failed probes and remaining gaps are in client-c/verification/completion/runs/2026-10-21-combat-sol/README.md. Full combat, scenery animation, social/exchange flows and broader visual parity remain incomplete. HANDOFF.md contains the continuation plan and preservation details.
