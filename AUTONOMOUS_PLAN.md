# Autonomous completion plan: functional and authentic revision-225 client

Prepared October 20, 2026. Status: ACTIVE; execution is recorded in
`client-c/verification/completion/progress.md`. This replaces isolated visual milestones as the execution roadmap.
The previous executable and verification remain a historical delivery baseline.

## Objective and boundaries

Produce a usable native PC client for the bundled May-2004 revision-225 game:
ordinary gameplay works through the real server, and presentation and interaction
match the bundled reference client. Complete client behavior against existing
server/content; do not invent replacement gameplay or change server rules to hide
client defects. Windows is the first acceptance target; retain C99/SDL2 portability
and verify Linux when a toolchain is available. Console work remains deferred.

Read/write/cleanup must remain within `2004scape-LostCityRS`. Keep build temporary
files and evidence within this tree. Use existing tools and dependencies; if
something requires access outside the allowed tree, find an in-tree alternative
or record the specific external requirement instead of crossing the boundary.
Preserve existing work, cache, account saves and nested repository histories.
Use disposable test accounts/world state within the project for gameplay checks.
No deployments, account messages, purchases, destructive resets or external setup
are implied by this plan.

The reported solid ground, minimap holes, combat-style and inventory failures
belong to the broader systems below. Do not start a separate patch queue for them.

## Source of truth and evidence standard

1. `javaclient/src/main/java`: primary reference for client behavior and rendering.
2. `webclient/src`: secondary implementation for resolving intent and comparisons.
3. `engine` protocol codecs/handlers and `content`: authoritative wire format,
   definitions, scripts, interfaces, actions and gameplay rules.

Inventory discrepancies between references explicitly. Prefer the revision-225
behavior supported by the bundled content and server over convenient approximations.
Run the bundled Java client against the same cache/server where feasible; if its
launch cannot be completed within the directory boundary, record the missing
comparison and use source/data evidence without calling it visual parity.

A loaded cache, visible widget, sent packet or green unit test is not proof that
a player-facing feature works. A scenario passes only when real input reaches the
production action/packet path, the server accepts it, the production receive path
applies the reply, and the client shows the correct result. Capture outgoing
operation/arguments, relevant replies/state and client-rendered evidence. Redact
credentials/session secrets from logs. Separate unit, integration, actual-client,
reference-comparison and live-server evidence; list skips as skips.

For visuals, compare the same location, plane, camera, detail mode, cache and scene
state. Distinguish original HSL-shaded underlays from textured overlays: authenticity
must not be approximated by adding textures where the reference uses solid shading.
Validate the minimap independently against reference map symbols, shaped floors,
rotation/masks, plane rules and loaded scene extent. Classify legitimate voids
separately from missing data, bad colors, sampling errors or mask defects.

## Execution sequence and acceptance gates

