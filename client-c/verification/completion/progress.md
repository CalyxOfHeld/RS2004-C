# Logout returns to the original title screen; in-client login — October 21, 2026

Native SHA-256: `0510EADD01B31BC64212ABAA78D6A71B62FFE62A4095F6356ED4CA0BCFF597E4`. Installed `client-c/bin/rs2client.exe`; the launcher update `rs2client_bridge.exe` and `rs2client_decor.exe` match. Launch `play_client.bat` normally. The previous build `CD6A014A…7C4A` is preserved in this run's `before/`. New cumulative overlay: `logout-title-checkpoint.zip` with its SHA-256 sidecar.

Reproduced first with disposable `sollogb21`: the logout button sent IF_BUTTON 2458 and server opcode 142 closed the socket, but the world stayed rendered with a synthetic "You have logged out." line, and logging in again was impossible.

**Now ported from `deob/client.java`:**
- The title screen: the `title.dat` JPEG is decoded bit-exact to Java's libjpeg-6b path by the new `src/gfx/jpeg.c`. Also the nine title PixMaps with the mirrored background and logo, titlebox/titlebutton, `updateTitle`, `drawTitleScreen` states 0/2/3, and `updateFlames`/`updateFlameBuffer`/`drawFlames` (new `src/ui/title_screen.c`).
- `login()` with all reply messages and its success reset, then `prepareGameScreen`.
- `logout()`: socket close, title state 0, credentials cleared, model caches, scene and collision reset, MIDI stop.
- Opcode 142 and malformed packets call `logout()`. I/O loss calls `tryReconnect`: the "Connection lost" overlay, a reconnect login with opcode 18 and reply 15, or logout when `idleTimeout > 0`.
- Clientcode 205 sets `idleTimeout = 250`.
- Reply 1 is retried after 2 s.

The launchers' `RS2_USERNAME`/`RS2_PASSWORD` now prefill the original login box and press Login once. The raw executable without them shows the title. `--offline` is unchanged. The synthetic connect/logout/connection-lost chat lines are removed.

All **53 test programs pass with zero warnings**. The new `test_title_screen` replays `TitleOracle.java`, which runs the unchanged client `loadTitle`/`drawTitleScreen`/`drawFlames` on the real archive. **1,182,628 pixels match exactly** across the background/logo, five title states and seven flame-gradient branches on fixed buffers. The test also checks the `updateTitle` input rules and the login opcode 16/18, reply 15 and reply-1 retry handshake against a loopback fake server.

Live on PID 2352 with disposable `sollogc21`: login, button logout to the title, typed re-login from the title, second logout. Captures are in `after/frame-*.png`. The normal launcher with `sollogd21` exits 0.

**Not claimed:**
- Flame frames cannot equal Java's frames, because `Math.random` is unseeded.
- A live connection loss was not induced, so reconnect is tested against the fake server only.
- `idleNetCycles` reconnect, the AFK idle timer, anticheat camera randoms and `drawProgress` remain unported.

Saves: 34 of 35 are byte-identical. `antigravity.sav` changed at 11:40:37 although no run used that account, as in earlier user sessions; it was not modified or restored. The disposable `sollogb21`/`c21`/`d21` saves are retained. No server, content, cache or launcher-source edits, commits or delegates.

Evidence and diff: `client-c/verification/completion/runs/2026-10-21-logout-title/`. Next: resume `AUTONOMOUS_PLAN.md` with the remaining gates: roofs/occluders, texture/palette parity, setBridge traversal, dynamic relighting, `idleNetCycles`/AFK timers, and the gameplay audit. The full roadmap remains unfinished.

---

# Lumbridge corner wall decorations corrected — October 21, 2026

Native SHA-256: `CD6A014A97D66C15AA6F09815D49B854ED224479B8680426A98082D1529D7C4A`. Installed `client-c/bin/rs2client.exe`, launcher update `rs2client_bridge.exe`, and `rs2client_decor.exe` match. Launch `play_client.bat` normally. The previous shared-lighting bridge binary is preserved under this run's before/ directory and in the unchanged shared-lighting-checkpoint.zip. New cumulative overlay: wall-decoration-checkpoint.zip and its SHA-256 sidecar. Existing engine/cache/content/build dependencies are required.

The reported floating/misrotated arrow slits are real cache loc1938 (castlearrowslit), shape8 on the diagonal Lumbridge castle walls (for example3223,3232 and3225,3230). Native static placement treated shapes4..8 as ordinary rotated flat decorations. Original World.addLoc builds their model in canonical rotation0, then World3D.drawTile supplies the proper render pose. Corner shapes6/7/8 require diagonal yaw and inset/outset offsets; shape8 can draw both faces. The new shared world3d_location_draws helper ports the original front/back camera tests, diagonal yaw, 53-unit inset and45-unit outset offsets. Rendering, picking and the pointer-only verification selector use the same poses. Flat decorations retain their original quarter-turn orientation; shape5 now also follows original wall-width offsets and static wall-insertion rescaling. Canonical geometry is skewed before draw rotation, matching the original source. Scenery animation replaces the canonical model while retaining the placement metadata.

All **52 test programs pass with zero warnings** (suite.log, exit0). WallDecorationOracle calls original World.addLoc and World3D.draw on real arrow-slit model1938 and real nondefault-width wall830: **120 placement cases, 3,000 camera positions and3,336 actual original decoration draw calls** match native positions/yaws and canonical/skewed vertices across shapes4..8, all four rotations, flat/sloped ground and wall-before/decor-before insertion orders. Its isolated Model.java observation hook records arguments at Model.draw entry and skips rasterization solely for the pose oracle; it does not duplicate World3D's branch algorithm. prepare_oracle.py reproduces the hook from the unchanged bundled source. The separate live Java screenshot uses the ordinary original renderer without this hook. The complete animation/temporary-location and shared-lighting suites also pass.

