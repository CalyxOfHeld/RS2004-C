# Workspace source inventory — October 21, 2026

The user added client and supporting project directories to `2004scape-LostCityRS` and asked that future work account for them. Inspect local source before inferring behavior. Folder timestamps do not establish when the user added a directory.

## Sources for the native revision-225 port

| Path inside this rewrite | Purpose |
| --- | --- |
| `javaclient/src/main/java` | Primary May 18, 2004 revision-225 client behavior and rendering reference. |
| `webclient/src` | TypeScript client port; readable cross-check of the same client behavior. |
| `engine/src` | Production server codecs, handlers and game engine. |
| `content` and `engine/data/pack/client` | Actual scripts, definitions, interfaces and packed cache assets. |
| `client-c` | Native C99/SDL2 implementation, tests and recorded verification. |

The handoff already references these bundled sources. They have already supplied collision, render and audio comparisons; their existence does not by itself validate remaining native behavior.

## Additional sibling directories

| Path relative to this rewrite | What is present and how to use it |
| --- | --- |
| `../Client-lastmain` | Refactored Java client project labeled May 18, 2004, with separate `client`, `runetek3`, loader, map viewer and annotations. It provides descriptive local variable names where the bundled Java often uses decompiler names. The sampled main client differs from the bundled Java file; compare provenance and modifications before importing code. |
| `../Client2-main` | TypeScript source port labeled May 18, 2004, with cache/model/item/sound viewers, message animation and interface tooling. `assembly/bz2.ts` is an AssemblyScript decompressor. Its tree differs from the bundled `webclient`; reconcile differences before using them as expectations. |
| `../Server-main` | Separate server checkout and synchronized historical handoff. The active native launcher uses this rewrite's configured server on port 43595. Confirm process ownership rather than assuming which directory a server uses. |
| `../rsmod-pathfinder-main` | Rust pathfinder dependency source. Useful to understand server routing, but native reference behavior remains the revision-225 client. |
| `../rsbuf-377` | Rust buffer dependency source. The directory name explicitly mentions 377; do not import later-revision protocol behavior. Inspect only relevant dependency mechanics when needed. |
| `../RuneScriptCompiler-main` | Public compiler release README; no compiler implementation source found at the directory root during inventory. |
| `../Website-main` | Website application, public assets and views; separate from native rendering and protocol behavior. |

## Continuation rules

Preserve all trees, cache, nested repository history and saves. Read-only verification on October 22, 2026 found the rewrite repository on branch master at commit 7eb8cee (Phase B3/B4: Java parity oracle, make parity, protocol parity with Rev 225), with no configured remote. The working tree contains tracked modifications and untracked files; preserve both. These are checkpoint observations; recheck Git state before future work. No commit/reset/cleanup is implied by continuation. Do not search for the deferred Cursor/Grok output or delegate work without new authorization. Use the latest handoff, rather than old ZIP documentation, to select the current binary and acceptance scope.
