# Revision225 friend/ignore action acceptance — October 24, 2026

**Accepted for controlled original/cache/native/candidate comparisons.** Friend and ignore add/remove actions now work through original prompts and menu actions. Report abuse, real-server list persistence/conversation and final combined-play qualification remain separate.

## Implementation

- Added original social prompt actions1/2/4/5 for clientcodes201/202/501/502; shared80-character input,code8 Backspace/Delete,Enter10/13 and bold12 prompt drawing. Existing private action3 remains distinct. Social input has priority over quantity and chat-interface input; successful submission retains raw socialInput after closing.
- Added typed chat/list menu actions406/436/557/556,including +2000 variants, original @whi@ name extraction/trim/base37 conversion and low-priority ordering. Friend Remove and ignore Remove now execute. Existing online-friend Message remains accepted.
- Applied exact source list100 limits,zero/missing no-ops,duplicate/cross-list error order,base37 formatting,case-sensitive friend self check and source-permitted self ignore. Prompt add-ignore at100 skips addIgnore entirely, unlike direct/menu addIgnore which displays the full-list message. Error sender is original empty string, not null.
- Added staged encrypted118/11/79/171 +8-byte names, with copied cipher,buffered capacity/immediate-write failure checks and local publication only after queue success. Additions append; removals shift active arrays while retaining unused tails exactly. Error-history allocation failures do not fabricate history or list mutation. As with earlier senders, later flush failure or partial immediate external transmission is outside the local transaction guarantee.
- Added dynamic ignore clientcodes401..500 text/button state and503 scroll extent, with original Remove menus. Uses chatbox-owned name buffers; Component size remains unchanged. Real cached friend/ignore Add/Remove buttons now open each prompt without spurious IF_BUTTON packets.
- Extended existing registered test_chatbox; new candidate/failure executables are opt-in diagnostics, not newly registered test programs. Existing public/private/incoming/overhead behavior remains covered by fresh regressions.

## Original comparison

Unchanged bundled225 Java addFriend/removeFriend/addIgnore/removeIgnore,useMenuOption,handleInterfaceAction,handleInputKey,updateInterfaceContent,handleSocialMenuOption,drawChatback/JString/ISAAC are the expectations. New SocialActionsOracle and SocialPromptOracle call those actual methods. No original implementation or expectations are patched.

**3266 cases,758 wire-producing cases,532 exact error-message histories.** Native and actual candidate compare encrypted bytes,full ISAAC scalar/rsl/mem state,all100 friend names/IDs/worlds and ignore IDs including inactive tails,all100 history/type/sender/text slots,prompt/action/recipient/input and ordinary/count input. Each record seeds defined lists/history; cipher persists across the corpus.

Coverage includes every count0..100,zero/new/middle-list targets,0/1/99/100 crossed capacities,first/middle/last removals,inconsistent cross-list membership for error precedence,self variants(normal/uppercase/null),direct methods/menu actions/prompt paths,both +2000 priorities,every key0..255,spaces/underscores/punctuation/case/long names,80-character limit,200 deterministic random strings and repeated backspace. It covers valid base37 list names and ASCII input; arbitrary Unicode or corrupt server names are not newly accepted.

**404 original ignore-row states,104 menu entries,24 complete prompt images/1103616 pixels.** Counts0/1/3/100,codes401..500/503,text/button/scroll and actual component hit testing match. Four source prompt headers and six input forms match every479x96 pixel with guards. The previous accepted804 friend-row/416 menu corpus is also rerun.

## Candidate and native fault controls

Actual client.c/cache/menu dispatch/deferred event phase matches3266 cases and758 isolated-loopback transmissions. A positive add goes through full client_update and flush. Four actual cached Add/Remove buttons on tabs5065/5715 receive queued mouse clicks and open source prompts; actual client_draw complete chatback regions match. No production login/human recipient is involved.

