# Shared scenery lighting implemented — October 21, 2026

Native SHA-256: `68D4B6DE7F27ECD78A2AFF07D72B28A89CC11B2A0FA29AFEE7B53D4BAB65C94A`. `client-c/bin/rs2client_shared.exe` and the launcher's `rs2client_bridge.exe` contain this build. Launch `play_client.bat` normally; its existing promotion/fallback behavior selects the new bridge executable even while the old normal executable remains locked by the user's PID788 session. The previous bridge executable (36294F58...) is preserved under this run's `before/rs2client_bridge.exe` and in the unchanged bridge-visibility-checkpoint.zip. The production server PID3128 and the user's existing client were not stopped. Recheck process/port ownership before future startup work.

Implemented the bundled revision-225 static-scene `World3D.buildModels` behavior: immutable pre-skew normals and separate working sums, original wall/location/ground-decoration traversal, same/next-source-plane terrain offsets, multi-tile location references, both-way original-normal accumulation, three-matched-vertex hidden-face threshold, the wall-pair no-hide case, and scene-wide Gouraud lighting while retaining location-specific flat-face shades. Hidden faces are excluded from both drawing and mouse picking. Each scene model owns its normal/type copies; cache eviction and frame-arena copies remain isolated. Working data is retained for deterministic C rebuilds, with active-entry state reproducing Java's processed-model exclusion.

All **51 native test programs pass with zero warnings** (`final-suite.log`, exit0). SceneLightingOracle calls the bundled Java methods on real cache models: **18,304 model-pair cases** compare pre-skew original normals, merged sums, visible face shades and hidden-face state. A full real m50_50/l50_50 scene compares **1,904 shared-light models** across all four source planes, including all22 railing3007 models and bridge bodies1619/3008/3009, 30 corner-wall parts, ground decorations and multi-tile scenery. Three repeated builds plus a complete scene reload with cache eviction pass. Boundary checks cover two versus three shared vertices, both-way sums, vertical offsets, no-hide merging, flat shades, heap/arena copy ownership, and hidden-face drawing/picking.

Actual SDL/server captures cover3251,3227 (pitch220/yaw512) and3245,3225 (pitch128/yaw1024). The exact final build was exercised through the unchanged normal launcher and shut down with exit0. Isolated Java harness captures call the original static-scene renderer after a live login, with temporary actors removed, at the identical native base, position, plane and camera. The side comparison is `final-static.png` versus `java-side-static.png`: base3192,3176, player3245,3225, plane0, camera6848,-762,7245, pitch128/yaw1024. The supplied user's side view is closely reproduced. Geometry/shading component comparison passes; **whole-scene pixel parity is not claimed**. The images retain visible texture/brightness differences on bridge masonry and separate terrain/water differences. The earlier research's missing-normal pass is resolved, but it was not proven to be the sole cause of the reported appearance. Original setBridge relinking/underlay traversal, occluder ordering and texture/palette comparison remain open.

Scope: static map-build scenery only. Existing dynamic `world_change_location` individual-light fallback and original animated/temporary-location behavior are preserved; the full location-animation/merge suites pass. No new neighbor relighting after a dynamic replacement is claimed. No server/content/cache/rule/environment/launcher-source changes, save edits, commits, resets, delegates or external-audit recovery. Existing30 saves:29 byte-identical; only antigravity naturally changed during the user's pre-existing session. New disposable sollight211, solfront211 and solside211 are retained; developer tutorial/teleport commands prepared their views, so natural tutorial/travel is not claimed.

Evidence: `client-c/verification/completion/runs/2026-10-21-shared-lighting-sol/`. The first Java login attempt found the native test account still logged in; the failed response is retained in reference-login-busy.log. The first launcher capture completed natively, but its caller's relative-path bookkeeping failed after the launcher changed directories; launcher-initial.log preserves that run. A later complete launcher run recorded exit0 with absolute caller paths. These failures are not acceptance evidence.

New cumulative source/runtime/evidence overlay: `shared-lighting-checkpoint.zip` with SHA-256 sidecar; engine/cache/content and existing build dependencies are required. Previous archives remain immutable. Resume AUTONOMOUS_PLAN.md with logout/login recovery: native opcode142 still closes the session and leaves the world onscreen with a synthetic message instead of the original title/login transition. Inspect the original logout/title code before changing it. Full client completion remains unfinished; older handoff status and usage percentages below are historical. Keep the deferred Cursor/Grok output search deferred.

---

Current handoff is now written at the top of HANDOFF.md. Latest complete package: handoff-checkpoint-2026-10-21.zip. No further implementation is running. Earlier usage/status wording below describes progress before finalization.

# Immediate next implementation: shared scenery lighting

