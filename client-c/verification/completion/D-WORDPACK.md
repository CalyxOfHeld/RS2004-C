# Revision225 WordPack chat codec - October 23, 2026 - Sol

## Scope and contract

Result: PASS for the bounded codec goal. All8 requirements are proved in runs/2026-10-23-wordpack-sol/completion-audit.json. Final suite-cache0, candidate/replay build0, WordPack Werror0 and reusable cache/original comparison0. All task jobs terminal.

Native core/wordpack.h and core/wordpack.c port the bundled revision225 WordPack pack/unpack, with the existing Makefile CORE_SRCS integration. No gameplay send/receive/admission callsite is added. This is a codec acceptance, not working public/private messaging.

The API uses the existing native single-byte Latin-1 text repertoire, including pound sign0xa3, and explicit lengths. It is not a UTF-8/UTF-16 conversion API. For this repertoire, ASCII A-Z lowercasing has exactly the same encoding result as original Java English/root lowercasing; all256 input bytes were compared. The oracle explicitly selects Locale.ENGLISH. Arbitrary Unicode strings and locale-specific Java behavior such as Turkish I are outside this byte API and must not be inferred from these results. SDL UTF-8 input needs an explicit caller conversion/validation policy before future integration.

Source authority: javaclient/src/main/java/jagex2/wordenc/WordPack.java, cross-checked against webclient/src/wordfilter/WordPack.ts. Java is primary: TypeScript's output-loop guard differs at overflow, so native valid output follows Java and native failure is explicitly bounded.

| Rule | Implementation |
| --- | --- |
| Input limit | Pack first80 Latin-1 bytes, corresponding to80 original UTF-16 units in this repertoire |
| Lowercase/unsupported | ASCII uppercase to lowercase; unsupported byte values, including embedded NUL, map to first space entry |
| Table | Original61 entries, including duplicate space at37; packing uses first matching space0 |
| Packing | Exact one/two-nibble mapping and carry, +195 for entries above12 |
| Flush | Boolean API matches original argument; gameplay source passes true; false discards pending carry |
| Padding | Final low zero nibble may decode as an extra space; do not trim codec output |
| Decoding | Consume exact packed byte count; unmatched final extended nibble is ignored as original |
| Capitalization | Initially and after '.'/'!'; '?' does not reset capitalization |
| Output bound | Original100-character scratch capacity; native overflow returns false without partial state |

Packet-based reusable APIs:

```c
bool wordpack_pack(Packet *packet, const char *text, size_t length, bool flush_tail);
bool wordpack_unpack(Packet *packet, size_t packed_length, char *text,
                     size_t capacity, size_t *out_length);
```

Packing precomputes into at most80 bytes before writing. Decoding stages at most100 characters before writing a terminating NUL. On failure, packet.error becomes/stays true, packet position and input/output buffers remain unchanged, and optional out_length becomes0. Existing packet errors stay sticky. Capacity includes NUL. NULL input text is valid only at zero length; decoding empty input still requires output space for NUL. Source-length, destination-capacity, NULL and invalid packet-position errors are checked before external writes. These are safe native failure semantics, not a claim that original exceptions leave identical partial state. The original can throw after partial reads/100 scratch writes.

## Verification

Existing tests/test_packet.c is extended; no new registered test is added. Its default expected fixture is tests/fixtures/wordpack-reference.bin, with optional RS2_WORDPACK_ORACLE override. It compares output bytes/text and packet positions, prefix/tail guards, exact and insufficient capacities, preserved buffers, NULL/empty/bounds cases and sticky errors.

reference/src/WordPackOracle.java invokes unchanged original WordPack directly. The final corpus contains:

- 140722 pack cases, each with original encoded bytes and original decoded result. Inputs include empty, all256 single Latin-1 values, all65536 ordered Latin-1 pairs, punctuation/case samples, lengths0..101 of representative one/two-nibble characters/spaces/pound signs,3000 deterministic random inputs up to160 bytes, and1046 actual revision225 cache strings. Each input is tested with flush=false and true.
- 71405 raw decode cases: empty, all256 one-byte sequences, all65536 two-byte sequences,612 patterned length cases0..101, and5000 deterministic random byte sequences.
- 2178 cases where original Java throws ArrayIndexOutOfBoundsException from its100-character scratch limit; native rejects safely and transactionally. This does not replace a valid original output with a different result.

Real-cache inputs come from the original WordFilter.unpack of engine/data/pack/client/wordenc: badWords/domains/tlds,1046 strings. They supply codec test text only; native filtering is not implemented or claimed. The generator verifies that these cache strings fit Latin-1. The original jar and cache bytes are unchanged.

Evidence directory: runs/2026-10-23-wordpack-sol/ relative to this document. Authoritative final results are listed in completion-audit.json. Earlier first comparison with138630 pack cases remains historical and is retained with reference-first.bin and WordPackOracle.first.java. Final real-cache corpus has140722 pack cases.

Reusable check, after building the current bin/test_packet.exe, from the rewrite directory:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File client-c/verification/completion/tools/wordpack_parity.ps1 -OutDir <fresh-output-directory>
```

The wrapper compiles the original oracle, regenerates expected bytes from unchanged jar/cache, requires identity with the bundled fixture, compares the native implementation, records identities and restores prior RS2_WORDPACK_ORACLE. Use a fresh directory to retain evidence. reusable-cache/result.txt and wrapper exit0 prove this path. A separate negative-cache-pack fixture changes original expected encoded byte at case3/offset74; existing controls reject it with expected exit3. The original expected corpus remains untouched. No production login or real account is used.

## Remaining dependencies

Do not display unfiltered decoded messages as if chat were complete. The bundled WordFilter.java is1225 lines and depends on the wordenc archive's fragmentsenc.txt, badenc.txt, domainenc.txt and tldlist.txt. Its filter entry point normalizes characters/whitespace, applies TLD/bad/domain/fragment processing, allowlist restoration and casing/trim logic. A source-derived native port plus cache/original comparison is the next prerequisite.

Original client.java dependencies:

- Private receive41: sender/id/rights,100-message dedup, ignore checks and overrideChat, then WordPack.unpack followed by WordFilter.filter before type3/7 history.
- Public extended mask0x40: appearance/ignore/override conditions, WordPack.unpack followed by WordFilter.filter, overhead colour/style/timer and type1/2 history. Current native skips compressed payload bytes.
- Outbound private148/public158 and local echoes: source protocol framing, WordPack.pack(true), JString.toSentenceCase and WordFilter for local display, privacy changes when applicable. Codec completion does not implement these paths.
- Composition input/social management, trade/duel request admission/acceptance and menu action dispatch remain separate, as documented in D-CHAT-DISPLAY.md.

## Preservation

Normal installed executable, original jar, existing43595/PID17880 server,118 starting saves and unrelated dirty work are preserved. No install, server restart, commit, deletion, delegation, real credentials or live messaging claim. before/ snapshots and task-only diff preserve the dirty Makefile and packet tests; new files are recorded as previously absent. Final handoffs identify remaining work and usage before the5% threshold.