Live before/after SDL captures at3227,3232, plane0, pitch220/yaw256 reproduce the reported wall and show the arrow slits fitted to its diagonal faces. Java and native static captures use identical base3176,3184 and camera7288,-1019,5513. Compare before-static.png, after-static.png, and java-static.png. The installed build was exercised through the normal launcher and exited0. Whole-scene pixel parity is not claimed: pre-existing terrain, texture/rasterization and roof/occluder differences remain separate gates.

All 33 account saves present at task start are byte-identical. New disposable soldecora21 and soldecorb21 are retained; developer tutorial/teleport commands prepared their views, so natural tutorial/travel is not claimed. No account, cache, content, server-rule or launcher-source edits, resets, commits, delegates or external-audit searches. No user client was running at preflight. All three server ports were initially free; started the existing server configuration, now PID2352 on43595/8889/8899, and left it available. Its startup took several seconds: an early native login failed before readiness; a second startup attempt conflicted with the now-started server and exited without replacing it. Those failures are retained as before-login-not-ready.log and server-session.log, and are not acceptance evidence. Later successful captures used only PID2352. Recheck ownership before any later startup; prior handoff PID3128 is historical.

Evidence and reproducible source diff: client-c/verification/completion/runs/2026-10-21-wall-decor-sol/. The supplied screenshot is preserved as user-report.png. Resume the broader AUTONOMOUS_PLAN.md with logout/login recovery and remaining presentation/gameplay gates. The full client roadmap remains unfinished. Earlier status and usage values below are historical; keep the deferred Cursor/Grok audit search deferred.

---

# Shared scenery lighting implemented — October 21, 2026

Native SHA-256: `68D4B6DE7F27ECD78A2AFF07D72B28A89CC11B2A0FA29AFEE7B53D4BAB65C94A`. `client-c/bin/rs2client_shared.exe` and the launcher's `rs2client_bridge.exe` contain this build. Launch `play_client.bat` normally; its existing promotion/fallback behavior selects the new bridge executable even while the old normal executable remains locked by the user's PID788 session. The previous bridge executable (36294F58...) is preserved under this run's `before/rs2client_bridge.exe` and in the unchanged bridge-visibility-checkpoint.zip. The production server PID3128 and the user's existing client were not stopped. Recheck process/port ownership before future startup work.

Implemented the bundled revision-225 static-scene `World3D.buildModels` behavior: immutable pre-skew normals and separate working sums, original wall/location/ground-decoration traversal, same/next-source-plane terrain offsets, multi-tile location references, both-way original-normal accumulation, three-matched-vertex hidden-face threshold, the wall-pair no-hide case, and scene-wide Gouraud lighting while retaining location-specific flat-face shades. Hidden faces are excluded from both drawing and mouse picking. Each scene model owns its normal/type copies; cache eviction and frame-arena copies remain isolated. Working data is retained for deterministic C rebuilds, with active-entry state reproducing Java's processed-model exclusion.

All **51 native test programs pass with zero warnings** (`final-suite.log`, exit0). SceneLightingOracle calls the bundled Java methods on real cache models: **18,304 model-pair cases** compare pre-skew original normals, merged sums, visible face shades and hidden-face state. A full real m50_50/l50_50 scene compares **1,904 shared-light models** across all four source planes, including all22 railing3007 models and bridge bodies1619/3008/3009, 30 corner-wall parts, ground decorations and multi-tile scenery. Three repeated builds plus a complete scene reload with cache eviction pass. Boundary checks cover two versus three shared vertices, both-way sums, vertical offsets, no-hide merging, flat shades, heap/arena copy ownership, and hidden-face drawing/picking.

Actual SDL/server captures cover3251,3227 (pitch220/yaw512) and3245,3225 (pitch128/yaw1024). The exact final build was exercised through the unchanged normal launcher and shut down with exit0. Isolated Java harness captures call the original static-scene renderer after a live login, with temporary actors removed, at the identical native base, position, plane and camera. The side comparison is `final-static.png` versus `java-side-static.png`: base3192,3176, player3245,3225, plane0, camera6848,-762,7245, pitch128/yaw1024. The supplied user's side view is closely reproduced. Geometry/shading component comparison passes; **whole-scene pixel parity is not claimed**. The images retain visible texture/brightness differences on bridge masonry and separate terrain/water differences. The earlier research's missing-normal pass is resolved, but it was not proven to be the sole cause of the reported appearance. Original setBridge relinking/underlay traversal, occluder ordering and texture/palette comparison remain open.

Scope: static map-build scenery only. Existing dynamic `world_change_location` individual-light fallback and original animated/temporary-location behavior are preserved; the full location-animation/merge suites pass. No new neighbor relighting after a dynamic replacement is claimed. No server/content/cache/rule/environment/launcher-source changes, save edits, commits, resets, delegates or external-audit recovery. Existing30 saves:29 byte-identical; only antigravity naturally changed during the user's pre-existing session. New disposable sollight211, solfront211 and solside211 are retained; developer tutorial/teleport commands prepared their views, so natural tutorial/travel is not claimed.

Evidence: `client-c/verification/completion/runs/2026-10-21-shared-lighting-sol/`. The first Java login attempt found the native test account still logged in; the failed response is retained in reference-login-busy.log. The first launcher capture completed natively, but its caller's relative-path bookkeeping failed after the launcher changed directories; launcher-initial.log preserves that run. A later complete launcher run recorded exit0 with absolute caller paths. These failures are not acceptance evidence.

