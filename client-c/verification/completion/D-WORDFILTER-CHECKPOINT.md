# WordFilter continuation checkpoint - October 23, 2026 - Sol

## Status: PARTIAL, full filtering unfinished

This continuation implements and verifies prerequisite cache tables and three original UTF-16 primitives. It does not implement the complete WordFilter pipeline. There is no pass-through/full-filter API and no gameplay message callsite or unfiltered-message bypass. Completed WordPack and chat display remain separate accepted goals.

## Implemented

- core/wordfilter.h/.c: WordFilterTables with4311 fragment values,336 bad-word records including signed-byte combination pairs,452 domains,258 TLD strings/types. Lengths are explicit; table character values originate from g1 and fit uint8_t. Arrays are owned heap data; initialize the struct to zero. Decode/load success replaces old data; failure preserves it. Free is idempotent. Original trailing bytes are ignored. Signed pair bytes preserve Java byte semantics. Huge/negative/truncated counts and failed allocations are handled without partial replacement.
- wordfilter_normalize: original filterCharacters on uint16_t arrays. Retains original width, collapses repeated literal spaces, pads the rest with spaces. Allows ASCII32..127,tab,newline,pound163 and Euro8364, as original. It is not String.trim; no final trim happens in this helper.
- wordfilter_restore_case: original replaceUpperCases using ASCII uppercase predicates; '*' masking is preserved.
- wordfilter_format_case: original formatUpperCases. Preserves a leading uppercase run until the first lowercase letter; lowers subsequent uppercase letters in that word and resets on non-ASCII-alpha. This is not ordinary title/sentence casing.
- Existing Makefile CORE_SRCS integration. No new registered tests; existing test_jagfile extended.

## Evidence

Package: runs/2026-10-23-wordfilter-sol/ relative to this directory.

- reference/src/WordFilterOracle.java invokes unchanged bundled original methods and real wordenc cache. Table fixture tests/fixtures/wordfilter-tables-reference.bin contains every original fragment, character, signed combination pair and TLD type. Native control compares all fields exactly.
- Native table controls reject17248 truncated prefixes transactionally while retaining current loaded pointers/counts. Also cover huge count,NULL archive, empty counts/strings, signed -128/-1 pair bytes,type255, replacement and idempotent free.
- tests/fixtures/wordfilter-helpers-reference.bin contains67544 cases: all65536 UTF-16 code units in context, explicit mixed/uppercase/whitespace/Euro/pound strings and2000 deterministic random strings. Each compares original normalization, case restoration and final case formatting; native checks prefix/tail guards and NULL/empty bounds.
- Native control/helper-control.exit0, helper-werror.exit0 and candidate/replay helper-build success. Consult final checkpoint.json for latest suite/build/negative statuses.
- tools/wordfilter_prereqs.ps1 compiles the unchanged original oracle, regenerates tables/helpers/full-output corpus into a fresh directory, requires exact hashes of bundled prerequisite fixtures, then runs the current existing test_jagfile against generated expectations. Wrapper restores both RS2_WORDFILTER_TABLES_ORACLE and RS2_WORDFILTER_HELPERS_ORACLE. reusable/result.txt explicitly reports prerequisite-only PASS.
- Separate altered tables/helpers fixtures must be rejected. Original expectation files are unchanged.

## Captured but NOT compared to a native full filter

filter-reference.bin and reusable/full-filter-reference.bin contain3772 original full-filter input/output pairs in UTF-16. Format: magic0x57464632, big-endian int case count, then each input/output as big-endian int UTF-16-unit count followed by writeChar units. Includes cache bad-word variants, domains/TLD contexts, allowlist, mixed casing, whitespace, byte-range inputs,pound/Euro and deterministic random strings. This is a starting corpus, not exhaustive coverage of every substitution/context branch. Expand it against original methods as the algorithm port proceeds.

The helper corpus is a different format: magic0x57464832,count; per case input,normalized,masked,restored,formatted arrays, each encoded as the same UTF-16 length+units. Table corpus magic0x57463232; read existing test_jagfile and WordFilterOracle for exact field sequence.

## Exact continuation

1. Re-read current queue/handoffs and this checkpoint; preserve the current dirty Makefile/tests/core module and snapshots. No active goal tool was created from ordinary continue; previous WordPack goal is complete. This is unfinished engineering work, not accepted full WordFilter.
2. Port the remaining algorithms only from javaclient/src/main/java/jagex2/wordenc/WordFilter.java (1225 lines); webclient/src/wordfilter/WordFilter.ts supplies readable cross-checks. Keep original entry-point order: normalize, Java trim, English/root lowercase, filterTld, filterBad(two reverse passes), filterDomains, filterFragments, allowlist restoration, replaceUpperCases,formatUpperCases, final Java trim.
3. Preserve normalization's Euro8364 support using uint16_t text. Existing WordPack's Latin-1 API does not replace full-filter input representation. After normalization, allowed characters have ASCII-only casing under the explicitly selected English contract. Java trim semantics and all filtering substitutions/context/fragment lookups still need their exact source-derived implementation.
4. Port original matching/context helpers and fragment hash/binary searches, then expose a complete filtering API only when all stages exist. Do not add a dummy pass-through function or bypass WordFilter when connecting messages.
5. Extend existing controls to consume3772 full-output pairs, diagnose first divergence against unchanged original, add source-derived branch/edge cases, and run appropriate full regressions. Current table/helper checks prove only those prerequisites.
6. Only after WordFilter acceptance integrate private41/public mask0x40 and outgoing148/158 with source ignore/dedup/override/privacy/casing/history rules. Full messaging/social actions remain unsupported.

Reproduce prerequisites after building current test_jagfile:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File client-c/verification/completion/tools/wordfilter_prereqs.ps1 -OutDir <fresh-directory>
```

## Preservation and usage

No installed-client replacement, server restart, account/credential use, commit, deletion or delegation. Existing43595/PID17880 server,118 starting saves, original jar/cache and normal executable are preserved; final identity/save evidence is in the package. Before/current source snapshots and task-only diff preserve pre-existing dirty work. Final handoff is written before the mandatory5% five-hour threshold. Do not resume implementation after reaching that threshold without user resumption. All process handles and actual final exits are recorded in the newest handoff.
