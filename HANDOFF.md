# Current handoff — October 21, 2026

## Read this section first

Stopping at the requested usage handoff boundary. The last usage reading before writing was93% used/7% remaining; the user reserves the final5% for handoff. No further implementation is running. The small final task consolidated TEST-THIS-CHECKPOINT.md and continuation instructions. All lower sections are historical and are superseded where they conflict with this section.

Latest runnable native: `client-c/bin/rs2client.exe`.
SHA-256: `78346F76B3BE7C689B2C472F60E092FA647B64F31D92AD742943D3AEE406D28F`.
Complete update: `handoff-checkpoint-2026-10-21.zip`; authoritative archive checksum is its .sha256 sidecar. It overlays this existing rewrite checkout and requires the existing engine/cache/content. `play_client.bat` launches the installed native on127.0.0.1:43595. TEST-THIS-CHECKPOINT.md is the consolidated user guide. No remote is configured; no Git push or commit was made. The Git working tree is predominantly untracked; do not reset it or discard changes.

## Eight small code tasks since the tested world-loops checkpoint

1. MIDI backend checks open/play/status/restart errors and reports failed song commands; short writes/signature buffers reject.
2. Minimap dots quantize target/player coordinates separately, matching original Java.
3. Ordinary visible-player minimap dots use original cache mapdots2. Friend colouring awaits social state.
4. Original cache mapflag renders scene-walk destination; arrival/packet19 clears it, region rebase shifts it, distant markers respect the mapback mask.
5. Shared operation approach helper updates the same flag after a successful nonempty route send.
6. MIDI_SONG54 uses byte10 name termination and exact metadata framing; unsigned metadata assembly avoids signed shifts.
7. Shared g4/g8 assemble unsigned values and map to signed Java values without C signed-shift overflow/out-of-range casts.
8. Live minimap clicks use original x582..727/y14..164 rectangle, inverse camera rotation and collision routing. Opcode165 shares route compression and appends the original14-byte trailer, included in packet length. Zero angle/zoom match current rendering. Successful sends set the flag; local movement is not fabricated. Menu/drag/main-modal gates apply; same-tile/failed routes send nothing. Ctrl forwards the run modifier.

Each task has source snapshots, source-review.diff, build.log, build.exit, README and manifest under `client-c/verification/completion/runs/2026-10-21-<task>/`. Every build exited0 with zero warnings. No tests or live sessions were run for these eight changes. Do not transfer old acceptance claims to this binary. The final minimap implementation is unverified in a live session.

## Earlier evidence that remains valid for its recorded checkpoint

The old `world-loops-test-checkpoint.zip` preserves executable3035DF94DAFA31A55C038CB1F25DEC795118BD7513EBA85AA463B3ACF0077919. Its37-program suite and the later shared-object Makefile suite passed. Ground COUNT150->200/public reveal, bank X/drag, natural bank dialogue/player portrait, natural shop Buy/Sell and stairs up/down have bounded instrumented live SDL evidence in runs/2026-10-21-world-loops. Fixture placement/preparation is explicitly distinguished from natural travel/acquisition. Failed probes are retained. Captures were losslessly converted to PNG; replay scripts still name original BMPs.

Original Java audio oracle matches all435 cache sounds at loop counts1/2 for20,359,589 WAV bytes with a shared deterministic noise table. This is not random-generator, MIDI or device-timing parity. It fixed actual tone terminator/track-bound/invalid-loop errors; fixture and provenance remain packaged.

## Next work

- Address the user's feedback first. A live minimap click/camera/arrival/cancel/region check and regression coverage are the immediate verification gaps; tests were not requested for these small tasks and were not run. Do not label them passed from build output.
- Review original-input parity, nearest endpoints, route compression limits and nonzero minimap angle/zoom before claiming full navigation completion. Original/server sources are authoritative; do not use OSRS or later revisions.
- Network-driven MIDI still calls the shared bzip_decompress without output-capacity bounds and with fatal error handling. This is a known follow-up, not fixed by packet framing checks. Song CRC/size validation, temporary-file collisions, jingle delay and device behavior also remain incomplete.
- Continue remaining item-use/spell/combat/skilling/social/effect and renderer/session gates from AUTONOMOUS_PLAN.md. Zone families23/42/69/191 remain unsupported. Roof/bridge/occlusion and complete gameplay/visual parity remain unfinished.

