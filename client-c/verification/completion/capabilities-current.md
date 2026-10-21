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

# Current capability evidence — October 20, 2026

## Current checkpoint: ground items and preservation correction (October 20, 2026)

This section supersedes conflicting status/launcher instructions below. The later sections are historical evidence, not authorization to reset saves. Current native SHA-256: `B77A420F8925F890F208E494D1E1DD39A4D0F49C458CD15D17500287071E9502`. No new ZIP was assembled in this run. Use the current in-place executable and sources; older ZIPs are stale and the hint-arrow-fresh-start ZIP contains the withdrawn save-purge launcher.

Removed the launcher's automatic recursive save purge before testing. Never restore that behavior from the older hint-arrow-fresh-start package. Both Windows launchers still automatically supply the default test account; explicit credentials select another account without changing any save. All six pre-existing saves remain byte-identical in this run. New disposable ground201/groundtut201 saves are retained. Existing production server PID3128 on43595/8889/8899 was reused and left running; no other server/client was stopped.

Implemented production ground-item ADD223/DEL49/COUNT151/REVEAL50, owner suppression using UPDATE_PID139, full-zone clearing, duplicate-preserving value-ranked piles, quantity cache models, three distinct displayed items, owned-model cleanup, region rebase retention, model picking, original minimap dots and typed Take with approach/OPOBJ. Cache/fixture/loopback regression covers state, malformed enclosed rejection, rendering/picking and encrypted approach93/Take200/world coordinates.

Actual SDL `verified-loop` passed on the unchanged production server: Drop bronze dagger1205 from slot0/component3214, receive ADD at world3232,3232 and empty inventory; select rendered Take; receive DEL and restored dagger in slot0. Captures at frames80/160/240/320/400 show the lifecycle. Fixture setup used existing developer commands on a new disposable account; this is not natural acquisition/travel/tutorial completion. Other early probes include missed clicks and pending logout5 and are not acceptance evidence.

Inherited audio/dialogue/tutorial/hint claims were audited against source/tests. Audio's old bit-for-bit PASS had no Java sample comparison and is corrected. Bank drag tests manually duplicate the swap and do not verify live banking. Player-head tests establish cache composites, not a scripted live dialogue. Live default-tab fallbacks were overriding server tutorial restrictions; removed those fallbacks. Audio queue and hint/tab blink timing moved into logical game ticks. A new-account SDL capture verifies initial sticky tutorial text, locked tabs and NPC arrow only. See `recent-claims-audit.md` for exact limits.

Evidence and runnable input: `client-c/verification/completion/runs/2026-10-20-ground-items/README.md`. All 36 test programs completed with exit0 and zero compiler warnings (`full-suite-fixed.log`, `.exit`). Optional standalone live login explicitly skipped without opt-in credentials; actual live SDL evidence is separate. The initial missing test-world link dependency is fixed and its failure log retained. `relog.log` confirms the restored dagger persists in slot0 after a later login. Do not infer overall gameplay completeness from passing tests.

Exact next dependency: live stack quantity/public reveal with two disposable accounts, then broader routing/reach/cancel/region/plane checks and bank/shop loops. Ground-item bridge/raised/multi-item original-render comparisons remain unverified; broader roof/occlusion semantics, audio oracle, live banking/drag/count, player dialogue, social and effects remain open. Unsupported zone families:23/42/69/191. Preserve history, cache and every save. Continue the active plan without routine approvals.


`capabilities.md` is the initial source audit, deliberately preserved. This table supersedes status for the listed paths only. Other entries retain their unverified/partial status.

| System | Current evidence | Scope and remaining gaps |
| --- | --- | --- |
| Transport framing/receive | Production parser through loopback TCP: split/coalesced bytes, bounds, disconnect, zone sizes; actual server sessions | Unit/transport regressions plus live sessions; not all incoming packet behavior implemented |
| UI inventory/state | Production wire fixtures, real-cache hit tests; actual server Wield dagger slot 1, Wield sword slot 0, Remove worn slot 3 | Server accepted exact object/slot/component; received authoritative inventories and appearance; banking/shop/use-on-target not end-to-end verified |
| Combat style | Actual SDL Lunge click component 2285; handler accepted, returned varp 43=1 | Correct current-session selection. Relog returned style 0; do not claim persistent style selection |
| Character design | Fresh disposable account through actual SDL Accept; production handlers accepted design and button 3651 | Defaults from cache kits. Female highlight/preview animation parity still incomplete |
| Entities | Actual active NPC/player lists render; bounded masks and removal fixtures | Animation blending, exactmove/spotanim rendering, hitmarks and world operations still incomplete |
| Terrain | Real cache, height-normal/shadow lighting, weighted underlay blending; native/reference scene pair at identical position/camera | One Lumbridge comparison; neutral hue/light offsets. Bridge/roof/upper-plane semantics, full scene light sharing and terrain categories remain |
| Minimap | Scene-floor colors/shape masks, wall outlines, mapscene/mapfunction assets from cache; inspected actual live capture | No fallback terrain paint. Icon jitter remains at exact cache position; destination/hints/item/player dots, plane transitions and click routing incomplete |
| Texture assets | Existing 50 cache textures; no generated/replacement bitmap assets | Native ground uses full texture rasterization; Java comparison configured low memory uses its source-defined HSL texture fallback. This is a named setting/path difference, not exact parity |
| Public chat | Local fake live echo removed; unsupported send is explicitly reported | Actual public/private/social transport and incoming display remain incomplete |
| World targets | Reference collision/reach oracle, target BFS, op93 routing, loc picking; actual Cook dialogue/choice and door Open/Close | NPC/loc dispatch verified in these scenarios; player dispatch unverified; ground-item state/dispatch pending |
| Verification | Reproducible input replay, observed production handler delegation, isolated original Java buffers | Fixture account preparation is not tutorial completion. Full-suite result and binary hashes are recorded in the release manifest |

