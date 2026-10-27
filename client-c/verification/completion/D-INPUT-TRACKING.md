# Input tracking — controlled acceptance, October 25, 2026

The candidate now handles ENABLE_TRACKING226 and FINISH_TRACKING133,records the original input stream,and emits encrypted EVENT_TRACKING81 reports. This closes the final two missing receive routes in the current source inventory:all71 original incoming types now have native handling routes. Route presence is not exhaustive behavioral or release qualification. Phase D remains active.

Evidence began October24 and completed October25 in `runs/2026-10-24-input-tracking-sol/`. Final checks: `suite-final.log`,`reproduce-final/`,and twelve `regression1-*-wrapper.exit` files. The existing registered `test_protocol_transport` was extended; `input_tracking_candidate_control` is opt-in and was not added to TEST_BINS.

## Original behavior retained

Primary references are jagex2/client/InputTracking.java,GameShell.java callbacks,client.read133/226,client.updateGame around7720,and original normal-login/logout reset sites. Engine ClientGameProt81,EventTrackingDecoder andEventTrackingHandler establish the var-short report framing and server's500-byte body limit.

- Enable replaces the current buffer,discards the old buffer,and updates lastTime. It retains trackedCount,lastMoveTime and lastX/Y. Disable clears logical buffers while retaining those metadata fields.
- A valid event increments the count and encodes elapsed milliseconds divided by10,with an upper cap250 and no lower clamp. Native arithmetic preserves Java signed wrap and truncation. Mouse moves read time but are ignored if less than50ms since the last accepted move. Invalid press/move coordinates consume no tracking clock read.
- Buttons,small/medium/absolute moves,keys,focus and enter/exit produce all13 original event codes. Position packing and delta ranges are exact. GameShell key normalization preserves the unusual function/home/end press/release asymmetry and InputTracking's subsequent key-byte remapping.
- Before an event,`position + eventSize >= 500` rotates the current buffer into the old slot. Another rotation replaces the previous old slot. Valid normal blocks are at most499 bytes,although the original allocated5000-byte Packet objects.
- The game-update prefix sends only the old buffer. Finish sends only a nonempty current buffer and drops the old buffer. Empty finish emits nothing. A later enable does not reset movement history/count.
- Normal login replies2/18 and logout disable tracking. Reconnect reply15 retains it. The native control verifies these call sites with injected login replies and an actual isolated server142 logout;it does not authenticate an account.

The C recorder uses fixed storage for the two logical prefixes. This avoids buffer-allocation failure while preserving successful source behavior. Tracking output stages bytes and a copied ISAAC state;capacity/sticky/disconnected failures retain recorder payload/state and cipher for safe recovery. This guarded failure policy is a native safety extension,not a claim of identical Java exception outcomes.

## Reference integrity and comparisons

The source oracle compiles isolated copies of the original InputTracking andclient classes. It replaces exactly10 clock-call boundaries with a deterministic provider and inserts one observer after the actual updateGame tracking-flush/idle prefix,before actor updates. The read133/226 handlers,recording algorithms and original GameShell callbacks remain unchanged. The original files and jar are never rewritten. A class guard rejects an accidental launch with uninstrumented classes before generating a fixture.

| Fresh check | Result |
| --- | --- |
| Original capture |5,339 states,336 actual read133/226 controls,163 actual updateGame prefixes;all13 encoded event types witnessed |
| Registered native |Complete active buffer prefixes/presence/lengths,count,times,positions,encrypted reports and full outgoing ISAAC match |
| Separate candidate |336 encrypted control packets,499 actual client_update calls,4,665 synthetic public-queue input events;all state/wire/cipher and source clock-call counts match |
| Report bounds |51 original reports;largest observed prefix499 and complete wire frame502 bytes |
| SDL mapping |760 key translation observations match the supplied original GameShell key bytes;4 focus/window routes and3 untracked event types checked |
| Event/clock replay |480 per-event complete recorder states and481 clock reads reproduce exactly,including a negative recorded time while the live provider deliberately differs |
| Lifecycle |Coalesced226/226 and226/133;native normal/mod login disable,reconnect15 retention,server142 logout without a final report;login replies injected |
| Guarded failures |23 invalid-kind/null/metadata/length guards,flush/stop output retention,exact capacity recovery and500-byte direct encoder framing pass |
| Negative expectations |Separate count,time,buffer,wire,cipher and clock-call mutations reject with exit3;wrong-class launch rejects with exit1 |

