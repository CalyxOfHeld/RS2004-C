# Revision225 outgoing public chat sender - October 23, 2026 - Sol

## Accepted bounded task

PASS for controlled outgoing public composition/sending and candidate input integration. The candidate now queues original opcode158, publishes the original filtered local actor/history echo, and reveals public-mode2 as3 with opcode244. This replaces the live-input “nothing was sent” path. Installed normal client is preserved. Real-server delivery to another account, outgoing private/social actions, player overhead rendering and full messaging remain separate acceptance gates.

Evidence runs/2026-10-23-public-outbound-sol/: completion-audit.json, final-audit.txt, source/candidate snapshots and identities. No original source or jar was modified. Primary reference: javaclient/src/main/java/deob/client.java handleInputKey2921-3114, JString.toSentenceCase, WordPack/WordFilter; GameShell.keyPressed maps Backspace and Delete to input code8. Engine MessagePublicDecoder confirms colour/effect then packed body in opcode158.

## Exact valid-input behavior

- Original input accepts codes32..122 and at most80 characters. Backspace/Delete remove the last character; Enter/newline submits nonempty input. Chat-interface modal state blocks ordinary text/backspace/submit. Existing amount-dialog and command paths keep their own priority.
- Colour prefixes are **case-sensitive sequential if statements**, in order yellow,red,green,cyan,purple,white,flash1,flash2,flash3,glow1,glow2,glow3. A later matching prefix may consume what an earlier prefix exposed. The native loop preserves this order; it does not use a modern case-insensitive parser or an else-if chain.
- Style checks run wave then scroll, also sequential and case-sensitive. Prefix-only initial input still sends the resulting empty body as original.
- Wire body is original WordPack of the stripped typed bytes. Local echo independently uses original English/root sentence casing and full cache-backed WordFilter of the typed body. It is deliberately not decoded from wire: unsupported wire characters/padding can differ from the local string.
- Success sets local actor text/colour/style/timer150, type2 history with the local player name and clears typed input. If public mode was2, set3 and append CHAT_SETMODE244 with current private/trade modes. Public duplicates are permitted.
- Existing candidate keyboard events remain deferred until the original late update phase, after actor chat expiry and before idle/output handling. This preserves timer150 in the send cycle. The final complete-update comparison verifies that ordering.

Native APIs: chatbox_send_public in ui/chatbox.h/.c and protocol_send_public_chat in net/protocol.h/.c. UI submission stages filter output and both history allocations before sending. Protocol submission stages the158 packet plus optional244 with a copied ISAAC state, writes the combined bytes once, then commits cipher state. Maximum combined size88 bytes. Native invalid input/modes/NULL tables, allocation failure, preexisting overflow or insufficient buffer capacity cannot publish a fabricated echo or consume cipher state/output bytes. Capacity failure marks out_overflow; caller retains input and normal recovery handling applies. A failed immediate socket write may already have transmitted partial external bytes; only native local publication is transactional. This safe failure policy is distinct from original exceptions/partial-buffer writes.

General FIFO insertion was factored to publish preallocated strings while retaining existing valid behavior. Existing incoming private/public and display checks were refreshed. The sender API handles ordinary input only; :: commands still use the existing separate path. Special developer command behavior such as ::clientdrop is not newly accepted here. The public policy is the original ASCII keyboard range, not arbitrary SDL UTF-8/Unicode input.

## Fresh original and native evidence

| Gate | Result/evidence |
| --- | --- |
| Actual original input method | PublicSendOracle.java invokes unchanged handleInputKey through GameShell's key queue, English locale and real wordenc;3521 key cases/3295 wire-producing cases |
| Registered native comparison | Current test_chatbox compares all outgoing encrypted bytes, complete ISAAC scalar/rsl/mem state,100 history type/sender/text slots, local actor fields, typed input and three modes after every case |
| Case coverage | All13 colour choices (none+12),four style sequences,four public modes and three private/trade modes; chained/reversed/repeated/uppercase prefixes, empty/prefix-only/unsupported input, all256 queued key codes, Backspace,80-character clipping, multi-submit, modal blocking and1000 deterministic random inputs |
| Actual candidate integration | Final reproduce2/loop.log:3521 deferred event-phase cases using actual client_queue_event/process_events/handler;3295 real isolated-loopback transmissions match original encrypted bytes; last case runs full client_update with source keyboard/timer/output ordering |
| Physical Delete mapping | Candidate diagnostic maps code8 to CLIENT_KEY_DELETE; original method/state agrees. Backspace uses the same source mapping/handler path |
| Allocation/capacity failures | Failure control passes two UI strdup failure points,88 insufficient capacities,exact88-byte combined success,preexisting overflow and disconnected immediate write; actor/UI/history/input/cipher/output unchanged on failed native publication |
| Registered suite/current builds | suite-final2.exit0,ALL TESTS PASSED,no compiler warnings/errors; separate225 candidate/replay builds pass |
| Strict compilation | Protocol/chatbox and final legacy client Werror builds clean; final candidate diagnostics and failure control Werror clean |
| Reproducibility | send-final.exit0; fresh original fixture SHA matches bundled40,177,730-byte fixture; native,candidate and failure controls pass |
| Negative expected values | Separate altered-wire and altered-echo fixtures fail expected exit3 at the first mismatch; original fixture unchanged |
| Incoming private regression | Current rebuilt diagnostic/private-wrapper.exit0;2634 original states and8 framed candidate packets pass |
| Incoming public regression | Current rebuilt diagnostic/public-wrapper.exit0;3237 original masks,155 ticks/four actors and9 framed candidate packets pass |
| Display regression | display-wrapper.exit0;144 original images,31,246,848 pixels and21,456 hover queries exact |
| Preservation |118 existing saves,installed client and original jar unchanged; server43595/PID17880 still listening |

