# C5 plan: original revision-225 rendering during normal play

Date: October 22, 2026. Owner: Sol, current Codex engineering chat.
Status: candidate integration implemented; qualification in progress. Latest implementation/results/limits: runs/2026-10-22-scene-runtime-sol/README.md. The planning checkpoint below is historical; fresh work after the user said go is recorded in that README.

## User-visible outcome

Ordinary play uses the original revision-225 scene renderer: terrain, scenery, players, NPCs, ground items and effects draw in original order, with correct hover targets, walking clicks and interface previews through map changes.

## Existing evidence

Evidence directory: client-c/verification/completion/runs/2026-10-22-scene-runtime-sol/.

- Separate rs2client225.exe and rs2replay225.exe targets exist; the normal client has not been switched.
- runtime-overlay.log: all 12 world frames and 12 interleaved previews match; 4,104,384 framebuffer/guard words, zero differences, 6,599 model comparisons, matching selection lists and roof levels. Includes bounded scenery removal/replacement, a scene reload and a player location overlay.
- registered-suite.log ends ALL TESTS PASSED; registered-suite.exit is 0. The optional credential-dependent live login was skipped. This is existing evidence, not a new run during planning.
- offline-final.log/.exit and pointer.log/.exit record successful ten-frame offline candidate captures. Offline fixture scenes do not establish live gameplay.
- Historical offline replay fails at line 14 (T versus F); compatibility versus current legacy code remains unclassified. Historical server-session replay fails at line 18 (NC versus F); RS2_PASSWORD was unavailable at that attempt. Neither replay gate passes.
- Installed rs2client.exe SHA-256 rechecked during planning: A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4.

## Work sequence

1. Preserve the candidate and source identities, baseline snapshots and current results in the evidence directory. Compare the existing pointer and ordinary captures to check that scripted target discovery leaves scene pixels unchanged.
2. Qualify actual input through the candidate: ground walking, NPC/scenery/item hover and right-click menus, option selection and pointer-leave dismissal. Check viewport coordinate translation, hit ordering and resulting route/action packets against the bundled original. Use existing scripted input and capture tools.
3. Qualify lifecycle transitions: plane/roof changes, bridge views, scenery replacement, cache clearing and a region rebuild. Check stale model references, item-stack elevation and temporary entity removal. Extend only a measured failing scenario; retain original-source evidence.
4. Resolve replay compatibility. Compare the historical clock failure with a separately built current legacy replay using consistent structure layouts; record a fresh candidate session and replay it strictly. Measure startup RNG call order and protocol output rather than masking differences. Use the existing saved server-session replay when its password is securely available through RS2_PASSWORD; never write credentials to evidence.
5. Run the existing C2/C3/C4 focused comparisons against the final DLL and the affected runtime/replay checks. Reuse the registered suite after relevant implementation changes; do not create a broad new testing project. Perform one bounded live interaction/region-change scenario with save snapshots and confirmed server ownership.
6. Once the playable gates pass, preserve the old executable, build and install the qualified renderer as the normal client, record hashes and update the queue, both handoffs, progress and source map. If a gate fails, keep the separate candidate and document the first reproducible mismatch.

## Completion criteria

- Existing original-scene equality and ordered picking remain exact.
- Normal input selects the intended target, emits original action/route packets and preserves UI rendering.
- Region/plane/scene changes leave no stale geometry or borrowed model references.
- Fresh deterministic replay passes; historical incompatibilities are explicitly classified, and required original state/packet/frame comparisons are recorded.
- Bounded live play passes; normal executable changes only after qualification. No claim of exhaustive world or long-session parity.

## Subsequent feature

After C5 closes, follow Phase D with session recovery and idle timers (idleNetCycles/reconnect and AFK behavior). First inspect the current coverage map and bundled client source for remaining gaps. This is queued planning, not an active implementation claim.