## Preservation and external-agent work

Keep all operations/temp files within2004scape-LostCityRS. Preserve cache, history and every save, including stack211a/stack211b/bank211/bank212. Existing server PID3128 was confirmed owning43595/8889/8899 at this handoff; leave it running and verify ownership before any startup. Do not stop unrelated processes or restore the old launcher's save purge. No .env or test-account passwords are packaged.

The user clarified that Cursor/Grok partially attempted the external audit before exhausting usage. Its output has not been located or reviewed; the expected gemini-audit directory is empty. The user explicitly deferred searching. Do not search again without a new request or supplied path/output. No external findings were accepted here. GEMINI-AUDIT-TASK.md retains its historical filename but directs review of current implementation rather than duplicate work. Do not spawn GPT delegates. User requested Sol or Astra for continuation; no new task/model switch has been started.

---

# Handoff: revision-225 native client — paused for user testing

## Current verified checkpoint - October 21, 2026

This section supersedes older status, hashes and package instructions below. Current native SHA-256: `3035DF94DAFA31A55C038CB1F25DEC795118BD7513EBA85AA463B3ACF0077919`. Test package: `world-loops-test-checkpoint.zip`, checksum in its `.sha256` sidecar. This updates the existing checkout; it is not a standalone server distribution. Launch the installed `play_client.bat`; see TEST-THIS-CHECKPOINT.md. Full functionality and original visual parity remain incomplete.

Verified live ground COUNT150 to200 coins and public REVEAL with two disposable accounts; modal/sidebar separation and normal-tab restoration; bank Deposit/Withdraw X and server-confirmed dragging; natural Varrock banker dialogue, choice, player portrait and bank access; natural Lumbridge shop Trade/Buy/Sell; original Lumbridge stairs1738 up and1739 down change planes0 to1 to0. Region rebase retains actual actor positions/queues; minimap rebuilds on plane changes. Fixture preparation used existing developer commands only on new test accounts; those commands do not prove natural acquisition/travel/tutorial completion.

A new original Java audio oracle exposed and fixed a missing five-harmonic terminator, out-of-range decoded track writes and invalid-loop duration discrepancies. All435 cache sounds at1/2 loops match20,359,589 WAV bytes with a shared noise-table input. Random generator, audible device timing and MIDI parity remain unverified. Malformed tracks/truncated streams reject cleanly.

All37 test programs completed with exit0 and zero compiler warnings in `client-c/verification/completion/runs/2026-10-21-world-loops/full-suite-audio-fixed.log`; optional standalone live login explicitly skips without opt-in. Actual live evidence is separate. README.md in that run describes exact scopes, successful replays, failed probes, reference provenance and PNG captures. Raw BMP captures were losslessly converted to PNG and pixel-verified; replay scripts still request original BMP paths.

All existing saves and history remain preserved. New test saves stack211a, stack211b, bank211 and bank212 must also be preserved. Existing production server PID3128 on43595/8889/8899 is reused and left running. Verify current listener ownership before startup; do not launch a duplicate.

PAUSED at the user's request for a testable client, next-step plan and Gemini assessment. No feature work is running. Read NEXT-STEPS-AND-GEMINI.md and GEMINI-AUDIT-TASK.md. Resume with user-reported failures, then minimap navigation and the remaining roadmap. User requests a runnable handoff at95% usage/5% remaining; this handoff is written early at the requested pause. Last check:71% five-hour and11% weekly used. No Git remote is configured; the local ZIP is the delivery, with no remote push or commit.

