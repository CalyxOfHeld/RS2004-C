# Revision225 outgoing private-message acceptance — October 23, 2026

**Accepted for controlled original/cache/native/candidate comparisons.** Outgoing private composition, prompt/input, packet148, type6 local history, private2->1/settings244 and the reachable friend Message action are implemented. Real-account delivery and the remaining social actions are separate open tasks.

## Changes and source behavior

- Original online-friend Message action679 opens the prompt with recipient base37 ID and `Enter message to send to <friend>`. An offline or missing friend does not open it. Menu activation closes quantity input as the original does.
- Original clientcodes1..200 supply dynamic friend names/world text/button state, and203 supplies list scroll extent. Actual component hit testing now generates original Remove557 and Message679 labels; Message uses a typed action carrying stable recipient ID, then resolves current friend membership/world on activation. Remove remains its existing unsupported social operation.
- Private input accepts codes32..122, at most80 characters, code8 backspace (including mapped physical Delete), Enter10/13 submit. Social input has priority over quantity/chat-interface input. Empty submission closes without bytes; successful send retains filtered socialInput after closing, preserves ordinary/count input strings and public actor state.
- Wire uses original unfiltered typed text, WordPack, recipient8 bytes, one-byte body length and encrypted148. Prefixes such as red:/wave:/:: have no special private interpretation. Local echo independently sentence-cases and filters through the real wordenc tables, formats the base37 recipient and adds type6 history. If private mode2/Off, it changes to1/Friends and queues encrypted244 with current public/trade values.
- Sender allocates UI strings and stages copied cipher/both packets before publishing history/prompt/privacy. Allocation/filter/capacity/write failures retain local state; insufficient buffered space marks overflow. An immediate socket failure may transmit partial external bytes even though local publication is retained.
- Real bold12 prompt drawing matches the original and outranks quantity/modal content. Incoming243 replaces it with quantity input. Normal login reinitializes the chatbox, preserving existing login/reconnect behavior.
- Kept Component allocation size unchanged. Dynamic status buffers live in Chatbox; component text points into stable chatbox storage. No new registered test program; existing test_chatbox extended, with opt-in candidate/failure diagnostic targets.

## Original evidence

References: unchanged bundled225 Java client.useMenuOption(679), handleInputKey, updateInterfaceContent, handleSocialMenuOption and drawChatback; original ISAAC/WordPack/WordFilter/JString, real title/media/wordenc. PrivateSendOracle and PrivatePromptOracle call these methods; original implementation and expected fixtures are not patched.

**1987 original menu/key cases,1746 wire-producing cases**: native and actual candidate match bytes, complete ISAAC scalar/rsl/mem state, all100 history/type/sender/text entries, prompt/open/recipient/input state, ordinary/count input and privacy modes. Cipher/history persist across records.

Coverage: all public0..3/private0..2/trade0..2 combinations for ten text forms and two online recipients (720 cases); every code0..255, empty/whitespace/prefix/command-like/filter/URL/tag text, input lengths0/1/79/80/81/100/120, repeated backspace/Enter10, offline/missing menu targets, blocked chat interfaces, and1000 deterministic random strings. Valid base37 friend recipients and ASCII input repertoire are the accepted scope; arbitrary Unicode/invalid server names/report-abuse editor behavior are not newly accepted.

**804 original dynamic friend row states,416 menu entries**: counts0/1/3/100, clientcodes1..200 and203, names/Offline/local-world/other-world labels, button state and scroll. Existing component_build_menu performs actual hit testing and returns identical menu labels/action numbers with typed Message recipient metadata.

**24 original complete prompt images,1103616 pixels** using real bold12/chatback: four recipient labels and six input shapes, empty/tag/mixed/long/80 spaces/80 wide glyphs. Original quantity/modal flags coexist to prove prompt priority. Native compares every479x96 pixel and guards.

## Actual candidate and fault evidence

