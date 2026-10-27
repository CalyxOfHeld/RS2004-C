# Revision225 animation reset and multiway indicator — October 24, 2026

**Accepted for controlled original/cache/native/candidate comparisons.** Added136 RESET_ANIMS and254 SET_MULTIWAY through the production entity-control delegate.

## Source behavior and implementation

Original client.read136 sets only primarySeqId=-1 for every non-null entry of all2048 player and8192 NPC slots,including cached inactive entries. Native also updates its separate embedded local-player representation. It retains primary frame/cycle/delay/loop and secondary animation fields. It does not invent sequence restarts or clear entire actor state.

Original read254 stores the complete unsigned byte. Only value1 draws the multiway imageHeadicons[1]. Original drawWildyLevel boundaries determine whether the indicator is at viewport472,258 (wilderness or inside a duel-arena floor) or472,296 otherwise. Native uses the real cached icon and preserves clipping. Other wilderness/arena icons,text and gameplay rules are not ported or accepted by this indicator task.

## Evidence

- **128 original reset cases**,dense/sparse/null patterns across complete2048/8192 arrays and eight animation fields per non-null actor. Active actor counts are zero so cached inactive slots must still reset. Full local PathingEntity state is retained apart from primary sequence in native controls.
- **5,696 actual original packet254/draw3DEntityElements icon regions:**all256 byte values across representative regions,all64x64 duel-area tiles,and surface/underground wilderness boundary points. Every40x32 region is reconstructed and compared:7,290,880 pixels,plus canaries and source-derived y position. This isolates the multiway icon;other original overlays are outside the compared region/scope.
- Actual candidate:5,824 ISAAC-framed packets/full incoming cipher,128 reset packet-phase full-array observations and184 actual client_draw icon regions/235,520 pixels. The reset observer preserves the local actor's state immediately after the real handler,excluding subsequent ordinary animation-tick delay changes. It does not claim those later ticks leave delay unchanged.
- Five malformed guards and unknown-opcode nonmutation pass. Changed reset-primary,multiway-byte and icon-pixel copies reject exit3. Real client/cache load and isolated sockets;no production server/player.

All initial native1/candidate1 comparisons passed. Source expects only primary ID clearing and flag1 drawing;other byte values remain stored without an indicator. This is not combat/animation system completion or real multiway gameplay qualification.

## Combined evidence

Package: runs/2026-10-24-receive-batch-sol/. Final acceptance uses suite-final2 (all registered tests,current separate candidate/replay and opt-in diagnostics),strict current builds,reproduce2 and nine regression-final2 families. Original fixtures regenerate byte-identically. Seven separately altered camera-state/RNG,model-geometry/pixel,reset-primary,multiway-state/pixel copies reject exit3. See completion-audit.json for checked exits and stable identities. Existing registered tests were extended;no new TEST_BINS. Original Java/jar,installed executable and118 save hashes are preserved. Production43595 remains absent as previously observed;no server start/stop/restart,production login,installation,commit,deletion or delegation.

Reproduce from client-c with the bundled PowerShell7 after building the three registered tests and four opt-in controls:

    pwsh -NoProfile -ExecutionPolicy Bypass -File verification/completion/tools/receive_batch_parity.ps1 -OutDir <fresh-output-directory>

The verifier compiles the three new reference captures against the unchanged original jar,regenerates four fixtures,checks hashes,runs native/candidate/fault controls and restores all four environment variables. It does not build or install automatically. Candidate binaries,source/before snapshots,task-only diff and identities are retained. Credential-gated live login skipped;CRC passed. Historical C5/recovery whole-frame corpora were not rerun. Source-route totals are64 present/7 missing out of71;these are not correctness or completion percentages. PhaseD continues;PhaseE pending.