## Current checkpoint: ground items and preservation correction (October 20, 2026)

This section supersedes conflicting status/launcher instructions below. The later sections are historical evidence, not authorization to reset saves. Current native SHA-256: `B77A420F8925F890F208E494D1E1DD39A4D0F49C458CD15D17500287071E9502`. No new ZIP was assembled in this run. Use the current in-place executable and sources; older ZIPs are stale and the hint-arrow-fresh-start ZIP contains the withdrawn save-purge launcher.

Removed the launcher's automatic recursive save purge before testing. Never restore that behavior from the older hint-arrow-fresh-start package. Both Windows launchers still automatically supply the default test account; explicit credentials select another account without changing any save. All six pre-existing saves remain byte-identical in this run. New disposable ground201/groundtut201 saves are retained. Existing production server PID3128 on43595/8889/8899 was reused and left running; no other server/client was stopped.

Implemented production ground-item ADD223/DEL49/COUNT151/REVEAL50, owner suppression using UPDATE_PID139, full-zone clearing, duplicate-preserving value-ranked piles, quantity cache models, three distinct displayed items, owned-model cleanup, region rebase retention, model picking, original minimap dots and typed Take with approach/OPOBJ. Cache/fixture/loopback regression covers state, malformed enclosed rejection, rendering/picking and encrypted approach93/Take200/world coordinates.

Actual SDL `verified-loop` passed on the unchanged production server: Drop bronze dagger1205 from slot0/component3214, receive ADD at world3232,3232 and empty inventory; select rendered Take; receive DEL and restored dagger in slot0. Captures at frames80/160/240/320/400 show the lifecycle. Fixture setup used existing developer commands on a new disposable account; this is not natural acquisition/travel/tutorial completion. Other early probes include missed clicks and pending logout5 and are not acceptance evidence.

Inherited audio/dialogue/tutorial/hint claims were audited against source/tests. Audio's old bit-for-bit PASS had no Java sample comparison and is corrected. Bank drag tests manually duplicate the swap and do not verify live banking. Player-head tests establish cache composites, not a scripted live dialogue. Live default-tab fallbacks were overriding server tutorial restrictions; removed those fallbacks. Audio queue and hint/tab blink timing moved into logical game ticks. A new-account SDL capture verifies initial sticky tutorial text, locked tabs and NPC arrow only. See `recent-claims-audit.md` for exact limits.

Evidence and runnable input: `client-c/verification/completion/runs/2026-10-20-ground-items/README.md`. All 36 test programs completed with exit0 and zero compiler warnings (`full-suite-fixed.log`, `.exit`). Optional standalone live login explicitly skipped without opt-in credentials; actual live SDL evidence is separate. The initial missing test-world link dependency is fixed and its failure log retained. `relog.log` confirms the restored dagger persists in slot0 after a later login. Do not infer overall gameplay completeness from passing tests.

Exact next dependency: live stack quantity/public reveal with two disposable accounts, then broader routing/reach/cancel/region/plane checks and bank/shop loops. Ground-item bridge/raised/multi-item original-render comparisons remain unverified; broader roof/occlusion semantics, audio oracle, live banking/drag/count, player dialogue, social and effects remain open. Unsupported zone families:23/42/69/191. Preserve history, cache and every save. Continue the active plan without routine approvals.


Updated October 20, 2026. Verified progress checkpoint; full functionality and visual parity remain incomplete.

## Authorization and continuation

Continue autonomously using `2004scape C Rewrite/AUTONOMOUS_PLAN.md` and `client-c/verification/completion/progress.md`. No routine phase approvals. Keep every file read/write/temp/cleanup within `2004scape-LostCityRS`. Use the bundled revision-225 Java/TypeScript/server/content references and actual cache. Never substitute textures, gameplay or successful outcomes. Preserve original accounts and repository history. The initial capability audit remains in capabilities.md; current evidence is in capabilities-current.md.