New cumulative source/runtime/evidence overlay: `shared-lighting-checkpoint.zip` with SHA-256 sidecar; engine/cache/content and existing build dependencies are required. Previous archives remain immutable. Resume AUTONOMOUS_PLAN.md with logout/login recovery: native opcode142 still closes the session and leaves the world onscreen with a synthetic message instead of the original title/login transition. Inspect the original logout/title code before changing it. Full client completion remains unfinished; older handoff status and usage percentages below are historical. Keep the deferred Cursor/Grok output search deferred.

---

# Next implementation ready: shared scene lighting

The user asked to continue and have Sol take over once the work is ready to implement. Prepared `SCENE-SHARED-LIGHTING-TASK.md` with the revision-225 algorithm, native integration points, lifecycle constraints, and acceptance checks. The research probe confirms a concrete mismatch for adjacent cache railing models in both tested orientations. No production code or runtime state changed. Next action: implement the generic `sharelight` normal merge and deferred lighting in line with that brief, then compare against the Java oracle before packaging.

---

# Bridge follow-up research — October 21, 2026

Latest user instruction permits further research and a fix only if quick, with enough usage reserved to finish this handoff. This supersedes the earlier blanket “DO NOT attempt a fix yet” restriction below. Research completed; no renderer fix attempted because the supported lead requires a general scene-lighting/hidden-face pass and verification, not a small bridge-only adjustment. Current native and launcher are unchanged from the bridge visibility checkpoint.

Concrete original-client evidence: BridgeLightingProbe.java loads the real cache loc3007 (lum_bridge_edge), constructs two adjacent level1 wall pieces with original World3D.addWall, invokes original World3D.buildModels, and compares against the same real model lit individually with the original light vector and contrast scale. Both actual bridge orientations, rotation1 and rotation3, have12 vertices/10 faces per piece. For the first piece in each case, original scene processing changes shading on8 of10 faces and hides2 end faces. Successful output is retained in lighting-probe.log, exit0, with build log and complete Java source at client-c/verification/completion/reference/src/BridgeLightingProbe.java. Run from rewrite root with javaclient/build/libs/rs2client.jar FIRST on the classpath. This is an intentionally flat, two-piece diagnostic, not a complete live Lumbridge scene comparison.

Source findings: content/scripts/_unpack/all.loc marks the main bridge pieces1619/3008/3009 and railing3007 sharelight=yes; railing3007 additionally has hillskew=yes. Original World.finishBuild invokes World3D.buildModels BEFORE setBridge tile relinking. World3D.buildModels calls mergeLocNormals, then applyLighting; mergeNormals accumulates shared-vertex normals in both models and sets fully matched internal faces to-1 when at least3 vertices coincide. mergeLocNormals checks the current and next source levels with terrain-relative vertical offsets, so rail/body and adjacent-piece interactions require the real scene neighborhood. Native world.c:place_location explicitly falls back to individual model_calculate_normals for sharelight models, and native model_calculate_normals does not retain the original normal accumulators for a later cross-model merge. The original scene operation therefore cannot be restored by simply tweaking a light constant or flipping one railing.

Assessment: missing scene-normal merging/internal-face suppression is now a demonstrated component-level discrepancy and a strong explanation for segment shading/seams. It is NOT yet a proven sole cause of the user's screenshot; the full same-camera original/native comparison, actual terrain heights, rail/body junction and depth/draw-order behavior remain unverified. Inspection found no demonstrated quick placement/orientation correction. Keep the visible-span fix; do not claim bridge presentation parity.

Recommended next work when budget permits: reproduce the supplied user view in both clients; retain pre-light normals through native model assembly; port original World3D.buildModels/mergeLocNormals/mergeNormals and hidden-face handling with proper ownership and cache isolation; compare the complete bridge's final face shades/types against original output before live captures and regression checks. Verify both orientations, real terrain skew, cross-plane neighbors and nearby nonbridge sharelight scenery. Do not hardcode bridge IDs, darken faces, add depth bias or suppress geometry to imitate the screenshot. Original setBridge traversal and shared-normal lighting remain distinct incomplete gates.

No gameplay/rendering/launcher code, executable, server, cache or saves were changed in this research pass. Only the Java diagnostic and documentation/evidence were added. The prior bridge ZIP/checksum and its snapshot manifest remain immutable and predate these research files. Both handoffs plus progress/capability notes have this update; older contradictory restriction and usage numbers are historical. Preserve all accounts and recheck current server/client ownership before future work. No delegates or external audit searches were used.

---

# User bridge follow-up — assessment only, October 21, 2026

The user confirms that the missing bridge now appears, but reports that the restored side may render incorrectly. Explicit instruction: assess and add to the handoff; DO NOT attempt a fix yet. This restriction supersedes autonomous continuation for this particular rendering issue until the user authorizes work on it.

Screenshot preserved at client-c/verification/completion/runs/2026-10-21-bridge-sol/user-bridge-followup.png. Visual assessment: the near parapet has a conspicuous horizontal boundary/different stone shading where the restored railing meets the darker arched bridge body; the far railing also has visible segment transitions. These are observations from the supplied view, not a confirmed geometry, orientation or lighting defect. The screenshot alone cannot establish the intended original appearance. The missing-span fix remains confirmed; correct presentation of the restored side is OPEN and must not be called visually complete.

