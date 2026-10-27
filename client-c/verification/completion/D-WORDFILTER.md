# Revision225 complete WordFilter acceptance - October 23, 2026 - Sol

## Result and scope

PASS for the complete native filtering library goal. The implementation now performs the entire bundled revision225 WordFilter pipeline. The earlier D-WORDFILTER-CHECKPOINT.md remains a historical prerequisite checkpoint. Current evidence: runs/2026-10-23-wordfilter-complete-sol/; requirement audit: completion-audit.json. All task processes are terminal.

This supplies the filter needed for future public/private chat admission. Gameplay composition, transport, ignore/deduplication, social actions and public/private message integration remain separate work. Existing candidate and installed gameplay paths were not changed to call this API.

Source authority is javaclient/src/main/java/jagex2/wordenc/WordFilter.java, cross-checked against webclient/src/wordfilter/WordFilter.ts. The original Java source and original rs2client.jar were not changed. Expected results call the original jar methods, not a second implementation of their algorithms. No later revision or OSRS source was used.

## Contract

`wordfilter_apply(tables,input,length,output,capacity,out_length)` in include/core/wordfilter.h consumes UTF-16 units and unmodified successfully loaded real-cache tables. Capacity includes the terminating NUL. Optional out_length receives the result length on success and zero on failure. Input/output may overlap in either direction or be identical; out_length must not alias text/table storage. Tables must remain valid and unmodified for the call.

The exact pipeline is normalization, Java trim (units <=32), English/root lowercase, TLDs, two reverse bad-word passes, domains, numeric fragments, original allowlist, uppercase restoration, original word-case formatting and final Java trim. Normalization retains ASCII32..127, tab/newline, pound and Euro; rejects other units to spaces and compacts literal spaces. English/root lowercasing is exact for this normalized repertoire. Original default-locale Turkish/Azeri behavior is not supported; the oracle explicitly selects English. The API is not an SDL UTF-8 conversion policy.

Native failures return false without altering text or tables: NULL with nonzero input length, NULL output, zero/insufficient capacity, invalid table metadata, missing fragments/zero-length patterns, lengths above INT_MAX-2, size multiplication overflow or allocation failure. Empty input may be NULL. As with ordinary C APIs, metadata checks cannot validate arbitrary dangling pointers. Five internal UTF-16 buffers stage all work before output is committed. These transactional failure semantics are intentionally safe native handling, not a promise to reproduce Java exception side effects.

## Source mapping and preserved details

| Original methods | Native implementation |
| --- | --- |
| unpack/read/readFragments/readBadWords/readBadCombinations/readDomains/readDomain/readTld (62-174) | tables_load/tables_decode/read_words |
| filterCharacters/allowCharacter (176-203) | wordfilter_normalize |
| filter (205-234) | wordfilter_apply |
| replaceUpperCases/formatUpperCases (237-266) | restore_case/format_case |
| filterBad and filter byte/combination overload (268-278,644-790) | two reverse passes / wf_bad_match |
| filterDomains/filterDomain (280-358) | reverse domain pass / wf_domain_match |
| four domain/TLD context status methods | wf_before/wf_after with exact direction/type distinctions |
| filterTld overloads and path/prefix extension | wf_tld_match and stage clones |
| comboMatches/getIndex (792-817,1026-1039) | wf_combo/wf_index; signed byte lexicographic lookup |
| getEmulatedDomainCharSize/getEmulatedSize (819-1024) | wf_domain_emulated/wf_emulated |
| filterFragments/indexOfNumber/indexOfNonNumber (1041-1113) | wf_numeric_fragments |
| original character predicates (1115-1164) | wf_symbol/wf_weak/wf_alpha/digit/lower/upper |
| isBadFragment/firstFragmentId (1166 onward) | wf_bad_fragment/wf_fragment_id |

The original type3 TLD left>2/right>0 expression is computed but unused; native masking follows the used left>0/right>2 condition. Six-unit fragment hashes and long numeric groups use explicit modulo32 arithmetic and Java signed interpretation. Fragment NUL units are skipped without multiplying. Bad-word emulation and narrower domain emulation remain distinct; Euro and pound substitutions retain UTF-16 values. Digit-match/skip flags,90% skip ratio, fragment exemptions, repeated symbols, reverse table order and allowlist restoration preserve the original order.

## Fresh evidence