## Updated build

`2004scape C Rewrite/client-c/bin/rs2client.exe`
SHA-256: `B7976187890A3FE4F9EFBA2C5BCA1F4D20C70D1BABFCCF9EF1839F6C8A09A124`

Package: `2004scape C Rewrite/hint-arrow-fresh-start-checkpoint.zip`, with matching .sha256 (`8D7A75C169770653E355B28E9AEE35C903EDA26903B26A6F233D125589D8A13D`). Extract into the existing rewrite folder; bundled engine/cache/content are required. It contains binaries/runtime, C sources/tests, reference harness/oracle, handoff/roadmap and evidence. No player saves, .env files or disposable test credentials are packaged; the Windows launchers automatically supply the existing local test account unless both credential variables override it. The prior foundation package is preserved under client-c/verification/completion/releases/.

`play_client.bat --offline` is explicitly a scene preview, not gameplay. Live mode uses the local server and RS2_USERNAME/RS2_PASSWORD; the Windows launchers automatically supply the existing local test account. A full native login UI remains unfinished. Failed live login never silently becomes offline simulation. The earlier observed server was stopped after its verification; the manual production server is now ready on43595 as described below. No server rules or .env files were changed.

## Implemented systems and actual-client results

- Bounded incremental network framing, partial/coalesced byte handling, unknown/oversize/disconnect rejection, zone sizes, keepalive, correct command delimiter and reference turn-first world-coordinate route compression.
- Transactional inventory full/partial and representative component/state packets. Typed actions carry the real operation/component/object/slot. 789x532 original client dimensions restore lower controls. Open context menus preserve their targets; reference low-priority Examine/attack sorting keeps ordinary operations usable.
- Fresh disposable character design accepted through the original design/button submission. Actual SDL Wield dagger from slot 1, Wield sword from slots 0 and 1 in separate runs, Lunge component 2285, and Remove worn slot 3/component 1688 all accepted by production server handlers, with matching inventory/worn/appearance replies. Unequipped state survived relog. Style returned to 0 on relog, so style persistence is not established.
- Bounded player/NPC mask decoding and active entity rendering. Clicked live routes no longer queue local movement and double incoming server steps. Server route endpoint and native final world tile match in the recorded walk check.
- Reference directional wall/rotated footprint/ground collision rules, bridge collision-plane adjustment, wall/decor/rectangular reach checks and target-aware pathfinding. Scenery has real model picking; typed NPC/loc/player dispatch sends opcode-93 approach before the operation. Player trade/follow dispatch is implemented but not end-to-end verified.
- Actual SDL Talk-to Cook accepted, received original Cook dialogue, Continue accepted, choice component 2485 accepted and original next scripted reply displayed. A real cache Cook was spawned using the existing developer command for this bounded scenario; this proves client/server dialogue flow, not natural travel or quest completion.
- Actual ordinary Lumbridge door Open (1530 at world3230,3235) and Close (1531 at world3230,3236) accepted after approach. Received delete/add scenery events update models, collision and minimap; actual open and closed captures inspected. Production zone handler supports follows/enclosed/add/delete/full-zone baseline restoration, validates enclosed packets before mutation, and explicitly reports still-unsupported zone families.
- Real cache terrain: weighted underlay blending, height-normal lighting, location shadows, correct missing/transparent/textured floor sentinels. Minimap: scene floor colors/shape masks, visibility flags, real wall marks, cache mapscene/mapfunction sprites. No fallback dirt paint or generated/replacement bitmap assets.
- Scenery model selection no longer substitutes an unrelated shape. Mirror, rotation-before-scale, offsets, slope skew, L-wall halves and diagonal centerpiece yaw now follow bundled references. Owned heap scenery models release on replacement/rebuild/shutdown; redundant fresh-load copying removed. Broader lifetime/performance audit remains.
- Unsupported live public chat never produces a fake sent-message echo. Offline preview no longer invents a Hans greeting. Chat prompt takes the actual synchronized player name.

