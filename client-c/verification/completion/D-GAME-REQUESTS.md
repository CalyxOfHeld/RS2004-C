# Revision225 trade/duel request admission — October 24, 2026

**Accepted for controlled original/cache/native/candidate comparisons.** Incoming packet4 now distinguishes trade/duel requests from ordinary game messages. This accepts request admission and history/modal behavior; trade/duel execution and real-player delivery remain separate.

## Source and implementation

Unchanged bundled client.java read4 (around10590),addMessage (around8274),JString.toBase37 and engine MessageGameEncoder define behavior. Content trade/duel scripts emit displayname:tradereq: and displayname:duelreq:. No original source or jar was edited.

- Exact case-sensitive ending :tradereq: becomes type4 with “wishes to trade with you.”; :duelreq: becomes type8 with “wishes to duel with you.”. Sender is the raw substring before the first colon,retaining case,punctuation,spaces and long text. Embedded colons and chained suffixes follow the original first-colon/final-suffix rule.
- Convert that raw sender to original base37 for the active ignore-list prefix. Ignore and nonzero stored region override suppress requests without adding history. Unused ignore tails do not suppress. Ordinary type0 messages bypass request suppression and keep source empty-string sender; no word filter or sentence formatting is applied.
- Only admitted type0 messages replace an existing sticky modal. Requests and suppressed messages leave it alone. Actual candidate uses the same draw-phase stored region override as private/public chat.
- Added decoder status distinguishes successful suppression from actual publication. Two history allocations complete before FIFO publication; allocation failure preserves history and other state. Existing non-override helper delegates to this implementation.
- Packet4 has a one-byte length: reject zero/over255 payload,missing LF,embedded NUL before first LF,invalid ignore metadata and null arguments. Embedded NUL rejection is a native safety policy rather than arbitrary Java-string parity. Bytes after the first LF are ignored,as source. The accepted original corpus uses ASCII; broader byte/Unicode conversion is not newly qualified.
- Existing registered test_chatbox extended; two opt-in diagnostics added outside TEST_BINS. No installed executable update.

## Original and actual candidate comparisons

**2,073 original packet4 cases;1,385 history additions;294 trades;286 duels.** Exact and near-match suffixes,empty/space/punctuation/mixed-case/long sender names,embedded/chained colons,LF/trailing data,overrides-1/0/1/2,all ignore counts0..100,first/last/inactive-tail matches,and every ordinary text length0..254. Includes maximum255-byte frames and500 deterministic mixed cases.

GameRequestsOracle invokes actual unchanged client.read/ClientStream with framed packets. Every record compares all100 history type/sender/text slots including null-vs-empty strings; native checks also preserve all100 ignore slots and editor/social/count/report state. Original captures modal,click and redraw fields. Candidate compares complete modal text; native tick click-consumption and absence of an equivalent redraw flag are not presented as per-field click/redraw parity.

Actual client_init loads the real cache. Candidate client_update receives all2,073 ISAAC-framed packet4 records on an ephemeral isolated connection,compares all100 history slots and full incoming cipher,plus modal state.21 split opcode/length headers retain state until completed. Four additional framed controls check before/after actual region draw,malformed missing-LF rejection and valid-message recovery. No production login or human recipient.

Fault controls pass6 forced allocation failures across ordinary/trade/duel,7 malformed/metadata guards,allocation-free ignore/override suppression,and first-terminator trailing-data behavior. Separately altered type/sender native expectations and modal candidate expectations each reject exit3; bundled source expectations remain intact.

## Reproduction and fresh evidence

Package: runs/2026-10-24-game-requests-sol/.

| Evidence | Result |
| --- | --- |
| reproduce1-wrapper.exit;reproduce1/result.txt |0/PASS; original fixture regenerates byte-identically;native/candidate/fault checks pass |
| suite-final.exit/log |0;ALL TESTS PASSED,current separate candidate/replay/opt-in builds,no compiler warning/error markers |
| strict-ui-chatbox and strict-client-client |0;Wall/Wextra/Werror,including legacy client configuration |
| private_message/public_chat/public_send_loop/public_send_failure build-final |0;strict diagnostics rebuilt with current source |
| regression-current-private_message |0;2,634 original states/eight framed packets |
| regression-current-public_chat |0;3,237 masks/620 actor timer states/nine packets |
| regression-current-public_send |0;3,521 cases/3,295 sends/fault controls |
| regression-current-private_send |0;1,987 cases/1,746 sends/friend rows/menu/prompt/cached button/243 |
| regression-current-social_actions |0;3,266 cases/758 sends/532 errors/ignore rows/prompt/four cached buttons |
| regression-current-report_abuse |0;2,182 cases/2,121 transmissions/24 panels/cached mute/category/cancel |
| regression-current-chat_display |0;144 images/31,246,848 pixels/21,456 hover queries |
| regression-current-overhead_chat |0;1,426 frames/243,857,408 pixels/all50 slots/actual candidate integration |
| negative-controls.json |three altered expectations reject exit3 |

From client-c,build then reproduce with bundled PowerShell7 in a fresh directory:

    make -j4 bin/test_chatbox.exe bin/game_requests_loop_control.exe bin/game_requests_failure_control.exe bin/rs2client225.exe bin/rs2replay225.exe
    pwsh -NoProfile -ExecutionPolicy Bypass -File verification/completion/tools/game_requests_parity.ps1 -OutDir <fresh-output-directory>

Verifier restores its environment variable and retains stderr/exits/coverage/identities. Credential-gated registered live login skipped;CRC checks passed. No full historical C5/recovery rerun;fresh prior display/overhead regressions are separate corpora.

## Capture correction and retained failures

The first oracle compile used String.repeat,unsupported by local Java8;replaced with local char-array helper. Second capture encountered an index100 test setup error;fixed index bounds. Strict failure diagnostic initially rejected misleading indentation;fixed diagnostic formatting. Logs remain.

Initial native/candidate comparisons caught an observer error at record109: original repeated request text uses the same constant object,so top-text identity undercounted additions. The revised capture observes the discarded FIFO tail with a unique marker,restoring the old tail when suppressed. The handler does not read that discarded tail;the marker never appears in output. capture-observer-correction.json proves only81 “added” flags changed,with every history and modal byte identical. The initial2073-case capture reported1304/251/248;accepted capture reports1385/294/286. Original source/jar never changed;no native production behavior or original history expectations were altered to hide a mismatch.

## Preservation and limits

All118 saves match prior report acceptance. Installed SHA A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4;original jar SHA57EF9307E2B31674CB55F0F48C1643CDED751E1A97D5035B28F75DB7FD1E6CC7 unchanged. **Production43595 is not listening when checked after this task.** Previous handoff PID17880 is historical;this task did not start,stop or restart a server and does not attribute when it stopped. No install,restart,commit,deletion,delegation,credentials,production connection or messages to people. Task jobs terminal/isolated sockets closed.

Dirty before/source snapshots,task-only diff,25 identities,five separate candidate binaries and completion audit packaged. Diff review is clean for the five changed files. Corrected incoming55 behavior routes/16 missing and raw outgoing60/71 heuristic remain unchanged;packet4 already had a route. Counts are not overall completeness percentages.

Next bounded task: original camera/cutscene receive family3/13/74/239,then other16 missing receive routes and broader workflow/release qualification. PhaseD active;PhaseE pending. Real-player request delivery,trade/duel acceptance/execution,report authorization,social persistence and final combined sessions remain open.
