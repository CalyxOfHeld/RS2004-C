# Revision225 incoming public player chat - October 23, 2026 - Sol

## Accepted scope

PASS for incoming public PLAYER_INFO0x40 admission, actor state/history, forced0x8 chat history/reset behavior and player/NPC chat lifetime. This wires the accepted WordPack, WordFilter, ignore list and last-drawn region override into the225 candidate. Player overhead rendering/effects, outgoing composition/sending, social actions and full messaging remain separate. The installed executable was not replaced.

Evidence: runs/2026-10-23-public-inbound-sol/, completion-audit.json and final-audit.txt. All task jobs terminal. Source authority: unchanged javaclient/src/main/java/deob/client.java getPlayerExtended10897 onward (0x8 at10934,0x40 at10953), updateEntityChats9034 and updateGame call; accepted WordPack/WordFilter and real wordenc. No later revision source used.

## Source-derived changes

| Source behavior | Native implementation |
| --- | --- |
|0x40 g2 colour/style,g1 rights,g1 packed length | entity_sync_player_info validates declared body before invoking bounded callback |
| Only named players; ignore if rights<=1; block if overrideChat!=0 | chatbox_add_public_message uses existing ignore IDs and candidate's last scene-drawn override |
| Decode then filter | Existing accepted WordPack/UTF-16 WordFilter; missing tables fail closed |
| Success changes actor text,high colour byte,low style byte,timer150 | Staged actor publication and history type1 for rights>1,type2 otherwise |
| Suppression/decode exception retains old actor/history | Callback leaves those fields unchanged; parser always advances to declared end so later masks stay aligned |
| No duplicate-ID check for public chat | Repeated public packets can add identical messages |
|0x8 forced chat sets raw text,colour/style0,timer150,history type2 | Existing forced-text decode now resets those fields and invokes history callback; source intentionally bypasses public filter/ignore/override |
| updateEntityChats: local player,active remote list,active NPC list; decrement positive timers and clear at0 | entity_sync_tick_chats, called at the matching candidate update phase; inactive cached actors retain timer/text |

EntitySyncContext gains an optional bounded player-chat callback/user context, keeping network mask parsing independent of UI state. The candidate installs its callback before PLAYER_INFO processing. An unconfigured standalone context consumes packed chat without publishing raw text. NPC existing timer behavior is retained; players now expire too.

The public API checks NULL inputs, packed length<=255, colour/rights ranges and ignore count before admission. Filter/decode/allocation failures return false and do not publish partial actor/history output. Mask parsing treats caught decode/filter failure as nonfatal, matching original cde2 catch behavior. Malformed declared bodies fail before the chat callback. Whole PLAYER_INFO packets are not transactional: preceding unrelated mask fields retain the existing source-ordered behavior.

Native empty player name represents original absent/null name. Native empty actor chat represents cleared Java null; an accepted empty message may still have a positive timer, as source. Packed WordPack text uses the accepted single-byte repertoire converted directly to UTF-16, and English/root filter casing. Forced0x8 verification covers ASCII payloads; arbitrary platform-default Java gjstr charset behavior and Unicode/SDL input conversion are not newly accepted by this task. Native forced text retains the existing byte-string contract.

## Fresh evidence