When authorized, reproduce this camera/position and compare the bundled revision225 Java client against the same cache, location and view. Inspect the separately stored level1 railings (loc3007) against level0 bridge-body locs1619/3008/3009. Potential investigation areas, not diagnosed causes: cross-model shared-normal lighting, bridge tile linking/draw order, railing placement/orientation and overlapping coplanar geometry. Original scene lighting and setBridge traversal remain incompletely ported. Do not change lighting, geometry or depth rules based only on this image.

No renderer/source/build/launcher/save/server changes were made for this follow-up. Existing executable hashes and bridge-visibility-checkpoint.zip remain unchanged. That ZIP and its manifest are the earlier immutable checkpoint; the current handoff and this new screenshot/note are later documentation, outside that archive. Existing PID/hash statements below describe the earlier test session and must be rechecked before acting.

---

# Lumbridge bridge fix — October 21, 2026

User-reported missing bridge deck/railings reproduced and fixed. New native SHA-256: `36294F584EC5D82701B1861B8BE656D821CAAEC6F3FBA049CBBD688C610BEA18` (client-c/bin/rs2client_bridge.exe). Launch play_client.bat normally. The previous rs2client.exe is still locked by the user's existing PID13088 session and remains1828C538...; play_client.ps1 promotes the newer bridge build when possible, or launches that build directly if the old file is locked. A subsequent newer normal build takes precedence by timestamp. Do not terminate the user's existing client or production server. Close the old client before logging the same account in again.

Cause: World3D.render iterated source levels only up to the player's logical level, omitting bridge decks and railings stored on level1. world_build now assigns original World.getDrawLevel tile visibility (flag8→0; level1 flag2 lowers positive levels by1); renderer visits source levels and gates terrain, items and scenery by each tile's draw_level. Heights/models remain on their original source plane. This is a bounded fix for draw-level visibility, not a full port of original setBridge tile relinking, bridge-underlay traversal, roof/occluder ordering or shared-normal scene lighting.

All50 programs pass with zero warnings (make -C client-c TARGET=bin/rs2client_bridge.exe all test). BridgeLevelOracle.java calls the bundled original World.getDrawLevel for262,144 source-level/flag combinations; the C test compares actual world_build results to every byte. Real Lumbridge cache shows the bridge base1619/3008/3009 on level0 and rails3007 on level1. Before/after native SDL captures at3251,3227 with pitch220/yaw512 show the missing span becoming a continuous deck with both railings. Original whole-scene screenshot parity is not claimed. The normal launcher was also exercised against the real server and completed cleanly using the fixed build while the old client remained open.

Preservation:29 saves recorded at test start;28 are byte-identical. antigravity.sav changed naturally while the user's pre-existing client remained active; no save was edited, restored or reset. New disposable solbridge211 is retained. No production-server/cache/content/environment changes. Existing server PID3128 was reused. Previous actor/scenery archives remain untouched; bridge-visibility-checkpoint.zip is the current cumulative source/runtime overlay with its SHA sidecar. Engine/cache/content/build dependencies are required. Sources, before snapshots, failed locked-link log, oracle fixture, before/after/launcher captures and manifests are in client-c/verification/completion/runs/2026-10-21-bridge-sol.

Continue AUTONOMOUS_PLAN.md with logout/login recovery and the remaining gates described below. No delegates; deferred Cursor/Grok audit must not be searched. Last usage check during this fix was92% used/8% remaining; reserve final5% for handoff. Earlier active-binary/usage fields below are historical. Full roadmap remains unfinished.

---

# Sol actor-animation checkpoint — October 21, 2026

Current runnable native SHA-256: `1828C538E1D2F9EE3AE66483CE92CA6EB87245E30AB9D7E9F3E19DF775DC87F0`. Launch play_client.bat against the existing server on43595. Cumulative source/runtime/evidence overlay: actor-animation-checkpoint.zip and actor-animation-checkpoint.sha256. Existing engine, cache, content and build dependencies are required. Previous scenery/combat archives and executable snapshots remain preserved.

This supersedes conflicting older status below. Continue AUTONOMOUS_PLAN.md without routine permission pauses. The full gameplay/visual-parity objective remains unfinished. No delegates or external audit recovery were used; Cursor/Grok audit recovery stays deferred. Preserve all account saves, cache and histories. Reserve the final5% usage for a runnable handoff; older usage-window percentages are historical. The last usage reading during this checkpoint was85% used/15% remaining, not a current pause boundary.

## Implemented and verified

- Original NPC/player rest-pose lighting, primary/secondary animation blending, held-item substitutions, appearance hash, NPC scaling and original30/200-entry model caches. Shared alpha and label ownership survives cache eviction and frame-arena resets. Actor spot effects use original lit vertex concatenation. Generic textured composite face types now match revision225.
- Original sequence clocks, secondary wrap timing, placeholders, replay/delay/stretch behavior, and production NPC/player/coordinate facing, including the server-local-PID alias and turn animation fallback. The SDL verification NPC selector now chooses the nearest received instance; it only resolves pointer input.
- All49 programs pass warning-free (final-suite.log). Java oracles call the bundled revision225 implementations: all1,017 NPC definitions/3,051 body meshes;4,676 player appearance/sequence meshes;836 complete actor/effect composites;6,256,848 clock states across all1,027 sequences;6,936 facing cases. First/last-frame and rig-compatible blending coverage is bounded; this is not a claim that every animation tick mesh or whole scene matches. Flat faces' unused B/C shades and unused raw nontextured face colors are excluded; rendered values are checked.
- Actual SDL/server ranged combat equipped a bow/arrows, played426, homed projectiles, consumed five arrows, updated Ranged XP and killed a chicken. Magic equipped dagger/shield, played711, consumed runes and updated Magic XP; subsequent melee used386/388. These D86C... combat runs precede only the facing integration. The exact installed1828... build consumed another pair of runes98→97 and advanced Magic XP130→180 with projectile91. Trace XP values are the client's xp_times10 field.
- The exact installed build's natural warrior combat reached player health0/10, received “Oh dear you are dead!”, rebuilt the Lumbridge region and resumed idle at3220,3219. The first death probe received “I can't reach that”; it is retained as a failure, not acceptance. Death-item loss, skull/protect-item variations and every combat style remain untested.