Updated October 21, 2026 after the user asked to continue and have Sol take over when the task is ready. The current implementation task is `client-c/verification/completion/SCENE-SHARED-LIGHTING-TASK.md`, based on the latest bridge follow-up research in `HANDOFF.md`. It supersedes the older resume order below for the immediate next work. Implement generic revision-225 `sharelight` vertex-normal merging, hidden-face handling, and deferred scene lighting; preserve the still-open minimap runtime verification and other roadmap gates for subsequent work. No implementation changes have been made yet.

---

# Current next move and external-agent work

Updated October 21, 2026. After the test checkpoint, the user authorized one small task at a time. Eight small changes are complete; no new implementation task is running during this documentation update. The server was left available for manual testing. Last account usage check:92% of the five-hour window used,14% weekly. Write the next full runnable handoff at95% used/5% remaining, as requested. Current progress and run notes supersede the older paused handoff where they differ.

## Resume order

1. Read the user's checkpoint feedback and reproduce/fix reported failures first. Preserve all accounts, history, cache and server behavior.
2. Verify the newly implemented live minimap clicks: hit area/inverse transform, camera rotation, collision routing and opcode165 with14-byte trailer. Current zero zoom/angle input and serialization are implemented; do not duplicate them. Scene-click destination rendering/arrival/packet19/rebase is now implemented, as are dot coordinate rounding and ordinary player dots. Approach routes now update the shared flag after a successful nonempty send. Friend colouring needs synchronized social state. All recent changes need scoped verification before claiming runtime parity; natural route, cancellation and region-crossing evidence remains open.
3. Close remaining item-use/spell and connected combat/skilling/social/effect gaps in dependency order from AUTONOMOUS_PLAN.md. Unsupported zone families 23/42/69/191 need source-backed coverage.
4. Continue original-render comparisons for roofs, bridges, occlusion and ground piles, then sustained session checks. Passing the current suite does not establish complete parity.

## Work suitable for Gemini

These are bounded assignments with outputs that can be checked against local source. The user reports that Cursor/Grok partially worked on the audit and then exhausted usage. Partial results have not yet been located or reviewed here; the expected gemini-audit directory is empty. Recover its files or last response before assigning duplicate work. The original Gemini-named brief remains the continuation entry point for compatibility. Model choice alone does not establish correctness.

| Assignment | Deliverable | Review boundary |
| --- | --- | --- |
| Protocol/interface coverage audit | CSV mapping original Java, server codecs and native handlers with exact file/line references | Read-only audit; mark unknowns explicitly |
| Checkpoint manual QA | Reproduction steps, screenshots/logs and observed quantities for bank/shop/doors/stairs | Separate disposable account; no save edits or server changes |
| Evidence/document audit | Scenario-to-evidence index, stale claims and proposed corrections | Distinguish actual server acceptance from fixture preparation |
| Cache/definition inventory | Missing item/NPC/animation coverage report from bundled real definitions | No invented assets or later-revision references |
| Small regression proposals | Boundary cases and independently derived expected values for a defined helper | Review before adding code; do not copy implementation as the oracle |

Searching for Cursor/Grok's partial output is deferred at the user's request. Revisit it only when the user supplies its location/output or requests recovery. No external audit results have been reviewed or accepted in this chat yet. Follow GEMINI-AUDIT-TASK.md when the audit resumes: review the current scene/approach destination implementations and focus remaining proposals on minimap click serialization. Keep one narrow assignment active at a time. Encryption/framing, entity rebasing, renderer ordering and audio arithmetic need close implementation review because errors can affect many systems. A later coding assignment should name one function, its reference behavior, allowed files and acceptance criteria.

## Checkpoint boundaries

Latest installed native SHA-256: `78346F76B3BE7C689B2C472F60E092FA647B64F31D92AD742943D3AEE406D28F`. Latest cumulative update: `minimap-click-client-update.zip`, with matching .sha256 sidecar. It includes MIDI failure reporting, dot rounding, ordinary player dots the scene-click flag operation-approach flag integration MIDI song packet framing defined signed packet reads and live minimap click implementation. All eight built with zero warnings; tests/live checks were not rerun for these updates.

The earlier world-loops-test-checkpoint.zip preserves SHA-256 `3035DF94DAFA31A55C038CB1F25DEC795118BD7513EBA85AA463B3ACF0077919`, the executable used for the accepted live scenarios. Its37-program suite and the later Makefile shared-object suite passed. Those results do not certify the newer binary. Read the latest task instructions from the workspace; copies in older ZIPs are historical snapshots.
Existing server PID 3128 owned ports 43595/8889/8899 at packaging. Verify ownership again before starting any server. New test saves stack211a, stack211b, bank211 and bank212 are persistent and must be preserved too.
No Git remote is configured; delivery is the local test ZIP and installed executable. No commit or remote push was made.
