# Retained populated fixture resolved — October 26, 2026

See [D-RETAINED-RENDERER.md](D-RETAINED-RENDERER.md). Original Java comparison now closes the previously unresolved synthetic error6/kind3 fixture family. The archived extra pending-tile fatal check was a native deviation already removed October25. A later malformed synthetic bridge topology is rejected by Java too. The camera candidate control now retains the populated cache map by building its topology before synthetic camera data is installed; all50 checked native draws pass. The historical empty-scene qualification below retains its original scope and is superseded by this added populated coverage. No production renderer code or normal binary changed in this investigation.

---

# Revision225 camera/cutscene packets — October 24, 2026

**Accepted for the defined controlled source/native/candidate corpus.** Added receive3 CAM_MOVETO,13 CAM_SHAKE,74 CAM_LOOKAT and239 CAM_RESET,plus cutscene interpolation,draw-only shake,pitch floor and cutscene top-level selection.

## Source behavior and implementation

Unchanged client.java read3/13/74/239,applyCutscene,drawScene,getTopLevelCutscene,updateGame and REBUILD_NORMAL define the behavior. Move/look packets contain local tile x/z,height,speed and acceleration. Acceleration>=100 immediately sets position or angles. Horizontal targets use tile*128+64;source height interpolation unusually receives the raw tile byte,not the scaled centre. That behavior is preserved.

Cutscene ticks follow source approach/clamp arithmetic,shortest yaw path/overshoot and128..383 pitch clamp. Original Java squared-distance integer wrap and negative-square-root NaN-to-zero conversion are explicit in C. Modifier cycles advance on update ticks,including disabled modifiers,and wrap as Java int. Shake consumes one shared Math.random double per enabled axis and applies source jitter+sine truncation,yaw mask and pitch clamp only to the drawn view. Pitch-axis wobble can raise the orbit pitch floor. Persistent base camera survives drawing. Reset clears cutscene and enabled bits while retaining other modifier arrays;accepted changed-region rebuild237 clears cutscene without resetting modifiers. Native height sampling supports source-valid edge tile103 with bounds/plane guards. Rebuild reset wiring is source-derived;downloaded-map lifecycle and combined rebuild/camera scenarios remain a later qualification gate.

## Evidence

- **3,307 states:**825 actual original read packets,1,552 applyCutscene/cycle ticks and900 actual drawScene camera/RNG captures. Four planes,ramped/bridge/roof data,acceleration0/1/99/100/101/255,all5 modifier axes,random jitter/wobble,reset/orbit restoration,edge0/1/103/255 and squared-distance overflow.
- Full cutscene fields,base camera and all5 enabled/jitter/scale/speed/cycle arrays compare exactly. Draw camera,top level and full48-bit random state match.
- The original draw capture subclasses World3D.draw to observe its incoming camera arguments and stops at that call. Original drawScene executes its camera/region/random prefix unchanged. The capture restores the documented base fields after stopping;it does not execute or qualify the remaining original renderer/overlay tail. Normal orbit base is computed with actual original orbitCamera. This is camera-view qualification,not original full-frame drawing.
- Actual candidate:825 ISAAC-framed packet-phase observations,1,552 real client_update ticks and50 real client_draw empty-scene views/base restores;full incoming cipher matches. The observer records immediately after the real decoder before the tick,then restores that packet-phase state for fixture sequencing. Orbit/player inputs are held at the reference values. Renderer x/z clamping is accounted for when comparing its view. No production connection.
- 28 invalid lengths,invalid modifier index/plane/null payload and Java cycle wrap controls retain state on rejection. Separate changed state/RNG copies fail exit3.

## Retained failures and limits

Strict initial candidate compilation rejected an unused control helper;the diagnostic now runs it. A populated offline preview with synthetic ramp heights reported renderer error6/kind3. That experiment is retained as an unqualified combined-renderer outcome;the accepted candidate camera draw fixture uses an empty scene and does not resolve or hide that populated preview failure. Initial diagnostic orbit and local-player setup mismatches were corrected to match the reference's held inputs. All failed logs remain;original expectations were not changed to pass those controls. Initial3305-state evidence was extended by two explicit overflow records before final3307 checks.

Arbitrary populated locations/actors,all camera/terrain combinations and new whole-frame parity are not accepted here. Invalid/off-map metadata outside source-valid terrain access uses native guards. Original random anticheat output after1802 orbit draws is outside this900-draw corpus. Source/candidate arithmetic and valid packet behavior are the accepted scope.

## Combined evidence

Package: runs/2026-10-24-receive-batch-sol/. Final acceptance uses suite-final2 (all registered tests,current separate candidate/replay and opt-in diagnostics),strict current builds,reproduce2 and nine regression-final2 families. Original fixtures regenerate byte-identically. Seven separately altered camera-state/RNG,model-geometry/pixel,reset-primary,multiway-state/pixel copies reject exit3. See completion-audit.json for checked exits and stable identities. Existing registered tests were extended;no new TEST_BINS. Original Java/jar,installed executable and118 save hashes are preserved. Production43595 remains absent as previously observed;no server start/stop/restart,production login,installation,commit,deletion or delegation.

Reproduce from client-c with the bundled PowerShell7 after building the three registered tests and four opt-in controls:

    pwsh -NoProfile -ExecutionPolicy Bypass -File verification/completion/tools/receive_batch_parity.ps1 -OutDir <fresh-output-directory>

The verifier compiles the three new reference captures against the unchanged original jar,regenerates four fixtures,checks hashes,runs native/candidate/fault controls and restores all four environment variables. It does not build or install automatically. Candidate binaries,source/before snapshots,task-only diff and identities are retained. Credential-gated live login skipped;CRC passed. Historical C5/recovery whole-frame corpora were not rerun. Source-route totals are64 present/7 missing out of71;these are not correctness or completion percentages. PhaseD continues;PhaseE pending.
