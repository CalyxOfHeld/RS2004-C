# C5 candidate acceptance — October 23, 2026

**Result: bounded revision225 candidate qualification passes.** The normal installed executable remains unchanged. This completes AUTONOMOUS-PLAN.md through its acceptance audit; deployment and Phase D are separate next work.

## Accepted evidence

All four gates below use the same final implementation. The bridge recording was made just before the hint-height correction; the final replay binary strictly reproduces its native hashes and all original frames, without allowing framebuffer differences.

| Gate | Strict replay | Original screens | Observed behavior and evidence |
| --- | ---: | ---: | --- |
| Integrated play, seed226 | 3,100 frames | 3,099/3,099 | Drawers350->351; region/plane rebuilds; minimap walking and bridge views; two co-located bankers; Talk-to and animated portrait; fountain. runs/2026-10-23-c5-integrated-sol/release/ |
| Natural stairs, seed225 | 1,200 frames | 1,199/1,199 | Actual1738 Climb-up and1739 Climb-down, plane0->1->0 without a teleport between those actions. runs/2026-10-23-stairs-final-sol/release/ |
| Item input and menu dismissal, seed225 | 1,000 frames | 999/999 | Knife946 at cache-defined3224,3202; menu closes when pointer leaves at520, reopens, Take sends, server OBJ_DEL49 confirms removal and player reaches item tile. runs/2026-10-23-ground-item-sol/release/ |
| Full bridge crossing and return | 1,500 frames | 1,499/1,499 | All bridge columns3231..3238 on rows3261/3262 traversed in both directions; returns to3230,3261. runs/2026-10-23-bridge-final-sol/traversal-proof.json and hint-recheck/ |

Total: **6,796 exact original game screens /169,900 regional hashes**, with original outgoing bytes and inbound consumption matching. Integrated release trace has **30,814 original/native NPC states, zero differences**. Raw local-state differences during hidden initial loading remain in release/actor-comparison.json; they were not removed or normalized, and every loading screen is included in image equality.

## Completion criteria audit

| Requirement from C5-PLAYABLE-PLAN.md and AUTONOMOUS-PLAN.md | Result | Authoritative proof |
| --- | --- | --- |
| Original scene equality and ordered picking remain exact | PASS for the accepted corpus | Fresh rebuilt comparison DLL: C2/C3/C4/runtime all exit0, exact original frames/callbacks/model comparisons. current-regression-results.json pins the DLL identity. |
| Normal input selects targets, sends original route/action packets and preserves UI | PASS | Three release recordings above; actual scenery replacement, NPC dialog, ground-item removal, ordinary pointer menus/dismissal; protocol reports MATCH. |
| Region/plane/scene changes preserve geometry/model lifetime | PASS for exercised transitions | Integrated rebuilds, full bridge control, natural up/down; all original frames exact. Runtime12 worlds+12 previews and existing cache-lifetime/effect/model checks pass. |
| Fresh deterministic replay; historical incompatibilities classified | PASS | Every accepted recording strictly replays. Historical T-versus-F clock failure was reproduced by the shared legacy baseline (2026-10-22-scene-runtime-sol/README.md). Old login recordings with unavailable passwords were not reused and are excluded from acceptance. |
| Bounded live play; preserve installed client, server and saves | PASS | Server43595/PID17880 reused;17 starting-save inventories show zero changes; installed client hash unchanged. |
| Final builds/checks and reviewable deliverables | PASS | release-suite.exit0, no compiler warning/error lines, legacy translation compiled with Werror; current C2/C3/C4/runtime exits0 and DLL hash matches packaged artifact. Candidate/source/hash/evidence package is runs/2026-10-23-c5-acceptance-sol/. |
| Synchronized queue, both handoffs, progress and source map | Completed with this acceptance checkpoint | WORK-QUEUE.md, HANDOFF.md, CLAUDE-HANDOFF.md, progress.md and phaseC-source-map.md newest sections. |

The optional credential-gated registered live-login test is skipped; separate actual live login/protocol controls above passed. No new registered test program was added.

## Corrections qualified in this period

- Original chat portrait posing, lighting and chat-area clipping; used high-detail texture17/24 rotation and cache invalidation.
- Original menu-opening lifetime and current-plane scenery membership; combat-level label spacing/color thresholds.
- Minimap route timing after actor movement, then camera update; retained minimap during region loading.
- NPC removal/reappearance lifetime across one packet and multiple packets in the same game cycle. Retained failure had292 mismatched NPC states; corrected same-record trace has31,215/31,215 exact states.
- Original entity bounds reset before forced/ordinary movement, preserving locomotion clocks; default undrawn-NPC height0 for hint arrows; original negative-projection speech rejection.
- Sixteen existing compiler warnings removed by whitespace-only statement separation. Existing tests gained source-derived animation, plane-filter/duplicate, NPC lifetime and bounds controls.

Every measured failure and pre-fix source is retained. Adaptive comparisons explicitly allowing old native framebuffer differences are labeled separately; they do not replace the fresh strict gates above.

## Candidate identities

- Playable candidate SHA256: `EDFF641D3D7F3419EA09791859F3437C4C20EF43CD5AE03821651D9947ECCCF2`
- Replay candidate SHA256: `EED26238471A4BC6E5C259C4FC384EF554505FB24EED8A04D6E710FE6B2BE57B`
- Preserved normal executable SHA256: `A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4`

Full binary/source/reference identities: runs/2026-10-23-c5-acceptance-sol/binaries.json, sources.json and reference-identities.json. Candidate executables are copied under that directory's candidate/. Launch using client-c as the working directory to access the existing cache. No installation, server restart, save deletion, commit or delegation occurred.

## Scope limits and next work

This is bounded C5 acceptance, not exhaustive world or extended-session parity. Texture24 and low-detail animation have real-cache/source-formula controls; the live fountain validates the visible animated-texture path. Mixed player/NPC expansion has source-derived unit coverage; live overlap uses two bankers and the local player. Effects have bounded original runtime/model/sequence coverage rather than every live combat situation. Later revisions are untouched.

Older failed recordings remain historical evidence with their original binary identities. Some cannot be rerun because their process-only login passwords are unavailable; no passwords were saved. Reproduce with fresh disposable credentials and new output directories using commands.json and the existing parity tools.

Current preservation instructions supersede the older plan's normal-executable installation step. The candidate is ready for user review; switching the installed client requires separate direction. Phase D session recovery/idle behavior remains queued and has not started.