## Preservation and next work

All27 saves present at actor-run start are byte-identical, including solscene211/solpipe211. New disposable solactor211 and soldeath211 are retained. Fixtures used existing developer tutorial/teleport/item commands on those new accounts; natural tutorial/travel/item acquisition is not claimed. No direct save/cache/server-rule/environment edits, commits, resets or production-server restarts. Preflight confirms the existing server PID3128 owns43595/8889/8899; recheck ownership before any future startup. No account save or credential file is packaged.

Evidence: client-c/verification/completion/runs/2026-10-21-actor-sol/README.md, before snapshots, source-review.diff, binary pins, Java sources, raw-array fixtures, logs, lossless PNG captures, pixel conversion checks and manifests. The actor fixture bodies use original decoded appearance; earlier merge fixture bodies were deliberately shared fixtures. Do not conflate those scopes. Setup binaryF7A7..., initial combat binaryD86C..., and final installed1828... are separately pinned.

Next: finish logout/login recovery and disconnect/reconnect behavior. Native opcode142 currently closes the socket but leaves the world onscreen and adds a synthetic logout message instead of returning to the original title/login state. Inspect original deob/client.java logout and title/login before changing it. Preserve this runnable checkpoint while implementing that state transition. Remaining roadmap gates include prayer/style/settings and broader combat/item/death cases, multiplayer/social/trade, target approach/retry/cancel, sustained travel/reconnect and original whole-scene roof/bridge/occlusion/shared-normal lighting. Static sharelight scenery still uses individual lighting. Native zero/reversed forced-movement duration snaps safely, and RNG identity remains unverified. Do not claim full roadmap completion.

---

# Sol scenery/merge checkpoint — October 21, 2026

Active continuation of AUTONOMOUS_PLAN.md. The user asked Sol to keep working. This is a runnable checkpoint; the full roadmap remains unfinished. Continue the next source-backed task without routine permission pauses and prepare the next handoff at 5% remaining usage. Last usage check was 48% of the five-hour window used and 22% weekly. No agents or external audit recovery were used.

Installed native SHA-256: `2924ABF31116CF69BF162BB061C3BB91937B79528C90755DE7643EEFB69B0971`. Launch play_client.bat. Current cumulative overlay: scenery-merge-checkpoint.zip and its .sha256 sidecar; requires existing engine/cache/content and build dependencies. The combat/effects archive and executable snapshots remain preserved.

## Completed and checked

- Original zone42 scenery animation plus automatic animations from all 164 real definitions, including looping, two-part walls, decoration orientation, slope deformation, removal/rebuild cleanup and original 500/30 model caches with reference-counted shared alpha.
- Original zone23 local/remote player/scenery merge, signed bounds/offsets, timed scenery removal/restoration/collision and original lit model concatenation.
- Original player forced movement and player/NPC primary-sequence restart/priority handling. Local player rendering now selects the authoritative primary transform and original player-lighting constants.
- All 46 programs pass with zero warnings in shipping-suite.log/exit. Original Java calls match 1,528 scenery meshes, 20 retained-alpha/cache transitions, all 1,027 sequence lifecycles, 92 zone42 scene replacements, 92 zone23 cases/644 composites using the same original body fixture, 1,392 forced-movement tick states and 7,196 sequence decisions. Flat faces' unused B/C shades are excluded explicitly. This is bounded component parity, not complete native-body or scene parity.
- Actual SDL/server input completed wool-to-ball crafting with item/XP replies and all 12 wheel frames; a real Gnome Squeeze-through completed three forced-movement stages, two merge removal/restoration windows and Agility XP. The exact installed build relogged the crafted item, then processed the remaining wool with authoritative inventory and XP replies. The run README pins intermediate binaries and scopes. Preparation used existing developer commands only on new disposable accounts; natural tutorial completion/travel/acquisition is not claimed.

## Preservation and next work

All 25 starting saves remain byte-identical. Preserve new solscene211 and solpipe211 too, plus all earlier accounts. Production server PID3128 was confirmed on43595/8889/8899 and reused; leave it running and verify current ownership before starting anything. No direct save/cache/server-rule/environment edits, commits or history changes. Sources, before/scope snapshots, fixtures/oracles, failed probes, PNG captures, source diff and manifests are in client-c/verification/completion/runs/2026-10-21-scenery-sol.

All ten original zone-event families have configured live-client behavior now. Native safety policy snaps zero/reversed forced-movement duration instead of dividing by zero. Native RNG identity, whole-scene roof/bridge/occlusion and cross-location shared-normal lighting remain unverified; static sharelight scenery retains the prior individual-lighting fallback. Player/NPC body blending, held-item replacements and rest-pose lighting need original comparisons next. Broader combat styles/prayer/ranged/death/respawn, multiplayer/social/trade, logout/settings, approach/retry/cancel and sustained travel/reconnect gates remain open. Read the run README for exact evidence boundaries. Cursor/Grok audit recovery remains deferred; do not search or delegate.

