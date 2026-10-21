# Lumbridge bridge fix — October 21, 2026

User-reported missing bridge deck/railings reproduced and fixed. New native SHA-256: `36294F584EC5D82701B1861B8BE656D821CAAEC6F3FBA049CBBD688C610BEA18` (client-c/bin/rs2client_bridge.exe). Launch play_client.bat normally. The previous rs2client.exe is still locked by the user's existing PID13088 session and remains1828C538...; play_client.ps1 promotes the newer bridge build when possible, or launches that build directly if the old file is locked. A subsequent newer normal build takes precedence by timestamp. Do not terminate the user's existing client or production server. Close the old client before logging the same account in again.

Cause: World3D.render iterated source levels only up to the player's logical level, omitting bridge decks and railings stored on level1. world_build now assigns original World.getDrawLevel tile visibility (flag8→0; level1 flag2 lowers positive levels by1); renderer visits source levels and gates terrain, items and scenery by each tile's draw_level. Heights/models remain on their original source plane. This is a bounded fix for draw-level visibility, not a full port of original setBridge tile relinking, bridge-underlay traversal, roof/occluder ordering or shared-normal scene lighting.

All50 programs pass with zero warnings (make -C client-c TARGET=bin/rs2client_bridge.exe all test). BridgeLevelOracle.java calls the bundled original World.getDrawLevel for262,144 source-level/flag combinations; the C test compares actual world_build results to every byte. Real Lumbridge cache shows the bridge base1619/3008/3009 on level0 and rails3007 on level1. Before/after native SDL captures at3251,3227 with pitch220/yaw512 show the missing span becoming a continuous deck with both railings. Original whole-scene screenshot parity is not claimed. The normal launcher was also exercised against the real server and completed cleanly using the fixed build while the old client remained open.

Preservation:29 saves recorded at test start;28 are byte-identical. antigravity.sav changed naturally while the user's pre-existing client remained active; no save was edited, restored or reset. New disposable solbridge211 is retained. No production-server/cache/content/environment changes. Existing server PID3128 was reused. Previous actor/scenery archives remain untouched; bridge-visibility-checkpoint.zip is the current cumulative source/runtime overlay with its SHA sidecar. Engine/cache/content/build dependencies are required. Sources, before snapshots, failed locked-link log, oracle fixture, before/after/launcher captures and manifests are in client-c/verification/completion/runs/2026-10-21-bridge-sol.

Continue AUTONOMOUS_PLAN.md with logout/login recovery and the remaining gates described below. No delegates; deferred Cursor/Grok audit must not be searched. Last usage check during this fix was92% used/8% remaining; reserve final5% for handoff. Earlier active-binary/usage fields below are historical. Full roadmap remains unfinished.

---

# Current Windows actor checkpoint — October 21, 2026

Launch play_client.bat from this folder. Reuse the existing server on43595. All saves are preserved. Native SHA-256: `1828C538E1D2F9EE3AE66483CE92CA6EB87245E30AB9D7E9F3E19DF775DC87F0`. Cumulative overlay: actor-animation-checkpoint.zip and its checksum sidecar; requires existing engine/cache/content/dependencies.

New: original player/NPC animation blending, equipment overrides, rest lighting, actor/effect composition, sequence clocks and target facing. Actual ranged/magic combat and death/respawn passed; final executable also passed live spell/rune/XP and death/respawn checks. All49 tests pass warning-free. Details and exact scope: client-c/verification/completion/runs/2026-10-21-actor-sol/README.md.

Full gameplay and scene parity remain unfinished. Logout still needs the original return-to-login transition. Preserve every account including solactor211 and soldeath211. No saves or credentials are packaged.