The corpus includes disabled/empty cases,coordinate edges,movement compression boundaries,49/50ms admission,negative and capped delays,GameShell key mappings/ASCII/undefined/large raw codes,all focus/window events,re-enable,old-buffer overwrite,flush versus finish,maximum normal block499,signed-time/count wrap,and4,000 deterministic random operations. The additional maximum-block observation was generated by the original,not a manual expected-data edit. Earlier5,338-state captures remain historical.

The candidate holds nontracking UI consumption at the observer boundary,while exercising the public queue recorder and actual native game updates for controls/flushes. Thus the original prefix and native full-tick controls qualify recording/traffic,not arbitrary actor/UI side effects or asynchronous OS interleavings.

## Platform and replay integration

Raw tracking key/focus/window events append new enum values without renumbering existing event IDs. Normal key/text events remain separate;the duplicate control proves that raw press/release plus ordinary UI events produce exactly two tracking records and two clock reads. Wheel,text-input and unrelated window events do not invent recording types absent from the225 source.

The Windows SDL platform provides logged wall-clock milliseconds for tracking,independently of its existing monotonic game clock. The replay entry point supplies the recorded clock to the same callback. The event log's six integer fields/text grammar remains unchanged. The480-event control uses the actual event and clock log APIs and compares every recorder state;its log is a control fragment,not a complete RS2REC gameplay recording. Whole active-session CLI replay is not newly qualified here.

SDL key observations cover the supplied plain/shift/caps/control US ASCII and special-key correspondences. Core normalization/encoding is checked independently through actual original GameShell calls. Arbitrary layouts,IME,physical keyboard timing and other untested keypad/chord combinations remain outside this acceptance. No physical user input was captured. The current Windows clock/provider builds strictly;OS timing/resolution is not a bit-for-bit scheduling claim. Other platforms retain their existing fallback clock and are not newly qualified.

## Regressions, failed evidence and preservation

Final full registered suite,current separate playable/replay candidates and opt-in builds pass with no compiler warning/error markers. Strict builds pass for the recorder,protocol,legacy client,SDL platform,replay platform and protocol test. The four older manually linked incoming/private/public/public-send diagnostics were rebuilt. All twelve regression wrappers exit0:incoming private/public,outgoing public/private,social actions,report abuse,game requests,chat display,overhead chat,camera/interface/reset receive batch,map download,andlast-login. Original fixtures in those wrappers regenerate and match their pinned expectations. Live-login gating was removed only in the suite's child process;credential-gated login is skipped and CRC checks pass. Historical full C5/recovery image corpora were not rerun.

The final tracking fixture reproduces exactly. Source/binary/cache identities,generated source copies,boundary diffs,dirty before/current snapshots,task-only diff and completion audit are retained. Initial setup failures are visible:the first generator expected11 clock sites and was corrected to the actual10;early C indentation warnings were fixed. Including the full SDL header in the test helper initially changed main toSDL_main and caused build3 to fail. Its subsequent native3 run used the previous executable and is not fresh acceptance evidence. The helper now includes only SDL_events;fresh native/candidate/reproduction checks pass. Earlier captures and failed logs were not deleted or used to change source expectations.

All118 saves match the preceding last-login baseline andall828 packed map files retain their names,sizes and SHA256 hashes. Installed normal rs2client.exe remains A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4;the original jar remains57EF9307E2B31674CB55F0F48C1643CDED751E1A97D5035B28F75DB7FD1E6CC7. Production43595 was observed NOT LISTENING;this task performed no production server action. No installation,commit,user-file deletion,delegation,production account use or human messages occurred. Inputs are synthetic;traffic uses isolated loopback;all task processes/sockets are terminal.

Eight existing source/test/build files and nine new module/header/oracle/control/verifier/fixture files are packaged with four separate binary snapshots. The normal installed executable remains unchanged and therefore does not automatically contain this feature.

## Current plan position

All71 original/native/engine incoming framing lengths agree,andall71 have source routes. Raw incoming62/62 still omits nine grouped zone entries from its denominator. Raw outgoing61/71 adds the real81 encoder but retains known literal-heuristic limitations,including the118 miss and unrelated11 category-bound match. These are not feature-completion percentages.

The remaining Phase D work includes existing UI/action/script behavior,settings/audio effects,real-server/player workflows and combined gameplay/release qualification. Last-login redraw/continue metadata is still unrepresented/unqualified;the concrete next bounded task is the continue-button waiting/suppression and interface-reset lifecycle. The previously documented populated-camera renderer error6/kind3 remains open. Current overview,remaining work,capabilities,coverage and both handoffs are synchronized.
