# Revision225 incoming private messages - October 23, 2026 - Sol

## Accepted goal

PASS: incoming private-message admission and ignore-list replacement, integrated into the separate225 candidate. Accepted WordPack and WordFilter libraries are now used by packet41. Packet21 maintains the ignore list. This is incoming private messaging; sending/composition, public PLAYER_INFO0x40 chat, social-action execution and full messaging remain separate tasks. The normal installed executable was not replaced.

Evidence: runs/2026-10-23-private-inbound-sol/; completion-audit.json and final-audit.txt. All task jobs terminal. Primary source is unchanged javaclient/src/main/java/deob/client.java:10485 (private) and10689 (ignore list), plus addMessage8274 and drawWildyLevel1589/scene draw6622. engine/src/network/game/server/codec/MessagePrivateEncoder.ts confirms41's sender/id/rights/packed data and staff-level adjustment. No later revision source was used.

## Behavior and contract

The native chatbox_add_private_message reads sender g8, signed message ID g4, rights g1, then the remaining WordPack bytes. It first checks all100 remembered IDs (initially zeros, so ID0 is initially duplicate), checks ignore IDs when rights<=1, and checks the last scene-drawn region override. Suppressed messages do not decode or alter the ring/history. Rights>1 bypass ignore but do not bypass duplicates or override.

An admitted ID enters the100-entry ring and advances its cursor before decoding, exactly as the original try block. Decode/filter failure leaves that ID remembered and adds no history, matching the original catch ordering. WordPack's100-unit scratch bound is safely enforced. The packed repertoire converts directly to UTF-16 for the complete cache-backed WordFilter; no unfiltered fallback exists. Result sender uses original base37 decoding/name formatting; rights>1 pushes type7, others type3. Original FIFO history length is100.

chatbox_apply_ignores replaces the count and active prefix from packet21's g8 IDs, retaining the unused tail as original. Normal chatbox initialization clears both ignore and duplicate state; existing reconnect flow retains chatbox state. Existing free/init/full-login ownership is preserved.

Native malformed policy: private header shorter than13, payload above255, NULL payload/chat, or invalid count/cursor metadata fails before mutation. Ignore payloads above800, nonmultiples of8, or NULL/nonempty fail before mutation. These reject malformed inputs for which the original may read stale buffer bytes, floor a partial ignore entry or throw; no parity claim for undefined malformed original behavior. If admitted decoding/filtering/allocation fails, return false after source-ordered ID admission. History insertions now allocate sender/text before FIFO mutation so allocation failure cannot destroy existing messages; existing void callers retain their API.

Real wordenc tables load with the other cache archives and free at shutdown. Failure to load fails initialization; missing tables never display raw private text. The225 candidate refreshes the region restriction during scene draw and incoming packets consume its stored value. This matters across movement/rebuilds before the next draw. Legacy source also compiles, but regional drawing/admission acceptance is for the225 candidate.

## Fresh verification

| Gate | Evidence/result |
| --- | --- |
| Actual original packet handler comparison | oracle1.log;2634 packets through unchanged client.read/ClientStream |
| Native full state comparison | native1.log/reproduce/native.log;2520 additions,1 caught decode failure; all100 type/text/sender/ignore/ring slots plus cursor/count exact after every packet |
| Original case coverage | Rights0..255; ignored and nonignored senders; staff bypass; override and duplicate priority; ID0, negative IDs,205-entry wrap and reuse; ignore full/shrink/clear; invalid/base37 boundary names; empty/unmatched nibble and random packed inputs; every real bad/domain/TLD cache pattern through decode/filter |
| Native safe boundaries | Existing test_chatbox checks all short private headers, overlong payload, all partial ignore lengths below800, oversize/NULL, and missing-filter fail-closed behavior after ring admission |
| Actual candidate dispatch | loop3.exit0/log;8 ISAAC-framed packets over an ephemeral isolated loopback socket through actual client_update/protocol; ignore, moderator, filtering, duplicate, clear, ordinary message, before-draw acceptance and after-draw regional blocking |
| Full registered regressions | suite-final2.exit0/ALL TESTS PASSED; no compiler warnings/errors |
| Builds | Separate candidate/replay built; candidate client compilation already uses Werror; werror-chatbox.log/werror-legacy.log clean; supplemental loop-build3.log Werror clean |
| Display regression | display-wrapper.exit0; original144 chatback/split images,31246848 pixels,21456 hover queries exact with updated FIFO |
| Reusable verifier | reproduce-wrapper.exit0; regenerated original fixture SHA matches bundled23,523,980-byte fixture; both native and candidate controls pass |
| Negative expectation controls | altered-ring.exit3 and altered-text.exit3; separate copied ID/text expectations rejected; original fixture unchanged |
| Preservation | save-preservation.json:118 unchanged saves; preservation.json installed client/original jar unchanged; server43595/PID17880 still listening |