| Phase | System-wide scope | Required result before moving on |
| --- | --- | --- |
| 0. Establish truthful coverage | Inventory reference features, client handlers, outgoing operations, interface scripts, rendering paths, test quality and demonstration-only state. Build repeatable client input, packet and screenshot scenarios. | A traceable capability matrix with reference locations and statuses; reproducible baseline failures; separate offline/demo and live modes; working reference comparison or an explicit evidence limitation. |
| 1. Make session and world state reliable | Audit every server opcode/length and client operation against codecs; incremental nonblocking reads, capacity checks, disconnect/reconnect, login/logout, entity masks, NPC/player synchronization, movement, region origin and rebuild lifecycle. Extract shared production handlers where needed to test the real paths. | Fragmented and coalesced packets decode correctly; movement and interactions use correct coordinates; multiple real entities synchronize; region crossings and reconnects do not corrupt state or freeze the UI. |
| 2. Complete the interaction and interface engine | Replace numeric action-range shortcuts with structured actions carrying operation, target, component, slot and item IDs. Unify left click/default action, right click, hit testing, scrolling, dragging, selection, cancellation and path-to-target. Audit all component types, scripts, varps, dynamic text/models, tab changes, modals and dialogs. | The same dispatcher serves world and interface actions. Correct arguments survive from hit test to wire packet. Server-driven interfaces update without hardcoded substitutes. Real-client input scenarios cover nested/scrolled widgets and menus. |
| 3. Complete player state and item workflows | Inventory and equipment as one lifecycle: correct slot operations, equip/unequip, appearance, stats, quantity prompts, rearrangement where supported, use/drop/examine, item-on-item and item-on-target. Include combat interfaces, weapon-specific styles, prayer/magic selection and authoritative varp/config updates. | Equip two different items from different slots; appearance, equipment, inventory and combat controls update from server replies; unequip and relog preserve the expected state. Style and selection changes affect the correct server state and visible interface. No fabricated success messages. |
| 4. Reconstruct world presentation faithfully | Audit terrain decoding, all overlay shapes/rotations, HSL/lightness, underlay blending, material/texture selection and animation; scenery shapes, walls/decorations, bridges, planes, roofs, visibility, priority/alpha behavior and animation. Rebuild the minimap from the same scene semantics, including wall/door marks, mapscene/mapfunction sprites, dots, hints, destination, masking and click coordinates. | Reference comparisons across town, building interior, water, slopes, stairs/bridge and region boundaries. No unexplained holes or wrong materials. Scene and minimap agree on location/plane and respond correctly to rotations, rebuilds and movement. Every remaining visual deviation has a named cause and evidence. |
| 5. Complete ordinary world interactions | All supported object, NPC, ground-item and player operation families; correct routing, reach checks, approach/retry/cancel behavior; ground-item stacks and scene changes; dialogue choices and continue, doors, ladders/stairs, containers and teleports. | Actual client can navigate to and operate targets using the same behavior as the reference, receive the resulting world/interface changes and continue playing after region/plane transitions. Exercise more than the demonstration NPC. |
| 6. Complete connected gameplay loops | Banking, shops, trading where supported, resource gathering/processing, quest and dialogue progression, melee/ranged/magic combat, damage/hitmarks/health bars, XP/stats, death/respawn, run energy and social/chat/settings flows. Implement missing client-side presentation and operation handling; retain existing server rules. | Representative sessions complete each supported loop end to end. A session combines bank withdrawal, equipment/style changes, travel, target interaction/combat or skilling, rewards, return/deposit and relog persistence. Exchange flows use separate disposable test accounts. |
| 7. Audit remaining revision behavior and release | Reconcile the full capability matrix: remaining packets/interfaces, friends/ignore/private/public chat, report/logout flows, settings, sounds/music, effects/projectiles/spot animations, camera, timing and input conventions. Audit memory ownership, rebuild lifetime, performance and error handling. | No unclassified reference feature remains. Essential gameplay and reference comparison gates pass; repeated rebuild/reconnect/travel sessions remain stable; unsupported or unverified behavior is explicitly listed rather than hidden by a blanket completion claim. A reproducible build and evidence bundle are delivered. |

Phases are dependency gates, not a ban on necessary cross-cutting work. Fix the
smallest underlying system needed to make a complete scenario pass. Recheck every
scenario affected by that system. If a later phase reveals a faulty earlier
assumption, reopen that gate and resolve it before expanding scope.

## Start-of-run work products

Create these before implementing fixes on the next authorized execution:

- `client-c/verification/completion/capabilities.md`: feature/operation, reference
  source, production path, scenario, unit/integration/client/reference evidence,
  state (unverified/failing/partial/verified), remaining gaps and dependencies.
- `client-c/verification/completion/scenarios.md`: deterministic setup, input,
  expected packet arguments, authoritative state and visible outcome. Each scenario
  must be runnable without manually editing source or assigning the expected state.
- `client-c/verification/completion/progress.md`: active phase, current failing
  scenario, decisions, completed evidence, blockers and exact next action.
- `client-c/verification/completion/runs/<run-id>/`: bounded logs, reference/native
  captures, relevant state traces, build hash and a manifest linking them to scenarios.

Initial evidence seeds, not a finished audit: the SDL event loop dispatches item
actions with a fixed slot/component; some interface actions encode IDs inside
numeric ranges; the minimap currently fills each tile with one color; some network
synchronization tests directly assign UI state. Existing tests and six-view captures
remain useful evidence for their limited assertions, not proof of full functionality.

## Minimum scenario coverage

- Fresh login, saved login, logout/reconnect, slow/fragmented network input and
  graceful server disconnect; map crossings in both directions and plane changes.
- Left/default and right-click actions; scrolling/dragging; use/cancel/target
  selection; nested dialogs, quantity prompts and every populated sidebar category.
- Multiple inventory slots and quantities, equipment changes with different weapon
  interfaces, selected combat style, stat/appearance updates, banking/shop cycles
  and persistence after relogging.
- Object/NPC/item/player actions, blocked and reachable paths, changing or removed
  targets, occupied tiles, ground-item updates, stairs/doors and scene transitions.
- Real combat and skilling loops, XP/level changes, damage/effects, death/respawn,
  chat/social/settings and audio appropriate to supported content.
- Native/reference view pairs for terrain/material categories, shaped floors,
  lighting, walls/decorations, upper planes, roofs/bridges, transparent/alpha
  geometry, entity occlusion and minimap content. Include boundary and rotating views.
- Sustained play plus repeated rebuilds/reconnects, bounded memory growth, stable
  frame/tick timing, no unexplained rendering glitches, crashes or input stalls.

Select representative scenarios from actual bundled content during phase 0;
record exact IDs/locations from references instead of guessing them here. Broaden
coverage from the capability matrix rather than repeatedly polishing one location.