## Verification and evidence

Everything is in `client-c/verification/completion/runs/2026-10-20-foundation/`.

- `zone-release-tests.log`: 32 test programs exit successfully, no compiler warnings. Optional standalone live-login scenario explicitly skipped without disposable credentials; CRC checks still run. Unit/cache/fixture/oracle/loopback evidence is distinct from live gameplay.
- `current-client-build.log`: historical foundation build. Current renderer build and captures are under runs/2026-10-20-flicker/ (fixed-build.log, fixed-live-yaw0, fixed-reset-walk). release-manifest.json records executable/source/evidence hashes and exact evidence scopes.
- Collision oracle: tests/fixtures/collision-reference.bin was produced by the original bundled Java CollisionMap class using reference/src/CollisionOracle.java. 33,408 wall/decor/rectangular reach decisions, wall add/delete flags and rotated footprint flags match. This establishes those methods, not all gameplay parity.
- `zone-regressions.log`: actual-cache Door update/delete/restore, malformed enclosed atomic rejection, 200 owned-model replacements with one retained scene entry, bridge collision-plane rule and missing-shape rejection. No blanket memory-leak-free claim.
- `observed-server.log`, `observed-server-next.log`, `world-server.log`: observational wrapper delegates unchanged production decoders/handlers; logged accepted operations are actual server evidence. Relevant client logs: trace-design, equipped-dagger, style-sword, unequipped, relog-unequipped, walk-authoritative, cook-talk, cook-continue, cook-choice, open-door, closed-door, current-native. Each has explicitly named scope.
- `cook-talk.bmp.png`, `cook-continue.bmp.png`, `cook-choice.bmp.png`: actual SDL dialogue states. `door-menu.bmp.png`, `open-door.bmp.png`, `closed-door.bmp.png`: real scenery input/state. `walk-verification.json`: accepted route and final world tile agree.
- `current-views/client-verification.txt`: six offline SDL render checks across four cameras, walking, viewport clipping and identical rebuild. These are fixture-rendering checks only.
- `reference-scheduled.log`, `java-*.png`, `comparison.html`: original Java render buffers and native views. Reference cache lookup and HTTP endpoint are confined to this project; observational stack trace/main-thread harness scheduling only, no original renderer changes. Prior region-buffer failure was a concurrent harness login race, resolved by invoking original login on its own update thread. The Java full-client image is labeled a composition of original buffers, not a desktop screenshot.

Disposable-account setup uses existing developer commands for items/tutorial flag/location/Cook. It is not tutorial completion, natural resource acquisition or quest completion evidence. Original account saves were not reset. The source baseline, historical scenery evidence and foundation checkpoint remain preserved. Do not rerun historical one-shot transformation scripts or baseline_audit.py.

## Exact remaining work

Next dependency: ground-item OBJ_ADD/DEL/COUNT/REVEAL synchronization, true item stacks/models/picking/minimap dots and Take, then a server-driven drop/take loop. These are currently explicitly reported as unsupported; projectile/spot/location animation families also remain. After that broaden ordinary world operations, cancel/reach/plane transitions and complete bank/shop/combat/skilling/social flows. NPC/door results do not mean all target families are complete.

Continue missing component/model/player-head/quantity/scroll/drag/spell behaviors, actual login/logout/reconnect, public/private/social transport, audio/effects/hitmarks and sustained-session checks. Rendering still needs normal sharing, exact wall-decoration offsets/occlusion, roofs/bridges/plane scene semantics and more terrain categories. Neutral native floor hue/light offsets and exact cache icon positions are documented deviations from original random variation/jitter. Native ground textures use full sampling while the recorded Java reference uses low-memory HSL fallback; do not claim exact visual parity. Minimap destination/hint/ground-item/player dots and click routing remain incomplete.

Keep advancing the roadmap without routine permission stops. This is a resumable build checkpoint, not a declaration of overall completion; no scheduled automation was created.

