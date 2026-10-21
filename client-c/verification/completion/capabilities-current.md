# Current capability evidence — October 20, 2026

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
