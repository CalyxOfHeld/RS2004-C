# Clue reader failure handling — October 26, 2026

The reader previously ignored `split_pagecount` and displayed only the first eight lines of oversized input. It also queried page zero without checking that a page existed, and would render the missing-description sentinel `null` as clue text. The old line-count diagnostic is not reproduced by the current valid clue data: all 102 bundled text clues fit one to eight lines; six map clues correctly use separate handlers.

Changed `content/scripts/minigames/game_trail/scripts/trail_clue_read.rs2`:

- Missing/empty text uses the existing `string_isnullorempty` helper, closes the old modal and logs only to the server console. It does not invent a missing clue.
- Check page count before reading page zero. Text exceeding the clue interface's capacity uses the existing `mesbox` reader, preserving every page.
- Unsupported line counts use that same fallback instead of returning after a diagnostic.
- Existing valid clue layouts, text, and map-clue handlers remain unchanged.

This fallback is a defensive repair using the bundled message reader, not evidence of original historical behavior for malformed clues.

Focused verification used the real compiled RuneScript interpreter, actual cached item parameters, q8 font and item-read trigger selection. All 108 valid clue displays produce identical UI events before and after the patch. A synthetic 17-line clue reproduces the old eight-line truncation and now displays every line across four message pages. Missing strings close the old display without sending replacement text. An injected zero-page split verifies recovery without an invalid page-zero lookup. Player/UI output is captured at the boundary; this does not claim a live player traversal or complete Treasure Trails gameplay verification.

Isolated compilation and normal server-pack compilation/load both pass with 8033 scripts. The actual normal pack passes the focused check. Script IDs remain unchanged. No native client build or broad suite was needed.

Evidence: `runs/2026-10-26-clue-read-sol/`, including the baseline clue inventory, source/pack backups, task diff, compiler outputs, `normal-check.log` and SHA-256 identities. The normal server pack is already rebuilt. No listener on43595 was present at completion: use the usual `start_local_server.bat`, followed by `test_client.bat`. No process, save, map or original Java file was changed.
