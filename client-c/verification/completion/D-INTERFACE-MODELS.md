# Revision225 interface-model packets — October 24, 2026

**Accepted for controlled original/cache/native/candidate comparisons.** Added46 IF_SETOBJECT,87 IF_SETMODEL and103 IF_SETRECOL through the production component delegate.

## Source behavior and implementation

Unchanged client.java read46/87/103,ObjType.getInterfaceModel,Component.getModel and Model define behavior. Object46 selects the original quantity50 stack model while retaining the base item's interface angles and zoom*100/divisor. Raw87 creates a distinct unlit model. Recolour103 changes the current model's base colours;null model is a no-op. Object models preserve source50-entry LRU sharing,aliases between components and referenced models surviving cache eviction. Heap-owned packet models are collected when neither cached nor referenced and cleared on client shutdown;Component size is unchanged.

Original lighting discards base colours when no texture face needs them. Native tracks this logical state while retaining its renderer-required colour storage. Source recolour would dereference discarded colours on a nonempty mesh;native rejects that malformed operation safely. An empty-face model keeps source's no-op behavior. Ordinary raw model recolouring and textured retained-colour behavior use the stored base arrays. Existing component-config ownership contains this UI model cache;interactions with the separate existing item-icon cache are not qualified by this task.

Raw models are posed/lit for viewport and tab model rendering;colourless already-lit models are reused when no animation transform is required. Viewport interfaces now draw in a local512x334 surface,matching the source viewport origin before compositing into the desktop frame. This task's preview corpus uses no animation transforms;arbitrary animation/colourless combinations and cross-consumer icon-cache behavior remain outside acceptance.

## Evidence

- **378 actual original packet states** on real cached model widgets126/127/128. Complete vertex/face/texture arrays,nullable colours/shades/priorities/alpha,angles/zoom and all pairwise model aliases compare. Includes40 raw model selections and recolours,100 object cache entries/eviction,shared/replaced references,divisors1..65535 and explicit coin/arrow/feather/rune/note quantity variants.
- **37 complete512x334 previews,6,327,296 pixels**,with source palette and matching black background,plus guards. Source invokes actual read handlers,Component.getModel and Model.drawSimple. Accepted geometry semantics include logical discarded colours;unused skin-label/internal cache structures are not a deep-model-state claim.
- Actual candidate matches all378 ISAAC-framed packets/full incoming cipher/mesh/cache states. All37 actual viewport foreground previews match. Native full frames qualify the background as well;candidate foreground comparisons use an empty scene with the same black underlay. Cache stays<=50 entries and<=53 owned models for the three-widget corpus;cleanup reaches zero.
- Fault controls:4 manager/node allocation failures,21 wrong lengths,invalid component,zero divisor,discarded-colour rejection,empty-face no-op and cleanup. These are the stated publication safeguards,not exhaustive allocation-failure testing inside every existing model-loader allocation.
- Changed geometry and pixel expectation copies fail exit3.

## Retained failures and limits

Native1/2 caught unused flat-face shade B/C values populated where original Java leaves zero. Normal shade allocation is now zeroed and flat unused slots remain zero;native3 onward and current full suite pass without changing source expectations. Initial360 cases were extended to378 for explicit stack variants. A stale native4 executable exposed a missing helper-header build dependency;explicit dependencies now rebuild all three extended registered tests correctly.

Initial preview captures were128x128 on a coloured background. Actual viewport comparisons exposed coordinate/background assumptions,including transparent faces. The corpus was expanded to the real512x334 viewport with a matching zero background;earlier captures and failed logs remain. Both earlier native crop comparisons and final full viewport comparisons matched their actual original captures. The viewport rendering path was corrected to source-local coordinates and model preparation;no expected pixels were edited by hand. All final captures regenerate exactly.

This accepts the defined component packet/cache/preview behavior,not every interface script/model animation,icon-cache interleaving,arbitrary textured preview,inventory flow or final UI qualification. Existing populated scene renderer limits are unchanged by these previews.

## Combined evidence

Package: runs/2026-10-24-receive-batch-sol/. Final acceptance uses suite-final2 (all registered tests,current separate candidate/replay and opt-in diagnostics),strict current builds,reproduce2 and nine regression-final2 families. Original fixtures regenerate byte-identically. Seven separately altered camera-state/RNG,model-geometry/pixel,reset-primary,multiway-state/pixel copies reject exit3. See completion-audit.json for checked exits and stable identities. Existing registered tests were extended;no new TEST_BINS. Original Java/jar,installed executable and118 save hashes are preserved. Production43595 remains absent as previously observed;no server start/stop/restart,production login,installation,commit,deletion or delegation.

Reproduce from client-c with the bundled PowerShell7 after building the three registered tests and four opt-in controls:

    pwsh -NoProfile -ExecutionPolicy Bypass -File verification/completion/tools/receive_batch_parity.ps1 -OutDir <fresh-output-directory>

The verifier compiles the three new reference captures against the unchanged original jar,regenerates four fixtures,checks hashes,runs native/candidate/fault controls and restores all four environment variables. It does not build or install automatically. Candidate binaries,source/before snapshots,task-only diff and identities are retained. Credential-gated live login skipped;CRC passed. Historical C5/recovery whole-frame corpora were not rerun. Source-route totals are64 present/7 missing out of71;these are not correctness or completion percentages. PhaseD continues;PhaseE pending.