The oracle drives actual original variable-length framed packet handlers using an in-memory Socket adapter. No original source or jar was modified, and expected history/state comes from the original object. It sets English locale, matching the accepted filter contract. Native supplemental integration uses actual sockets/protocol ISAAC framing without login or production-server access. It loads real cache into an explicit offline diagnostic fixture before attaching the isolated socket. This is controlled candidate integration, not a real-account conversation or a production live-session acceptance.

The full suite's credential-gated login test skipped; CRC checks passed. Existing full filter/codec comparisons still pass in the registered suite. Whole-world recovery parity was not rerun because this goal changes incoming chat admission, not recovery flow.

## Reproduce

From client-c using the bundled MSYS2 shell:

```sh
make -j4 test bin/rs2client225.exe bin/rs2replay225.exe
x86_64-w64-mingw32-gcc -std=c99 -Wall -Wextra -Werror -O2 -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 -Iinclude -Ilib/SDL2/x86_64-w64-mingw32/include verification/completion/tools/private_message_loop_control.c $(find build/src -name '*.o' ! -path 'build/src/client/client.o' ! -path 'build/src/client/client225.o') -Llib/SDL2/x86_64-w64-mingw32/lib -lSDL2 -lws2_32 -lwinmm -lm -Wl,--stack,16777216 -o bin/private_message_loop_control.exe
```

From the rewrite root in PowerShell, use a fresh directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File client-c/verification/completion/tools/private_message_parity.ps1 -OutDir <fresh-directory>
```

The wrapper compiles PrivateMessageOracle.java against the original jar, regenerates the fixture, checks its hash, runs current test_chatbox and the supplemental candidate loop, verifies coverage markers and records identities. RS2_PRIVATE_ORACLE is restored on exit. Existing chat_display_parity.ps1 reproduces the display regression. The large binary fixture deliberately records every history/ring/ignore slot per step; it is not application data.

## Source/package and retained failures

Production changes: include/ui/chatbox.h, src/ui/chatbox.c, src/client/client.c. Existing tests/test_chatbox.c gains original state and safe native controls; no new registered test. Source snapshots, own before snapshots/task-only.patch, identities, candidate binaries and preservation files are packaged. Original Java client/WordPack/WordFilter and engine sources were unchanged.

Initial supplemental loop1 failed because diagnostic code passed NULL platform to client_init; corrected to a zero-initialized platform struct. loop2 passed seven basic packets. Source review then identified last-drawn override ordering; production changed to retained state and loop3 added the eighth before-draw packet plus scene draw/after-draw block. Earlier failed/pass evidence remains; acceptance uses loop3 and suite-final2. No expectations were adjusted to hide a production mismatch.

No installation, production-server restart, commit, deletion or delegation occurred. All task listeners/peers/processes closed; no credentials were used. Next bounded engineering task: source-audit and implement incoming public PLAYER_INFO0x40 admission, reusing the accepted ignore/filter libraries with exact actor chat colour/effect/lifetime/history behavior and original-state comparisons. Outgoing messaging remains separate.