## Autonomous working agreement

Once execution is requested, continue through this roadmap without asking for
permission at phase boundaries or stopping after each isolated bug. Routine source
edits, refactors, local rebuilds, in-project verification servers/test accounts,
client launches, regressions, packaging and handoff updates belong to that work.
Keep progress messages brief and explain outcomes and remaining uncertainty.

Use this repeating work loop:

1. Select the highest dependency scenario that is failing or unverified.
2. Reproduce it through the actual client and compare with the reference/data.
3. Trace the complete input/action/network/state/render chain and correct its cause.
4. Add a meaningful regression through the production path; avoid tests that assign
   the desired state directly or repeat the implementation's assumptions.
5. Verify the scenario in the native client against a local server, then rerun the
   relevant earlier scenarios. Run the full suite for integrated release checkpoints.
6. Record evidence, update the matrix/ledger and both handoffs, then continue to
   the next dependency. Checkpoints are resumable progress, not a reason to stop.

If a scenario is blocked, document the concrete failure, try safe in-scope
alternatives and continue independent work. Ask only when essential missing
information, a destructive action or a genuinely external dependency prevents
further useful in-scope progress. Do not ask routine implementation preferences or
repeat existing authorization. Preserve the directory boundary throughout.

If the session is interrupted, leave a runnable checkpoint and exact continuation
instructions. On resumption, read the ledger, inspect current files/build evidence
and continue the next failing scenario; do not restart the audit or repeat finished
work. This plan does not itself create a scheduled/background automation.

## Delivery and meaning of completion

At integrated checkpoints deliver the native binary, required in-project runtime
files, launch instructions, source changes, SHA-256 and evidence manifest; update
both handoffs identically. Label the artifact as a checkpoint until the final gates
are met. Keep the last known good runnable build while changing shared systems.
Avoid large duplicate captures; retain lossless PNGs and bounded traces. Clean up
only task-generated disposable files and servers.

Do not call the client complete because all currently written tests pass. Final
acceptance requires the capability matrix to cover the bundled reference scope,
ordinary player sessions to work through the live server, the visual comparison
matrix to pass with no unexplained failures, and stability checks to pass. Known
unsupported features, skips and unverified claims must remain explicit. If those
conditions cannot be met within a run, deliver an accurately labeled checkpoint
and continue when the session resumes; do not redefine the objective downward.

## Shared layered-model flicker correction (October 20)

Latest native SHA-256: `9071CE61356D92E2BBBE75DCBB0B32C2101ED754208E4FCF6DD080A8BE3A294C`. See RENDER-FLICKER-FIX.md and client-c/verification/completion/runs/2026-10-20-flicker/comparison.html.

The tutorial-island white carpet has authentic black cache geometry, with detail/base faces exactly coplanar and different face priorities. Native rendering ignored cache priorities and rounded camera vertices before shared depth testing, so overlapping triangles could alternately hide their decoration. Implemented shared original Model.draw priority grouping (including10/11 interleaving) for world and item-icon paths, preserved source constant priorities in composite models, and used one continuous camera transform plus double reciprocal depth for ground/models. Relative64-ULP depth tolerance handles numerical equality; no geometric offset, carpet-ID exception, asset editing or replacement texture was added. This is a native depth-renderer precision correction; exact original integer-rasterizer image parity is not claimed.

User-authorized GPT-6 Luna independently wrote tests/test_coplanar_visibility.c. Actual cache model1067 detail-only mask survives full rendering across28 views; reversed synthetic face order, composite priorities and nearer/farther model occlusion also pass. Separate actual-cache1110/1134 masks pass80 views /22,383 pixels. Preserved pre-fix sources fail the same actual-cache check. Parent independently reproduced the old-source failure after a warning-free successful baseline build; baseline-reproduction-command/result/build/run in tester/ are authoritative. Historical initial baseline-build.log is a mixed-source link failure, not that successful reproduction.

Fresh full-suite.log: all33 programs completed, zero warnings. Optional standalone live-login scenario explicitly skips without opt-in credentials; current real live-client evidence is separate. Actual SDL before/after captures have matching world camera and player position (matched-camera.json). fixed-reset-walk.log and40 PNG frames record two click-to-walk routes, incoming player movement and15 camera positions; complete decoration remains visible in inspected movement captures. The location fixture uses the existing developer teleport on a disposable account; no natural-travel/tutorial-completion claim. Original accounts and server rules were not reset. Local production server remains on43595 for user testing.

Current executable and updated checkpoint ZIP include the shared fix, tests, evidence, this handoff and roadmap. Both handoffs synchronized. Close/reopen play_client.bat to load the rebuilt executable. Full functionality, broader original/native parity, effects, occlusion/roof semantics and ground-item next dependency remain as documented above. Delegated tests cover this bug class and three assets, not an exhaustive tour of every model.