| Gate | Result |
| --- | --- |
| Actual original masks |3237 calls to unchanged original getPlayerExtended, producing actor fields, packet positions and every100 history type/sender/text slot |
| Native comparison | native2.exit0;3237 states exact;2586 history-changing steps (a combined forced/public step can add two messages) |
| Timer source comparison |155 actual original updateEntityChats calls across local,active remote,inactive remote and NPC;620 actor tick states exact |
| Cases | Rights0..255 crossed with ignore/override, missing/mixed-case names, all real bad/domain/TLD cache patterns, empty/unmatched/raw packed inputs, decode overflow and suppression before decoding; forced-only/mixed forced+public |
| Mask alignment | Every source case includes a following0x100 suffix, and its position/spot fields match even after public suppression or caught decode overflow |
| Safe native checks | Overdeclared packed body rejects before chat mutation; NULL payload and missing filter reject; existing source masks/regressions retained |
| Candidate integration | loop1.exit0:9 ISAAC-framed ephemeral-loopback packets through actual client_update/protocol, plus149 subsequent expiry ticks; same-update timer149, staff/ignore/filter/repeated public/clear/ordinary/overflow-suffix/override/forced paths pass |
| Registered suite/builds | suite-final.exit0/ALL TESTS PASSED; no compiler warnings/errors; separate candidate/replay builds succeed |
| Strict builds | Chatbox/entity_sync/legacy client and supplemental candidate diagnostic compile with Werror, clean logs |
| Private regression | Rebuilt current private_message_loop_control; private-wrapper.exit0;2634 original states/2520 additions and8 candidate framed packets pass |
| Display regression | display-wrapper.exit0;144 original chatback/split images,31246848 pixels and21456 hover queries exact |
| Reproducible expectations | reproduce-wrapper2.exit0; original fixture26,370,677 bytes regenerates exact SHA; native and actual candidate diagnostic pass |
| Negative expectation controls | altered-text.exit3 and altered-colour.exit3; separate copies rejected; original expected fixture unchanged |
| Preservation | All118 saves unchanged; normal installed client/original Java jar unchanged; server43595/PID17880 still listening |

The original oracle invokes actual getPlayerExtended/updateEntityChats from the unchanged bundled jar, not copied game formulas. Native controls construct full local-player PLAYER_INFO bit headers around those mask bodies and compare actor/history/suffix state. The supplemental candidate loads real cache in explicit offline fixture mode and attaches an isolated local socket with ISAAC framing; it does not log in or contact production. These are controlled source/state/integration comparisons, not a real-player conversation or player-overhead pixel acceptance. The registered credential-gated login check skipped; cache CRC checks passed.

## Reproduce

From client-c using bundled MSYS2:

```sh
make -j4 test bin/rs2client225.exe bin/rs2replay225.exe
x86_64-w64-mingw32-gcc -std=c99 -Wall -Wextra -Werror -O2 -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 -Iinclude -Ilib/SDL2/x86_64-w64-mingw32/include verification/completion/tools/public_chat_loop_control.c $(find build/src -name '*.o' ! -path 'build/src/client/client.o' ! -path 'build/src/client/client225.o') -Llib/SDL2/x86_64-w64-mingw32/lib -lSDL2 -lws2_32 -lwinmm -lm -Wl,--stack,16777216 -o bin/public_chat_loop_control.exe
```

From the rewrite root in PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File client-c/verification/completion/tools/public_chat_parity.ps1 -OutDir <fresh-directory>
```

The wrapper compiles PublicChatOracle.java, regenerates/checks fixture SHA, runs existing test_entity_sync and actual candidate diagnostic, verifies coverage markers and records identities. It restores RS2_PUBLIC_CHAT_ORACLE. The loop's deliberate caught-decode diagnostic writes stderr; the wrapper retains it and explicitly checks exit status and the final PASS marker. Existing private_message_parity.ps1 and chat_display_parity.ps1 reproduce the named regressions after rebuilding their current executables.

## Retained failures and package

native1.exit3 caught a diagnostic setup mismatch: original cycle1000 vs native context0 affected the following spotanim delay. Setting the native test cycle1000 produced native2.exit0 with unchanged original expectations. First reproduce-wrapper.exit1 was a PowerShell5 stderr-handling failure on the expected caught-decode diagnostic, not a native mismatch. The corrected wrapper retains stderr and checks process exit/PASS, yielding reproduce-wrapper2.exit0. Failed evidence is preserved.

Production files: include/net/entity_sync.h,src/net/entity_sync.c,include/ui/chatbox.h,src/ui/chatbox.c,src/client/client.c. Extended existing tests/test_entity_sync.c; no new registered test. Own dirty before snapshots, task-only.patch, current source/fixture/candidate snapshots, exact identities and preservation evidence are packaged. No application source changes after successful builds/comparisons.

No install, server restart, commit, deletion or delegation occurred. No credentials were used; all isolated listeners/peers/jobs closed. Next bounded task: original player overhead chat rendering and effect positioning, or source-audited outgoing public composition; neither is accepted here. Prior private-message, filter, codec and chat-history display acceptances remain separate and passing.