Actual included client.c loads real cache, dispatches menus, drains normal deferred key events and transmits1746 isolated loopback messages across1987 cases; the final case goes through full client_update and flush. Actual cached friend tab5065 builds Message as the primary action, receives a queued left click, and opens the prompt. Actual client_draw chatback region matches the complete prompt image. An actual ISAAC-framed incoming243 through client_update cancels private input and opens empty quantity input. No production connection/login or human recipient is involved.

Two forced UI allocation failures,94 insufficient capacities,exact94-byte combined148/244 success,sticky overflow and disconnected immediate write pass with unchanged prompt/history/privacy/cipher/output on failure. Native missing-filter/length/character/NUL guards pass. Separate copied wire,prompt-state and prompt-pixel expectations all reject with exit3; original fixtures remain unchanged.

## Fresh checks and reproduction

Evidence package: runs/2026-10-23-private-outbound-sol/.

| Evidence | Result |
| --- | --- |
| reproduce1-wrapper.exit / reproduce1/result.txt |0/PASS; both actual original fixtures regenerate byte-identically; native/candidate/fault checks pass |
| suite2.exit / suite2.log |0/ALL TESTS PASSED, no compiler warning/error markers; current registered suite and separate candidate/replay/overhead diagnostic builds |
| controls-build2.exit |0; both private diagnostics, strict candidate macros |
| strict-chatbox/protocol/actions/component/legacy build logs/exits |0; -Wall -Wextra -Werror |
| rebuilt private/public incoming and public outgoing diagnostics |0; strict/current actual client source |
| regression-private_message |2634 original states/eight candidate packets; wrapper exit0 |
| regression-public_chat |3237 original mask states,620 actor timer states/nine packets; wrapper exit0 |
| regression-public_send |3521 original input states/3295 transmissions and public-send fault controls; wrapper exit0 |
| regression-chat_display |144 images,31246848 pixels,21456 hover queries; wrapper exit0 |
| regression-overhead_chat |1426 frames,243857408 viewport pixels/full50-slot state,actual candidate projection/settings/draw clock; wrapper exit0 |
| negative-controls.json |three altered expectation copies rejected with exit3 |

Build and reproduce from client-c (fresh output directory required):

    make -j4 bin/test_chatbox.exe bin/private_send_loop_control.exe bin/private_send_failure_control.exe bin/rs2client225.exe bin/rs2replay225.exe
    powershell -NoProfile -ExecutionPolicy Bypass -File verification/completion/tools/private_send_parity.ps1 -OutDir <fresh-output-directory>

The verifier restores both environment variables and does not build/install automatically.

## Retained failure and fix

suite1.exit2 stopped with a4MiB arena allocation failure in an existing interface test after adding40 bytes to each Component. The requested allocation was4272000 bytes against4194304. Moved the dynamic friend status buffers into Chatbox and restored Component size. suite2 and strict/regression checks pass afterward. Retained suite1 evidence; this was a real introduced allocation regression, now fixed. Original expectations unchanged. Compiler checks are clean; an existing unrelated blank-at-EOF warning in client_actions.c's cumulative Git diff predates this task and was not rewritten.

## Preservation and remaining scope

All118 player saves match the accepted overhead baseline. Normal installed executable SHA A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4 and original jar SHA57EF9307E2B31674CB55F0F48C1643CDED751E1A97D5035B28F75DB7FD1E6CC7 unchanged. Production43595/PID17880 still listening. Dirty before/source snapshots,task-only diff,identities,five separate binaries and reproducible evidence packaged. Diagnostics finish and ephemeral sockets close; no credentials/background jobs retained. No install/restart/commit/deletion/delegation.

Fresh unmodified coverage generator output records outgoing literal/encoder heuristic56/71 after148 (previous55/71). Corrected incoming source inventory remains55 behavior routes/16 missing; the generator's48/62 receive output remains inaccurate. These counts are not completion percentages.

Real-server two-account delivery/conversation, friend/ignore add/remove/report execution, request admission and remaining protocol/settings/gameplay qualification remain open. This does not rerun all historical C5/recovery screens or accept the whole messaging/social system. PhaseD continues,PhaseE pending.