## Manual startup correction (October 20)

Port43594 was occupied by a different game server. Previous play/test wrappers did not start this project's engine and defaulted to that occupied port. Added start_local_server.bat for the existing production engine (game43595/web8889/management8899); play_client.bat and test_client.bat now pass --port43595, with explicit later arguments able to override it. Read MANUAL-TESTING.md. Start the server, wait for World ready, then launch play; test compiles first. Windows launchers automatically supply the existing local test account; the raw executable requires both credential variables. No finished native login screen is claimed.

Login diagnostics now distinguish connect, seed, send and response failures and print the endpoint; no automatic offline fallback. Corrected controls: WASD preview only, Escape first closes interfaces/menu; live public chat remains unsupported. Actual production startup through the new server batch and live SDL sessions through BOTH play/test batches succeeded using a disposable account. Latest binary capture/log: manual-test-launch.bmp.png / manual-test-launch.log. Closed-port43596 failure returned1 with connection help (manual-network-failure.log). Rebuilt loopback transport checks pass (manual-transport.log). test_client's compiler exposed an indices-maybe-uninitialized warning in route encoding: an explicit empty-turn guard removes the warning, and route transport checks still pass. The first warning log is preserved as manual-initial-compiler-warning.log; the current test-launch build has no warnings. The prior full32-program suite remains prior evidence; this change reran relevant transport checks and both actual launchers, not the full suite.

The manual verification production server remains running on43595 for immediate user testing; launcher PID is recorded in manual-server-launcher.pid, server PID11324 at verification. Do not start a duplicate while it is running, and never terminate the unrelated43594 server. For later fresh sessions use the documented foreground server batch and normal Ctrl+C shutdown. No .env/server rules/save resets. All files stayed inside the project; process metadata identified the conflicting listener without opening its directory.

A smaller-model read-only audit was explicitly authorized by the user and identified inaccurate controls, then checked the updated guide/launchers. It made no edits or process operations. Gameplay roadmap and the ground-item next dependency remain unchanged.

## Shared layered-model flicker correction (October 20)

Latest native SHA-256: `21BF30F48D84A5ED5EA5A4E3202EE72C479B2B3A79AF8A7A3D906D682D7DADC4`. See RENDER-FLICKER-FIX.md and client-c/verification/completion/runs/2026-10-20-flicker/comparison.html.

The tutorial-island white carpet has authentic black cache geometry, with detail/base faces exactly coplanar and different face priorities. Native rendering ignored cache priorities and rounded camera vertices before shared depth testing, so overlapping triangles could alternately hide their decoration. Implemented shared original Model.draw priority grouping (including10/11 interleaving) for world and item-icon paths, preserved source constant priorities in composite models, and used one continuous camera transform plus double reciprocal depth for ground/models. Relative64-ULP depth tolerance handles numerical equality; no geometric offset, carpet-ID exception, asset editing or replacement texture was added. This is a native depth-renderer precision correction; exact original integer-rasterizer image parity is not claimed.

User-authorized GPT-6 Luna independently wrote tests/test_coplanar_visibility.c. Actual cache model1067 detail-only mask survives full rendering across28 views; reversed synthetic face order, composite priorities and nearer/farther model occlusion also pass. Separate actual-cache1110/1134 masks pass80 views /22,383 pixels. Preserved pre-fix sources fail the same actual-cache check. Parent independently reproduced the old-source failure after a warning-free successful baseline build; baseline-reproduction-command/result/build/run in tester/ are authoritative. Historical initial baseline-build.log is a mixed-source link failure, not that successful reproduction.

Fresh full-suite.log: all33 programs completed, zero warnings. Optional standalone live-login scenario explicitly skips without opt-in credentials; current real live-client evidence is separate. Actual SDL before/after captures have matching world camera and player position (matched-camera.json). fixed-reset-walk.log and40 PNG frames record two click-to-walk routes, incoming player movement and15 camera positions; complete decoration remains visible in inspected movement captures. The location fixture uses the existing developer teleport on a disposable account; no natural-travel/tutorial-completion claim. Original accounts and server rules were not reset. Local production server remains on43595 for user testing.