| Gate | Result/evidence |
| --- | --- |
| Real-cache tables | 4311 fragments,336 bad words,452 domains,258 TLDs; exact original values |
| Existing loader/helper checks | 17248 truncated prefixes rejected transactionally;67544 primitive cases including every65536 UTF-16 unit |
| Complete output | 160976 unchanged-original input/output pairs, max65536 units; fixture7,377,308 bytes |
| Direct original branches | 8540625 records, detailed counts below |
| Native safety | 1136 distinct loader allocation failure points preserve existing tables; full API allocation failure/NULL/metadata/size/capacity/guards and forward/backward/in-place overlap pass |
| Registered regressions and candidates | suite-final.exit0, ALL TESTS PASSED; candidate/replay links succeeded; no compiler warnings/errors in log |
| Strict module/diagnostic builds | wordfilter-werror.exit0; branch-promoted-build.log clean, branch-promoted-control.exit0 |
| Reproducibility | reproduce-wrapper.exit0; regenerated tables/helpers/full fixtures have identical SHA256; reproduce/result.txt PASS |
| Negative full-output expectation | altered-full.exit3; first nonempty expected output changed only in separate copy, mismatch detected at case2 |
| Negative branch expectation | altered-branches.exit3; first emulation expected byte changed only in copy, record1 rejected |
| Preservation | all118 saved-player hashes unchanged; normal installed executable and original jar hashes unchanged; server43595/PID17880 still listening |

Direct branch records:8161164 emulations (each compares both emulation helpers),66362 fragment ID/search,189720 combination lookups,1 signed synthetic combination table definition,37552 context records (247020 positions, each four status methods),55695 bad matcher,6737 domain,20898 TLD and2496 numeric. The total includes the single table-definition record. This is source-directed branch evidence, not a numerical code-coverage percentage or exhaustive proof over all possible strings.

Expanded complete-output families, in addition to the3772 baseline: bad boundaries40656; skip3024; repeats672; skip ratios1680; two-unit substitutions2200; single substitutions7012; cache exemptions415; domain6737; TLD20898; numeric2496; every UTF-16 unit in context65536; trim260; allowlist605; deterministic random ASCII5000; long inputs13. Cases include source-derived real-cache words, signed context pairs, numeric overflow/group limits, type3 TLD behavior, prefix/path extensions and long text.

## Reproduce

From client-c in the bundled MSYS2 shell, build existing tests and separate candidates:

```sh
make -j4 test bin/rs2client225.exe bin/rs2replay225.exe
x86_64-w64-mingw32-gcc -std=c99 -Wall -Wextra -Werror -O2 -Iinclude -Ilib/SDL2/x86_64-w64-mingw32/include verification/completion/tools/wordfilter_branch_control.c $(find build/src -name '*.o' ! -path 'build/src/client/*' ! -path 'build/src/core/wordfilter.o') -Llib/SDL2/x86_64-w64-mingw32/lib -lSDL2 -lws2_32 -lwinmm -lm -Wl,--stack,16777216 -o bin/wordfilter_branch_control.exe
```

Then from the rewrite root in PowerShell, use a new output directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File client-c/verification/completion/tools/wordfilter_parity.ps1 -OutDir <fresh-directory>
```

The wrapper compiles both WordFilterOracle.java and WordFilterCases.java, regenerates original table/helper/full/branch expectations, hashes the three bundled fixtures, runs the registered Jagfile comparison and direct source diagnostic, verifies current coverage markers and records identities. It restores all three process-only oracle overrides. wordfilter_prereqs.ps1 delegates to this complete verifier for compatibility. The supplemental native diagnostic includes actual wordfilter.c to inspect private stages without adding production hooks, and must be built without NDEBUG.

## Retained failures and limits

Initial implementation Werror errors (misleading indentation and memcpy static bounds) were corrected with explicit bounds and formatting; first-build.log/exit1 remains. First supplemental diagnostic compiler failures were corrected without suppressing warnings; branch-build1 remains. Initial expanded Java compile failed because the classes output directory did not exist; creating it produced expanded-oracle-build2.exit0. All original expected values stayed unchanged. An optional GCC coverage attempt could not link __gcov_exit; coverage/build.log and control.exit1 are retained. No gcov coverage percentage was collected or claimed.

The suite's credential-gated live login check skipped; its cache CRC checks passed. No new live messaging claim follows from that suite. This goal uses real cache/original algorithm comparisons and native safety controls. Existing scene/display/recovery/WordPack acceptances remain separate evidence.

Source snapshots, task-only.patch against this goal's before snapshots, exact identities and separate candidates are packaged. The existing dirty Makefile integration originated in the prerequisite task and is retained. Existing dirty project work was preserved; no commit, install, server restart, deletion or delegation occurred. Next engineering task: source-audit one bounded message admission path using the accepted WordPack and WordFilter libraries before implementing its full state/deduplication/ignore/filter and display behavior.