---

# Sol combat/effects handoff — October 21, 2026

This supersedes older status below. The user authorized continuation and requested a handoff at 5% remaining usage. Finish this handoff and stop feature work at that boundary. Current implementation is complete for this bounded checkpoint; the full AUTONOMOUS_PLAN.md remains unfinished. No agents or external audit recovery were used.

Installed native SHA-256: `93CDC5C40FDF7EC2A0B5C2C68447E7246153D6C00BAC4FEB90252028669A6511`. Launch play_client.bat. Current cumulative overlay: combat-effects-checkpoint.zip and its .sha256 sidecar. Existing engine/cache/content and build dependencies are required. Earlier packages and the starting MIDI/action executable remain preserved.

## Completed since the MIDI/action checkpoint

- Ported all 244 original spot-animation definitions from the real cache, including recolour, scale, orientation, lighting and transparency. Added owned base-model caching with per-frame copies and cleanup.
- Implemented original zone 69 projectiles: packet fields, source/destination height, delayed start, arc/pitch, acceleration, NPC/player homing, animated flight, expiry and plane/region clearing. Zone 191 stationary effects and actor-attached casting/impact effects now animate and render too.
- Fixed actor hit timers and absolute spot-effect start timing in the real NPC/player update masks. Added original cache hitmark sprites, damage text and green/red health bars, with original 70-tick hitmark / 300-tick health-bar display windows. World transparency flush now precedes entity overlays.
- Implemented the missing original model animation transform type 5 (face alpha). Original animated-model comparisons exposed this omission.
- Optional replay NPC picking now chooses a mesh point near the projected centre rather than an unstable edge. These helpers only inject SDL events; all positions, inventory, hits and outcomes still come from the server.

## Evidence

All 43 test programs pass with zero compiler warnings in shipping-suite.log/exit. The standalone live-login test still explicitly skips without opt-in; separate actual SDL checks did run. EffectsOracle.java calls the bundled original implementations: all 244 definitions, 732 animated/pitched/oriented effect meshes (vertices, relevant lit face values and alpha), eight moving-target projectile trajectories, full stationary/actor frame lifecycles for all 209 animated definitions, and 18 complete original damage/health overlay framebuffers match. Unused B/C shades on flat faces are excluded explicitly. Tests also cover production hit masks, enclosed 69/191 framing, malformed rejection before mutation, delayed starts, expiry and plane clearing. This is bounded component parity, not complete scene parity.

The exact shipping executable completed shipping-cast on new disposable solfight211 against the unchanged production server: real Wind Strike selection/click, red X, rune consumption/Magic XP, actual id91 flight draws with changing positions/frames, NPC damage including 2 with health1/3, local-player zero hitmarks/health10/10 and subsequent NPC death. verified-cast and final-cast are additional intermediate-build evidence; their hashes and visible captures are documented in the run README. Actual captures show flight, casting, red damage text and health bars. Preparation used existing developer commands to set tutorial completion, location and runes; natural tutorial completion/acquisition is not claimed.

Native bounds are explicit safety policies: at most 4096 queued scene effects, and extreme/non-finite flight coordinates are culled before C float-to-int conversion. Zero-length/expired malformed trajectories are discarded. These are not claimed as original protocol limits. Whole-scene occlusion/roof/bridge parity, sustained performance and two-player live homing remain unverified.

## Preservation and next work

All 24 saves present at this run's start are byte-identical; preserve new solfight211 too. Earlier antigravity.sav change belongs to the prior run and was retained as found. Production server PID3128 on43595/8889/8899 was reused and left running. No save/cache/server-rule/environment edits, commits or history changes occurred. Source snapshots, fixtures, oracle, logs, retained failed probes, lossless captures, source-review.diff and manifest are under client-c/verification/completion/runs/2026-10-21-combat-sol.

Continue AUTONOMOUS_PLAN.md with original scenery animation (zone42), temporary player/scenery merge(zone23), broader combat styles/prayer/ranged/death/respawn acceptance, multi-player homing/social/trade, logout/settings, remaining targeting/retry/cancel gates, roof/bridge/occlusion comparisons and sustained travel/reconnect sessions. Spell flight/impact and hitmarks are implemented now; do not reclassify them as unimplemented based on historical sections. Cursor/Grok audit recovery remains deferred. Review SOURCE-INVENTORY.md for added Java/TypeScript/client-tool source directories. Check current usage/reset before restarting work.

---

# Sol MIDI and action checkpoint — October 21, 2026

This section supersedes older status below. Continued the authorized plan using the bundled revision-225 Java client, TypeScript port, engine and content. SOURCE-INVENTORY.md records the additional directories and source roles. No delegation or external audit recovery occurred.

Installed native SHA-256: `8D430F47FC59403DB7513A7A1F6F84435CFD8A13BE692DA9EA4298790DB302EF`. Launch play_client.bat. Cumulative overlay: midi-actions-checkpoint.zip with its .sha256 sidecar; existing engine/cache/content and build dependencies are required. Prior packages and executable snapshots remain preserved.

## Implemented