Next dependency: ground-item zone state, real stack models/picking/minimap dots and Take, followed by broader bank/shop/combat loops. Collision/reach, Cook dialogue and ordinary door operations now have separate oracle/live evidence. Continue remaining packet/interface coverage and visual plane semantics in the roadmap.

Post-foundation extension: 32 test programs exit successfully (optional standalone live-login scenario skipped); original Java CollisionMap oracle matches 33,408 reach results and wall/footprint flags. Dynamic scenery update/restoration uses production zone handler with actual-cache fixtures and actual door Open/Close. Owned scenery model cleanup and missing-shape rejection added. Player portrait packet197 and other UI/effect families remain incomplete.

## Shared layered-model flicker correction (October 20)

Latest native SHA-256: `21BF30F48D84A5ED5EA5A4E3202EE72C479B2B3A79AF8A7A3D906D682D7DADC4`. See RENDER-FLICKER-FIX.md and client-c/verification/completion/runs/2026-10-20-flicker/comparison.html.

The tutorial-island white carpet has authentic black cache geometry, with detail/base faces exactly coplanar and different face priorities. Native rendering ignored cache priorities and rounded camera vertices before shared depth testing, so overlapping triangles could alternately hide their decoration. Implemented shared original Model.draw priority grouping (including10/11 interleaving) for world and item-icon paths, preserved source constant priorities in composite models, and used one continuous camera transform plus double reciprocal depth for ground/models. Relative64-ULP depth tolerance handles numerical equality; no geometric offset, carpet-ID exception, asset editing or replacement texture was added. This is a native depth-renderer precision correction; exact original integer-rasterizer image parity is not claimed.

User-authorized GPT-6 Luna independently wrote tests/test_coplanar_visibility.c. Actual cache model1067 detail-only mask survives full rendering across28 views; reversed synthetic face order, composite priorities and nearer/farther model occlusion also pass. Separate actual-cache1110/1134 masks pass80 views /22,383 pixels. Preserved pre-fix sources fail the same actual-cache check. Parent independently reproduced the old-source failure after a warning-free successful baseline build; baseline-reproduction-command/result/build/run in tester/ are authoritative. Historical initial baseline-build.log is a mixed-source link failure, not that successful reproduction.

Fresh full-suite.log: all33 programs completed, zero warnings. Optional standalone live-login scenario explicitly skips without opt-in credentials; current real live-client evidence is separate. Actual SDL before/after captures have matching world camera and player position (matched-camera.json). fixed-reset-walk.log and40 PNG frames record two click-to-walk routes, incoming player movement and15 camera positions; complete decoration remains visible in inspected movement captures. The location fixture uses the existing developer teleport on a disposable account; no natural-travel/tutorial-completion claim. Original accounts and server rules were not reset. Local production server remains on43595 for user testing.

Current executable and updated checkpoint ZIP include the shared fix, tests, evidence, this handoff and roadmap. Both handoffs synchronized. Close/reopen play_client.bat to load the rebuilt executable. Full functionality, broader original/native parity, effects, occlusion/roof semantics and ground-item next dependency remain as documented above. Delegated tests cover this bug class and three assets, not an exhaustive tour of every model.


Login usability: Windows launcher automatically supplies the existing local test account; code 5 correctly describes an active/pending logout session, and code 3 invalid credentials. Verified offline wrapper forwarding and rendering. This does not verify a new live account login.


## Native credential guard completed (October 20; launcher defaults restored below)

Removed native main's hard-coded account fallback. Both RS2_USERNAME and RS2_PASSWORD must be nonempty for live mode; otherwise the executable exits 1 with launcher guidance before SDL/cache initialization or any network attempt. Offline mode requires no credentials and both native/launcher messages identify it as a scene preview. Automatic account selection through the Windows launchers is the normal live entry point.

Verification: warning-free native rebuild; five actual executable cases (missing both, username only, password only, empty username, empty password) rejected with zero connections to a temporary loopback listener. With both explicit test values supplied, the executable reached that listener and correctly reported its missing server seed. This was transport-only, with no account backend or successful-login claim. test_client.bat --offline rendered the actual cache to offline-preview.png with exit 0 and zero observed connections. All seven checks pass; source diff, build output, per-case logs, results.json and runnable verify_startup.py are in client-c/verification/completion/runs/2026-10-20-explicit-login/. The earlier 33-program suite and live gameplay captures remain prior evidence; neither was rerun for this startup-only change. No real account, save, running game server or asset was modified.


