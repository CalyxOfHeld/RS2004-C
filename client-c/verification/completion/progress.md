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