- Network MIDI now uses bounded, recoverable headerless BZip2 decoding with exact output length, stream CRC, packed song CRC/size checks and safe filenames. All 230 actual songs/jingles (4,783,946 decoded bytes) match the original Java decoder, including the cache's RIFF/RMID song. The 2 MiB cap is a native resource policy. Legacy cache/map callers still use the old decoder; this is not a claim that all decompression paths are hardened.
- Unique process/serial MIDI output files avoid rewriting open files or colliding between clients. MCI failures are reported. Original packet 212 delay is counted in 20 ms logical steps; a jingle replaces the ambient song and the original song restarts at expiry. A new song cancels the pending restart. Windows refused two simultaneous sequencer tracks; closing the ambient handle first resolves that observed failure. Audible/device timing parity remains unverified.
- Walking now uses the original yellow cross frames 0–3; successful world action dispatch uses red frames 4–7 at the actual click. Original 400 ms lifetime, placement and viewport clipping are shared across the action families. Cross feedback means the operation was dispatched, not that the server guaranteed success.
- Item targeting now reaches NPCs, scenery, ground items and players. Spell selection uses the real cache component metadata and target masks, and reaches those world targets plus held items. Selection, cancellation, source-slot checks, menus, routing and packet fields come from the original dispatcher. Live music buttons await the server's song command.

## Evidence and preservation

All 42 test programs pass with zero compiler warnings (release-check.log/exit). The optional standalone live-login test explicitly skips; separate actual SDL sessions ran. New comparisons cover 230 cache MIDI assets, malformed/corrupt/boundary/multiblock streams, all 65,536 jingle delay values, 24 complete original-Java cross sprite framebuffers and all nine encrypted original item/spell target streams including approach packets. MCI accepts ambient playback, jingle replacement and restart in the device test; this is not a listening test.

Actual SDL banker interaction showed red feedback and authoritative dialogue; a subsequent ground click showed yellow feedback. Actual inventory Drop logs, select tinderbox and click the rendered logs sent OPOBJU, showed red feedback, received the server's firemaking success message and increased Firemaking XP from 0 to 40. The exact final executable selected Wind Strike from the real spell interface and clicked a rendered chicken: red feedback, one air/mind rune consumed, Magic XP 0 to 9, and authoritative bones/raw-chicken drops. The server's projectile event 69 remains explicitly unimplemented. Server developer commands prepared the disposable account, location and items; natural tutorial completion/acquisition is not claimed. Live records are pinned to their individual binary hashes in the run README; do not transfer every prior visual check to the final build.

All 21 starting saves remain present; 20 remain byte-identical. antigravity.sav changed on disk at 05:39:39 during the run; its writer/cause was not established and its current contents were preserved. Native live probes used only new solcross211, solcrossbank and solfire211 accounts; retain those too. Production server PID3128 was reused and left running. No direct save writes, server rules, cache files, environment configuration or Git history edits were made by this work. Before snapshots, failed probes, source diff, original oracle generators, fixture data, real SDL captures and manifests are under client-c/verification/completion/runs/2026-10-21-midi-actions-sol.

## Continue next

Continue AUTONOMOUS_PLAN.md: complete live spell/combat feedback, hitmarks/health bars and original effects/projectiles (unsupported zone families 23/42/69/191), remaining item/target/retry/cancel gates, social/exchange/logout/settings flows, renderer roof/bridge/occlusion comparisons and sustained sessions. Full gameplay and original visual parity remain incomplete. Check the run README for the current spell probe outcome. Older pause/usage boundaries below are historical.

---

# Sol navigation checkpoint — October 21, 2026



Current native SHA-256: `487B85739750A5456CD5D0DB1BBCAC4ACC8360FA9BA9C7610377FA3197F94F9E`. User authorized continuation. Navigation same-tile and long-route handling, original dot masking and trig-table constant corrected. All39 programs pass warning-free;20 original-Java routing cases,4096 trig entries and160 exact sprite framebuffers match. Final-binary live release-road verifies modal/menu gating, natural walking across a region rebuild, destination preservation and interruption before the former destination. Exact evidence/limits: runs/2026-10-21-navigation-sol/README.md and the top of HANDOFF.md. Current overlay: navigation-verified-checkpoint.zip. SOURCE-INVENTORY.md records added directories. Original12 saves preserved; retain new solnav* accounts and server3128. Remaining input/approach gates, bounded MIDI decompression and the broader roadmap remain open. No external audit recovery or delegation occurred.



---



# Final current checkpoint



Read the new top of HANDOFF.md and TEST-THIS-CHECKPOINT.md. Native SHA-256: `78346F76B3BE7C689B2C472F60E092FA647B64F31D92AD742943D3AEE406D28F`. Latest complete package: handoff-checkpoint-2026-10-21.zip. Eight small changes built cleanly; tests/live checks were not rerun. No further code work is running. Cursor/Grok recovery deferred.



# Current progress — October 21, 2026



PAUSED at the user's request for manual testing. Read the top of HANDOFF.md, TEST-THIS-CHECKPOINT.md and NEXT-STEPS-AND-GEMINI.md. Full functionality and original visual parity remain incomplete.



Verified checkpoint: live ground quantity/reveal, modal/sidebar separation, bank Deposit/Withdraw X and server-confirmed drag, natural banker dialogue/player portrait, natural shop Buy/Sell, and ordinary stairs up/down. Region rebasing retains received actor positions and path queues; plane changes rebuild minimap. Fixture teleports/preparation establish bounded scenarios, not natural travel or tutorial completion.



Audio: 435 actual cache sounds at loop counts1/2 match 20,359,589 original Java WAV bytes with a shared noise input. Fixed five-harmonic terminator desynchronization, invalid track bounds and invalid-loop duration arithmetic. MIDI/device timing/random-generator parity remain unverified.



All37 programs passed with zero warnings: world-loops/full-suite-audio-fixed.log and navigation/incremental-suite.log, each with exit0. The latter also covers the Makefile refactor to shared objects/dependency files. No minimap navigation feature was implemented. The retained executable SHA is in HANDOFF.md and the release manifest.



