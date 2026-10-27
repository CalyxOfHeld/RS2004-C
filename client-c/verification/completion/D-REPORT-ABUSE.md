# Revision225 report-abuse acceptance — October 24, 2026

**Accepted for controlled original/cache/native/candidate comparisons.** The report interface now opens from the footer or chat menu, edits the name, displays the original moderator option and submits the selected category. No reports were sent to production or to people.

## Implementation and source

The unchanged bundled225 client.java methods handleChatSettingsInput, useMenuOption34/947/951, closeInterfaces, handleInputKey, updateInterfaceContent600/613 and handleInterfaceAction601..613 define behavior. Engine ReportAbuseDecoder defines the8-byte name,category byte and mute byte. ReportAbuseOracle calls the actual original methods and real cached interface/title/media; original Java and jar remain unchanged.

- Footer bounds416..516/490..522 and tagged chat-menu34/+2034 open cached root5875 after encrypted close231. Source opens even when another interface is already active. Menu names are trimmed literal text, preserving case/punctuation; missing tag only closes quantity input.
- Report editor has priority over social/quantity/chat input. Only ASCII letters,digits and space append while length<12; Backspace removes one character. Enter/Escape do not submit or close. Prefilled menu text may exceed12,as in source.
- Component600 draws the original20-cycle cursor. Rights hides or shows component613; its source action toggles mute unconditionally. Rights enforcement by a real server is outside this client-only comparison.
- Codes601..612 send close231 then optional190 with base37 name,zero-based category and mute. Empty input closes only; nonempty spaces still report name0. Cancel sends close231,closes quantity input and retains report fields exactly.
- Cipher/output/local interface publication is staged. Capacity,sticky-overflow and disconnected immediate-write failures retain editor/mute/interfaces/cipher/output as documented. Later flush failure and partial immediate external transmission are outside this local transaction guarantee.
- Added typed report menu metadata and chatbox-owned buffers. Component size is unchanged. Existing test_chatbox was extended; two new opt-in diagnostics are outside registered TEST_BINS. Separate candidate/replay builds preserve the installed executable.

## Original and candidate evidence

**2,182 original cases;1,720 category reports;2,121 wire-producing cases.** Footer,context-menu and already-open modes; both rights states; empty/case/space/long/punctuation names; all12 categories and cancel; repeated mute toggles; every key0..255; exact footer boundaries and missing tags. Full encrypted bytes and ISAAC count/a/b/c/rsl/mem are compared with all viewport/chat/sidebar/report interfaces,editor/mute,social prompt/action/recipient and social/count/ordinary input. Cipher persists across records.

**24 complete512x334 panels,4,104,192 pixels** match original,including editor cursor,rights/mute text/colour and guards. Six input forms,cursor transition cycles and both rights/mute states are covered.

The actual candidate uses real client.c/cache,deferred editor events,footer mouse events,typed menu dispatch and isolated loopback. It matches all2,182 states and2,121 transmissions; a positive report goes through full client_update and flush. Actual cached mute/category/cancel buttons receive real queued mouse clicks. Candidate integration also matches292,800 original painted foreground pixels; full candidate3D scene parity is not claimed by that overlay comparison.

Fault diagnostic passes15 capacity rejections (open,12 category capacities,empty category,cancel),exact12-byte close/report and1-byte open successes,sticky overflow,disconnected immediate writes and invalid categories. Separate copied wire,state and pixel expectations are rejected with exit3; bundled original expectations stay unchanged.

## Fresh evidence and reproduction

Package: runs/2026-10-24-report-abuse-sol/.

| Evidence | Result |
| --- | --- |
| reproduce1-wrapper.exit;reproduce1/result.txt |0/PASS; both original fixtures regenerate byte-identically; native,candidate,fault checks pass |
| suite-final.exit/log |0;ALL TESTS PASSED;current candidate/replay/opt-in diagnostics;no compiler warning/error markers |
| strict-ui-chatbox/client_actions/context_menu,strict-net-protocol,strict-client-client |0;Wall/Wextra/Werror,including legacy client configuration |
| current private/public incoming and public-send diagnostic rebuilds |0;strict current source |
| regression-current-private_message |0;2,634 original states/eight framed packets |
| regression-current-public_chat |0;3,237 masks/620 actor-timer states/nine packets |
| regression-current-public_send |0;3,521 cases/3,295 candidate sends/fault controls |
| regression-current-private_send |0;1,987 cases/1,746 sends/friend rows/menus/pixels/cached click/243 |
| regression-current-social_actions |0;3,266 cases/758 sends/532 errors/ignore rows/prompt pixels/four cached buttons |
| regression-current-chat_display |0;144 images/31,246,848 pixels/21,456 hover queries |
| regression-current-overhead_chat |0;1,426 frames/243,857,408 pixels/all50 slots/actual candidate integration |
| negative-controls.json |three altered copies reject exit3 |

From client-c,build then reproduce in a fresh output directory with the bundled PowerShell7:

    make -j4 bin/test_chatbox.exe bin/report_abuse_loop_control.exe bin/report_abuse_failure_control.exe bin/rs2client225.exe bin/rs2replay225.exe
    pwsh -NoProfile -ExecutionPolicy Bypass -File verification/completion/tools/report_abuse_parity.ps1 -OutDir <fresh-output-directory>

The verifier restores both environment variables,retains stderr and validates exit/coverage markers. No automatic build or install. Registered credential-gated live login skipped; CRC check passed.

## Retained failures and preservation

The first seven regression-final wrapper launches used an older powershell without Get-FileHash and failed during identity capture. Logs remain; all seven regression-current reruns with bundled pwsh pass. Initial2146-case evidence remains historical; final2182 corpus adds36 explicit cancel cases. This task found no production mismatch in its original comparisons. Existing cumulative Git blank-at-EOF warning in client_actions.c predates this task and is retained.

All118 saves match the prior accepted social-action baseline. Installed normal SHA A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4; original jar SHA57EF9307E2B31674CB55F0F48C1643CDED751E1A97D5035B28F75DB7FD1E6CC7. Production43595/PID17880 still listening. Dirty before/source snapshots,task-only diff,identities,five separate binaries and completion audit are packaged. No install,restart,commit,deletion,delegation,production login or human recipient; task processes terminal and isolated sockets closed.

## Limits and next work

This covers valid ASCII/source/cache/client and isolated transport,not server report acceptance/moderator authorization,real-player conversation,list persistence after relog,arbitrary corrupt names or full historical C5/recovery screen reruns. PhaseD remains active;PhaseE pending.

Raw outgoing source heuristic is60/71 (was58/71). Report190 is a real new literal match; the second added match11 is a false-positive from the new category upper-bound expression in send_report,not new social implementation. Original social118/11 were already directly verified; raw heuristic still misses118. Corrected incoming55 behavior routes/16 missing remain unchanged. These counts are not completion percentages.

Next bounded task: original packet4 trade/duel request admission,then remaining receive-route families and workflow/release gates. Real-server report/public/private/social delivery and persistence remain separate gates.