## Automatic local test login restored (October 20)

User correction: testing gameplay must not require entering credentials each run. play_client.ps1 now automatically supplies the existing antigravity/antigravity local test credentials when neither RS2_USERNAME nor RS2_PASSWORD is set. Both play_client.bat and test_client.bat use this path with no prompt. A complete explicit environment pair still selects another account. A partial pair is rejected rather than mixing accounts; clear both variables to return to the default. Offline previews bypass credentials. The native executable's nonempty-credential guard is retained; the launchers satisfy it automatically. No server authentication or save handling was changed.

Actual live verification against the existing production test server on43595, without credential variables or offline mode: play launcher auto-login succeeded, received real account state, rendered80 frames and exited0. Evidence is in client-c/verification/completion/runs/2026-10-20-default-autologin/default-live.log and default-live.png. No account resets, teleports, fixture items or server restarts. The separate native client already running was left open.

The compile/test batch path (which invokes play_client.bat) also auto-logged into the live server without credential variables, rendered80 frames and exited0 with no compiler warnings. test-default-live.log, test-default-live.png and results.json record this second actual-client check. Both launcher paths are verified live.


## Server startup port preflight (October 20)

start_local_server.bat now runs check_local_server.ps1 before invoking Bun. Free configured ports permit the existing foreground startup path. If43595/8889/8899 already listen in one process, the launcher reports those facts, skips a second server and returns0; the existing server window remains the readiness authority. Partial or split-process port conflicts and failed checks stop startup with a clear message. No process is terminated and no ownership of an arbitrary listener is assumed. This checks existing listeners, not concurrent launches before any listener has bound.

Four checks passed: temporary real sockets covering all-occupied/single-owner, partial conflict and free-port cases; and the actual server batch against production PID11324, which returned0 without starting a second server. No server restart, account change or native-client source change. Per-case logs and results.json are in client-c/verification/completion/runs/2026-10-20-server-preflight/. Default automatic game login remains enabled.


## NPC minimap visibility correction (October 20)

Shared minimap_draw_npc_dot now requires an NPC with a decoded type and type.minimap enabled. Both live and offline NPC paths use it. This follows bundled javaclient/src/main/java/deob/client.java drawMinimap lines3515-3520 and jagex2/dash3d/entity/NpcEntity.java isVisible (type != null). Previously every nonnull live NPC received a dot even when the real cache set opcode93 to hide it. No cache, scenery or sprite changes. Trace output now includes each NPC's cache minimap flag.

Warning-free client/minimap-test rebuild. Expanded existing minimap suite renders the original cache mapdot for all1017 NPC definitions:933 permit a dot,84 suppress it; null and untyped NPCs also leave the framebuffer unchanged. Existing floor/mask/radar tests pass. Actual live SDL check on the existing disposable codexdesign account received six NPCs including hidden fishing spots/butterflies and visible guides; captured80 frames and exited0. Default account returned5 because it was in use; existing user clients were left open and the failed attempt is preserved as default-account-busy.log. No teleport, item fixture, account reset or server restart.

Evidence: client-c/verification/completion/runs/2026-10-20-npc-minimap/ (build, cache regression, live log/PNG, results and source diff). Targeted minimap suite was rerun; earlier full33-program suite remains historical. Remaining minimap coverage and broader gameplay roadmap are unchanged. Automatic default login remains enabled.

## October21 live-loop extension (supersedes earlier pending bank/item/audio rows)

| System | Evidence | Remaining scope |
| --- | --- | --- |
| Ground quantities/public reveal | Real two-account ADD/COUNT/REVEAL,150 to200 coins; private pile hidden until public reveal | Raised/bridge/multi-item original-render comparisons and broader session coverage |
| Modal/sidebar and bank | Real Deposit/Withdraw X, quantity10/5, drag0 to2 accepted by server, close/reopen/tab restoration; natural banker dialogue/choice/player portrait opens bank | Notes, full-bank/limits, nested scrolling and broader trade/container interfaces |
| Natural shop | Real Lumbridge shopkeeper Trade, Buy1 pot, Sell1 pot with authoritative stock/coin/inventory replies | Larger amounts, specialized shops, affordability/space boundaries |
| Region/plane state | Reversible decoded entity path/world rebase; live region teleports and ordinary Lumbridge stairs0 to1 to0; minimap rebuilds on plane changes | Natural boundary walks/reconnects, roofs/bridges and full visual parity |
| Audio synthesis | Original Java oracle,870 WAV records across435 cache sounds,20,359,589 equal bytes with shared noise input; malformed cache rejection | Original random table generator differs; audible timing/settings/MIDI and arbitrary loop counts unverified |

Detailed source/evidence scopes and failed probes: `runs/2026-10-21-world-loops/README.md`. Full functionality remains incomplete.