Detailed scenarios, failed probes, reference provenance and lossless PNG captures: runs/2026-10-21-world-loops/README.md. History from earlier work remains in HANDOFF.md and prior run directories. The progress file was found empty during packaging; this summary reconstructs current status from those records.



Preserve every existing and new account, cache and history. Server PID3128 on43595/8889/8899 was left running; verify listeners before startup. Next: user feedback, minimap navigation, remaining item-use/spell/combat/skilling/social/effect gates, renderer comparisons and sustained sessions. Gemini's first bounded assignment is a read-only minimap protocol audit; no delegation has been started.



## Subsequent small task: MIDI failure reporting



Completed the bounded MIDI status fix. Installed executable SHA-256: `773E06C0D5508E0A6510DD8ADAD5EEABA1D85CBFB9F08C613D6B545598736BE8`. Build exited0 with zero warnings; no tests or audible/live checks were run for this change. Prior suite/live evidence applies to the old binary. See runs/2026-10-21-midi-status/README.md and source-review.diff. The earlier checkpoint ZIP is preserved; midi-status-client-update.zip contains this newer executable. Latest usage75%; write the full handoff at95% as requested. No additional feature task has started.



## Small task: minimap dot offsets



Matched revision225 division-before-subtraction for NPC/ground-item minimap dot offsets on both axes. Built exit0 with zero warnings; no tests/live visual checks run. Latest executable SHA-256: `39CEFC6F09C3A7557081D8FE41211E58D2E56A7E7619EC9DBB090AA6C5ACB88B`. See runs/2026-10-21-minimap-offset/README.md and source-review.diff. Latest update ZIP: minimap-offset-client-update.zip (also includes MIDI fix). Full minimap navigation/destination flags remain open.



## Small task: ordinary player minimap dots



Draw active received players using original mapdots index2, gated on non-null/visible. Friend colouring still awaits synchronized social state. Build exit0/zero warnings; no tests or live two-client visual check run. Latest native SHA-256: `61DEF5023541E50CF56E22CAF6D8D30FD04EFBA34E664E37A8FAECA4B2B864FF`. See runs/2026-10-21-player-dots/README.md and source-review.diff. Latest cumulative update: player-dots-client-update.zip. Usage78%; full handoff at95% as requested.



## Small task: scene-walk destination flag



Loaded original mapflag; set from successfully sent scene-click route; clear on arrival/packet19; retain world destination on rebase; original distance/mask rules and final centre marker. Scope excludes approach-route flag replacement and minimap clicking. Build exit0/zero warnings; no tests/live scenario checks run. See runs/2026-10-21-destination-flag/README.md and source-review.diff. Latest native SHA-256: `5799B7166660D13580BDEB7BE0537F68F33D2CC518DEB9D60376E71B82F9F868`. Latest cumulative package: destination-flag-client-update.zip. Full handoff remains due at95% usage.



## External audit status correction



The user clarified that Cursor/Grok partially performed the external audit before exhausting usage. Prior statements that no work had run are superseded by this report. The expected gemini-audit directory is empty; no partial deliverables were located in the checkout by filename/content searches, so their scope and correctness remain unknown. Asked for their saved path or final response. GEMINI-AUDIT-TASK.md now instructs recovery and continuation before duplicate analysis, records Cursor/Grok attribution, and separates recovered claims from reviewed findings. No external findings have been accepted or integrated in this chat yet. No code or executable changed during this documentation correction.



## Small task: operation-approach destination flag



Shared approach helper now updates optional pointers to the same destination after a successful nonempty send. Failed/empty routes preserve prior state. NPC/loc/player/ground-item callers share this behavior. Build exit0/zero warnings; no tests/live checks run. Latest native SHA-256: `EC1BC954A68815B6DA156E60F98594418A528E13243AC515DAAD829D4AD3E2C1`. Latest cumulative package: approach-flag-client-update.zip. See runs/2026-10-21-approach-flag/README.md and source-review.diff. Cursor/Grok recovery deferred; its brief now reviews this completed integration. Full handoff at95% as requested.



## Small task: MIDI_SONG delimiter and layout



Fixed byte10 name termination, exact metadata length, native name bounds and unsigned metadata assembly. Build exit0/zero warnings; no tests/live audio checks run. Latest native SHA-256: `2BB6B9E47EBA7C008A89B759429AB17ADE7979586296D9351812A9D881DAF104`. Latest cumulative update: song-packet-client-update.zip. See runs/2026-10-21-song-packet. Follow-up: network-driven music still uses a decompressor without output-capacity bounds/error recovery. Full handoff at95% as requested.



## Small task: defined signed packet integer reads



g4/g8 now assemble unsigned values and map to signed Java values without undefined signed shifts or out-of-range casts. Build exit0/zero warnings; no tests/live checks run. Latest native SHA-256: `20D9AD3D709FB183649B509D3AA1F3BF8D99F0B1081361545A091C94339B8C23`. Latest cumulative update: packet-integers-client-update.zip. See runs/2026-10-21-packet-integers. Full handoff at95% as requested.



## Visible task: clickable live minimap



Original inverse click transform/current zero zoom-angle, collision routing and opcode165 with14-byte trailer implemented. Successful send sets destination; actual movement stays server-driven. Main modals/drag/context menus gate input. Build exit0/zero warnings; no tests/live checks run. Latest native SHA-256: `78346F76B3BE7C689B2C472F60E092FA647B64F31D92AD742943D3AEE406D28F`. Latest cumulative update: minimap-click-client-update.zip. See runs/2026-10-21-minimap-click. Usage92%; full handoff at95%.

