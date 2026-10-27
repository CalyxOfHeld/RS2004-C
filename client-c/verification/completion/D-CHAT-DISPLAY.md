# Revision225 chat display parity â€” October 23, 2026 â€” Sol

## Scope and result

Result: PASS for the bounded display goal. Final requirement audit: `runs/2026-10-23-chat-display-sol/completion-audit.json`. Current full suite0, candidate/replay0, legacy Werror0 and client integration0. All task jobs terminal.

This goal implements chat **display and hover-menu construction** from the bundled revision225 original. It does not implement private-message transport/composition, compressed public-chat reception/sending, friend/ignore management actions, report submission, or trade/duel request admission and acceptance. These existing gaps remain explicit; a rendered fixture is not evidence of working messaging.

The implementation shares original visibility rules between rendering and hover rows; hidden messages consume no rows. It supports normal message types0â€“8, public/private/trade modes, case-insensitive friends and self, forced types1/7, filtered scroll height, exact prefix/colour/spacing/clipping, login-name prompt formatting and raw-login input positioning, and split-private shadow/foreground with five-row limit and countdown reservation. Unknown type9 is ignored as original.

Client integration applies server filter settings32 and local filter clicks, cache clientcode8 (actual cache varp287), and friend update152. It retains chat/friends on reply15 and resets them on fresh reply2. Viewport menu construction appends after private options, preserving Cancel/private/viewport priority and the existing actor400-entry gate. Normal message hover remains active under a sticky chat interface, matching the original's unusual handleInput behavior.

## Source authority and message audit

Only bundled sources supplied behavior; the original jar was unchanged.

| Concern | Original Java source in javaclient/src/main/java/deob/client.java |
| --- | --- |
| Normal type visibility, colours, prefixes, scroll/prompt | drawChatback,9663â€“9755 |
| Split private presentation and countdown row | drawPrivateMessages,1641â€“1692 |
| Normal hover rows/rights/self/trade/duel | handleChatMouseInput,2689â€“2755 |
| Split hover bounds and low-priority action IDs | handlePrivateChatInput,5819â€“5860 |
| Global menu order and chat area bounds | handleInput,6518â€“6560 |
| Split setting | updateVarp clientcode8,4564â€“4567 |
| Friends include self, ignore case, offline membership | isFriend,10859â€“10877 |
| Friend packet/world order/notices | read152,9834â€“9882 |
| Message retention/reset | login reply2/15,7169â€“7318 |

Type admission audit:

| Type | Original meaning/admission | Current native limitation |
| --- | --- | --- |
| 0 | System text, MESSAGE_GAME4 | Native reception exists |
| 1/2 | Privileged/ordinary public history, player extended chat and local send echo | Compressed public mask0x40 currently skips bytes; live send explicitly unavailable; offline type2 echo exists |
| 3/7 | Ordinary/privileged private input, packet41 | Packet41 transport/decode/filter/dedup not implemented |
| 4/8 | Trade/duel notification classification from packet4 suffix | Existing native packet4 stores generic type0; classification, ignores/restricted-area admission and acceptance remain separate |
| 5 | Friend login/logout and social notices | Friend packet152 now produces source-derived login/logout notices |
| 6 | Outgoing private echo | Private composition/send remains separate |

For types whose admission is unavailable, this goal proves display/menu behavior using controlled original history fixtures. Menus expose original entries and priorities, but existing legacy action dispatch returns ACTION_UNAVAILABLE and sends no packet. No full social-system claim is made. Native nodeId10 matches the original standalone default and this project's configured default; multi-world client configuration is outside this goal.

## Evidence

Package: `runs/2026-10-23-chat-display-sol/` relative to this directory.

- `reference/src/ChatDisplayOracle.java` invokes unchanged original drawChatback, drawPrivateMessages, handleChatMouseInput and handlePrivateChatInput using real title/media cache.144 combinations cover public4/private3/trade3/split2/countdown2; each uses one of four scroll offsets0/42/448/1000 and four raw login-name variants. The history has100 entries with types0â€“9, friend/self/stranger senders and literal colour-tag text. This is not an exhaustive cross-product of scroll positions/usernames with every setting.
- Extended existing `tests/test_chatbox.c`, enabled with RS2_CHAT_ORACLE, checks every pixel of144 chatback and144 viewport images:31246848 pixels exactly, plus21456 normal/split hover queries and original action IDs/labels/rights/bounds. It also checks self/friend visibility, friend packet length/idempotence and notices.
- `reusable-final2/result.txt`, `native.exit0`, `oracle.exit0` and wrapper2.exit0 demonstrate the promoted `tools/chat_display_parity.ps1`. Build the current `bin/test_chatbox.exe` first; the wrapper records executable, jar and fixture identities and requires a fresh output directory. It restores prior RS2_CHAT_ORACLE on exit.
- `client_control.c` includes the actual client implementation and existing fake-login fixture. It uses encrypted loopback packets32/152/150/175/193, current real cache metadata, client_draw glyph/menu checks, public/private/trade click events, and actual login15/2 paths. No production connection or account is used. Its log text Connected to LIVE SERVER means only its loopback test peer.
- `negative-rgb.exit3` is expected: a separate copied fixture changes the blue channel of one pixel, and the native comparison rejects case0/chatback pixel0. The unchanged original fixture remains intact.
- Full registered suite, candidate/replay build, legacy Werror translation compile and source/identity/preservation audits are recorded in this package. Consult the completion audit for final exit statuses.

Reproduction from the rewrite directory (PowerShell):

```powershell
# First build test_chatbox with the existing project make environment.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File client-c/verification/completion/tools/chat_display_parity.ps1 -OutDir <fresh-output-directory>
```

## Retained failures and corrections

- Initial unquoted JVM option was parsed incorrectly by PowerShell before oracle execution; quoted retry succeeded.
- client-control1 used swapped150/175 fixture sizes; original source confirmed150=g2+g1b,175=g2+g4. Corrected the fixture, not production expectations.
- client-control2/3 caught actual missing split menu integration: an insertion landed in an earlier input branch, then viewport menu rebuilding cleared the options. Fixed actual integration and added an append API/control.
- prompt-control rejected mixed-case login-name pixels. Corrected the native prompt to format the entered login name while retaining raw-login positioning, as original. Original expectations unchanged.
- First reusable wrapper failed its own Select-String positional parsing after native comparison passed; explicit path/pattern fixed wrapper2.
- First legacy compile caught texture-only varp metadata declaration. Declaration/load/free now serve chat in both build paths.
- First control log's pixel total31291488 was a reporting typo; measured total is31246848, corrected in the current test and final logs. Earlier logs remain historical.

## Preservation and next work

Normal installed executable and original jar retain prior accepted identities. Existing server43595/PID17880 and118 starting saves are preserved. No install, server restart, commit, deletion or delegation occurred. Before/current source snapshots and a task-only diff preserve unrelated dirty work. All diagnostic processes must be terminal at acceptance.

Next implementation should source-audit and implement message admission/transport and social action execution as a new explicit goal before claiming usable full public/private messaging. Current display acceptance cannot substitute for that work or for full PhaseD acceptance.