The candidate diagnostic loads real cache in explicit offline fixture mode, attaches an ephemeral local socket, runs actual event handling and flushes its output into the isolated receiver. Most cases isolate the deferred keyboard phase to match the original handleInputKey fixture; the last case additionally runs the full update. It does not run3521 full game cycles or a real-player conversation. The registered credential-gated live login check skipped; cache CRC checks passed. Earlier C5 whole-screen corpora were not rerun and retain their own binary identities.

## Reproduce

From client-c in the bundled MSYS2 shell:

```sh
make -j4 test bin/rs2client225.exe bin/rs2replay225.exe
x86_64-w64-mingw32-gcc -std=c99 -Wall -Wextra -Werror -O2 -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 -Iinclude -Ilib/SDL2/x86_64-w64-mingw32/include verification/completion/tools/public_send_loop_control.c $(find build/src -name '*.o' ! -path 'build/src/client/client.o' ! -path 'build/src/client/client225.o') -Llib/SDL2/x86_64-w64-mingw32/lib -lSDL2 -lws2_32 -lwinmm -lm -Wl,--stack,16777216 -o bin/public_send_loop_control.exe
x86_64-w64-mingw32-gcc -std=c99 -Wall -Wextra -Werror -O2 -Iinclude -Ilib/SDL2/x86_64-w64-mingw32/include verification/completion/tools/public_send_failure_control.c $(find build/src -name '*.o' ! -path 'build/src/client/*' ! -path 'build/src/ui/chatbox.o') -Llib/SDL2/x86_64-w64-mingw32/lib -lSDL2 -lws2_32 -lwinmm -lm -Wl,--stack,16777216 -o bin/public_send_failure_control.exe
```

From rewrite root PowerShell, use a fresh directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File client-c/verification/completion/tools/public_send_parity.ps1 -OutDir <fresh-directory>
```

The wrapper compiles PublicSendOracle against the original jar, regenerates/hash-checks the fixture, runs existing test_chatbox and both supplemental controls, verifies current markers and records identities. RS2_PUBLIC_SEND_ORACLE is restored. Supplemental source includes actual production files; no production fault hooks were added. Existing public_chat/private_message/chat_display parity wrappers reproduce the named regressions after rebuilding current diagnostic binaries.

## Retained failures, package and next task

Initial loop1 failed at record0 because diagnostic process_events had queued keys without reaching the existing deferred keyboard phase. Loop2 corrected the diagnostic phase and added a full update; source expectations stayed unchanged. Final reproduce2 uses the current candidate after adding source-correct physical Delete handling and passes. A documentation delta initially used a default cp1252 read and failed on Unicode; the overview was losslessly restored and all subsequent edits used explicit UTF-8/LF. Production/test files were ASCII throughout; no expectations were affected. Failures remain retained.

Source/before snapshots, task-only.patch, current candidates, original fixture identities and preservation are packaged. Existing dirty work retained. No new registered test program, install, production-server restart, commit, deletion or delegation occurred. All ephemeral peers/listeners/jobs closed; no credentials were used.

OVERALL-STATUS.md,D-REMAINING.md,coverage-map.md and current capability entry now reflect controlled public-sender acceptance. Fresh unchanged generator raw output finds55/71 outbound encoder matches, up from54/71 because158 now exists; this remains a heuristic, not an overall completion percentage. Incoming55 routes/16 missing routes are unaffected by this sender addition.

Next bounded implementation: original player overhead chat rendering/colour/effect positioning, using now-accepted text/colour/style/timer state. Private/social sending and real-server public delivery remain open. PhaseD remains in progress; final extended-session/release qualification is still ahead.