Current executable and updated checkpoint ZIP include the shared fix, tests, evidence, this handoff and roadmap. Both handoffs synchronized. Close/reopen play_client.bat to load the rebuilt executable. Full functionality, broader original/native parity, effects, occlusion/roof semantics and ground-item next dependency remain as documented above. Delegated tests cover this bug class and three assets, not an exhaustive tour of every model.


## Historical login response and account prompt correction (October 20; prompt superseded below)

The Windows play and test launchers now invoke `play_client.ps1`. If both `RS2_USERNAME` and `RS2_PASSWORD` are not already supplied, it opens the Windows credential prompt for the local account; cancelling exits before the client starts. The password is passed through the launcher process environment only for that client run and is removed afterward. `--offline` skips the prompt because it does not log in. The wrapper test completed an actual cache-backed SDL offline preview with exit code 0 and a captured frame. The native executable was rebuilt warning-free after the diagnostic update.

For this revision-225 server/client protocol, login code 5 means that the same account is already logged in or its previous logout is pending. Close another client for that account, wait for logout to finish, and retry, or use another valid account. It does not mean the password is wrong. Invalid credentials are client code 3. This mapping is verified against `client-c/include/net/protocol.h`, `engine/src/engine/World.ts` and `engine/src/server/login/LoginServer.ts`. The launcher now explains code 5 separately. No login, account, or save was changed for this verification.


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


## Audio Synthesizer, MIDI Engine & Interactive Dialogue Enhancements (October 20)

Completed revision-225 software sound synthesizer, native Windows MCI MIDI engine, dynamic player head composite model rasterization, item dragging, quantity count dialogs, and recursive scrollbars.

1. **Option 4: Audio & MIDI Engine**:
   - Software wave synth parsing `sounds.jag` and `sounds.dat` at 22,050 Hz 8-bit unsigned mono PCM, verified bit-for-bit with revision-225 synthesizer logic.
   - Client delay queue decrementing at authentic 50Hz client tick rate (`audio_queue_sound`, `audio_tick`).
   - Native Windows MCI General MIDI subsystem (`-lwinmm`), decompressing cache songs (`engine/data/pack/client/songs/*.mid`) using headless BZip2 decompression, volume scaling, and auto-looping.
   - Packet opcodes decoded: Opcode 12 (`SYNTH_SOUND` with client delay ticks), Opcode 54 (`MIDI_SONG`), Opcode 212 (`MIDI_JINGLE`).

2. **Option 3: Interactive Dialogue & Interface Enhancements**:
   - Packet 197 (`IF_SETPLAYERHEAD`): composited 3D player head model dynamically built from identity configs (`idk.jag`), worn equipment (`obj.jag` for hats/helmets), beard suppression, and recoloring tables.
   - Packet 159 (`INV_BUTTOND`): Item dragging with authentic threshold (>5px) and tick cycle (>=5 cycles / 100ms) gating, semi-transparent (alpha 128) floating icon rendering, and local slot swapping.
   - Quantity Input Dialog: Opcode 243 (`P_COUNTDIALOG`) activating chatbox numeric amount entry, digit filtering, and Opcode 237 (`RESUME_P_COUNTDIALOG`) 4-byte count submission upon Enter.
   - Recursive scrollbars: component click, thumb drag clamping, and mousewheel event propagation across nested container components.

3. **Verification**:
   - 35 test programs compile warning-free and pass with 100% exit 0 (`make test`).
   - Three dedicated tests: `test_audio.c` (wave synthesis & MIDI), `test_dialogue_head.c` (unhelmeted & helmeted head model compositing and rasterization), `test_bank_interface.c` (slot picking, drag lifecycle & swap, quantity dialog, scrollbar clamping).
   - Evidence directory: `client-c/verification/completion/runs/2026-10-20-audio-dialogue/` containing `build.log`, `regression.log`, `results.json`, `README.md`, `source-review.diff`, and captured `live.bmp`/`live.png`.