Fault controls:36 insufficient capacities across four packet types,4 exact9-byte successes,4 forced error-UI allocation faults,all4 sticky-overflow/disconnected immediate-write paths,prompt-retaining submit failure and malformed counts/action/opcode guards. Lists/history/cipher/output remain unchanged on the documented failures. Separate copied encrypted-wire,unused-tail-ID and prompt-pixel expectations reject with exit3 on the final executable; original fixtures remain unchanged.

## Fresh evidence and reproduction

Package: runs/2026-10-24-social-actions-sol/.

| Evidence | Result |
| --- | --- |
| reproduce2-wrapper.exit / reproduce2/result.txt |0/PASS; both actual original fixtures regenerate byte-identically,native/candidate/fault checks pass |
| suite-final.exit / suite-final.log |0/ALL TESTS PASSED,no compiler warning/error markers,current separate candidate/replay/diagnostics; credential-gated live login skipped,CRC passed |
| controls-build2.exit |0; strict candidate/failure controls |
| strict protocol/actions/context/legacy and final chatbox builds |0; -Wall -Wextra -Werror |
| rebuilt current private/public incoming and public-send diagnostics |0; strict/current source |
| regression-final-private_message |2634 original states/eight framed packets; wrapper0 |
| regression-final-public_chat |3237 mask states/620 actor timer states/nine packets; wrapper0 |
| regression-final-public_send |3521 input cases/3295 candidate sends and public failure controls; wrapper0 |
| regression-final-private_send |1987 input/menu cases/1746 sends,804 friend rows/416 menus/24 prompt images,cached friend click/243; wrapper0 |
| regression-final-chat_display |144 images,31246848 pixels,21456 hover queries; wrapper0 |
| regression-final-overhead_chat |1426 frames,243857408 viewport pixels/all50 slots,actual projection/settings/draw cycle; wrapper0 |
| negative-controls.json and negative-final logs |three altered expectation copies reject with exit3 |

From client-c,build then reproduce into a fresh directory:

    make -j4 bin/test_chatbox.exe bin/social_actions_loop_control.exe bin/social_actions_failure_control.exe bin/rs2client225.exe bin/rs2replay225.exe
    powershell -NoProfile -ExecutionPolicy Bypass -File verification/completion/tools/social_actions_parity.ps1 -OutDir <fresh-output-directory>

Verifier restores both environment variables,retains process stderr and checks exit/coverage markers; no automatic build/install.

## Retained failures

native1/2 failed at record3 because the generic native message helper collapsed empty sender to null. Source explicitly stores an empty string for social errors. Added localized allocation/publication of an empty sender; native3/4 and final comparisons pass without changing original expectations or unrelated message policy. Initial candidate diagnostic failed strict compilation on misleading indentation and an assert macro/compound-literal comma; corrected diagnostic,controls-build2 and final strict builds pass. Failed logs remain. Existing cumulative Git blank-at-EOF warning in client_actions.c is historical and was not rewritten.

## Preservation and limits

All118 saves match the previous accepted private-send baseline. Normal installed SHA A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4; unchanged original jar SHA57EF9307E2B31674CB55F0F48C1643CDED751E1A97D5035B28F75DB7FD1E6CC7. Production43595/PID17880 still listening. Dirty before/source snapshots,task-only diff,identities,five separate binaries and reproducible evidence packaged. Task jobs terminal/isolated sockets closed,no credentials retained. No install/restart/commit/deletion/delegation.

All four social encoders are verified here. The unchanged coverage generator reports58/71 literal outgoing heuristic matches (was56/71); it misses shared ternary calls for118/11,while79/171 match. Its raw file is retained. Corrected incoming inventory remains55 behavior routes/16 missing. Do not interpret these heuristic numbers as correctness or completion percentages.

This accepts controlled client social-list behavior,not server persistence after relog,report abuse,trade/duel request admission,real-player conversation or the whole social system. Full historical C5/recovery screen corpora were not rerun. PhaseD continues;PhaseE remains pending.