## Tutorial Island Mechanics & Interface Interactivity (October 20)

Addressed Tutorial Island unique mechanics vs. mainland game states to resolve UI lockouts and unmapped sidebar tabs:

1. **Tutorial Interface & Tab Protocols**:
   - Server dynamically unmaps sidebar tabs on Tutorial Island (`if_settab(null, ...)`). Added `DEFAULT_TAB_INTERFACE_ID` fallback table mapping authentic defaults (e.g. inventory 3213, stats 3917, quest 638, combat 2423, music 962) to prevent permanent tab lockouts when clicking between tabs.
   - Opcode 185 (`TUT_OPEN`): sticky tutorial chatbox interface persistence (`sticky_chat_interface_id`).
   - Opcode 126 (`TUT_FLASH`): authentic blinking tab icons (`media_draw_tabs_flashing`) rendering icon on `loop_cycle % 20 < 10` cycle.
   - Removed arbitrary button code rejection in `client_actions.c` (`client_code != 0 && client_code != 205` check eliminated), ensuring character creation buttons and general interface buttons dispatch cleanly.
   - Developer commands added to chatbox parser: `::tabs` / `::unlocktabs` (toggles tab lock), `::mainland` / `::lumby` (teleports to Lumbridge courtyard), `::skip` (completes tutorial varps).


## Automatic Fresh Account Reset & Flashing Hint Arrows (Packet 25) (October 20)

Implemented automatic test account reset on launcher start and authentic revision-225 Hint Arrow rendering across Tutorial Island and quest objectives:

1. **Automatic Fresh Account Preflight**:
   - `play_client.ps1` detects and automatically purges any existing save file (`engine/data/players/*/$env:RS2_USERNAME.sav`) before starting the client executable.
   - Guarantees each client launch initializes a 100% fresh account at step 0 of Tutorial Island with the character design screen displayed.

2. **Rev 225 Hint Arrow (Packet 25)**:
   - Added `Pix32 *headicons[20];` in `Media` struct (`media.h`) and loaded sub-sprites from `media.jag` in `media.c`. Confirmed `headicons[2]` is the authentic 25x25 yellow downward-pointing hint arrow.
   - Handled Packet 25 (`HINT_ARROW`, 6 bytes) in `sdl2_main.c`:
     - **Type 1 (NPC)**: Decodes target NPC `nid`/slot. Automatically samples model height to position the arrow directly above the NPC's head.
     - **Types 2–6 (Tile / Scenery / Doors / Nodes)**: Decodes world coordinates `(hintTileX, hintTileZ)` and elevation height. Applies authentic directional tile offsets (type 2: center 64,64; type 3: west 0,64; type 4: east 128,64; type 5: south 64,0; type 6: north 64,128) and scales elevation (`hintHeight * 2`).
     - **Type 10 (Player)**: Decodes target player slot.
     - **Type -1 / 255 (`HINT_STOP`)**: Clears active hint arrow.
   - Real-time 3D-to-screen perspective projection (`project_from_point`) matching `client.java` arithmetic bit-for-bit.
   - Blinking at authentic 20-tick cycle (`rendered_frames % 20 < 10`).
   - Drawn at `(projectX - 12, projectY - 28)` inside the viewport clipping bounds.

3. **Verification**:
   - `test_sprites.c` verified loading `headicons[2]` (25x25) and rendering into the framebuffer.
   - All 35 test suites compile and pass 100% (`make test`) with zero warnings.
   - Packaged checkpoint: `hint-arrow-fresh-start-checkpoint.zip` (SHA256: `8D7A75C169770653E355B28E9AEE35C903EDA26903B26A6F233D125589D8A13D`).


