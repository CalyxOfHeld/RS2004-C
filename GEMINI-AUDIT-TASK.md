# External-agent continuation: revision-225 minimap protocol audit

Updated October 21, 2026 after eight small client changes. The user reports that Cursor/Grok partially performed this work before exhausting usage. Its partial results have not yet been located or reviewed here: the expected gemini-audit output directory is currently empty. Do not treat an empty output folder as proof that no work occurred. This filename and output directory are retained for compatibility with the original assignment; the reported worker was Cursor/Grok, not Gemini.

Recovery of the prior Cursor/Grok output is deferred at the user's request; do not spend more tokens searching for it. If the user later supplies files or a final response, record completed/incomplete items and continue the outstanding audit. Preserve recovered output and identify which claims were independently checked. Without that output, state the limitation before producing new analysis; do not claim to have resumed specific findings you have not seen.

Produce a read-only audit to prepare the next native client task. Do not implement features. Read AGENT.md, NEXT-STEPS-AND-GEMINI.md, the latest sections of client-c/verification/completion/progress.md, and AUTONOMOUS_PLAN.md first. HANDOFF.md describes the earlier paused checkpoint; the recent run notes and source supersede its minimap status. The next full handoff is due at95% usage/5% remaining.

Use only the bundled revision-225 Java client, TypeScript port and engine/content references. Do not use OSRS, later revisions, web guesses or generated behavior. Keep all access and outputs inside this workspace. Do not start agents or delegate to GPT.

## Scope

### Already implemented — review rather than duplicate

| Current native change | Evidence and limits |
| --- | --- |
| MIDI open/play/status/restart failure reporting | runs/2026-10-21-midi-status; outside this audit's scope |
| Minimap dot coordinate division before subtraction | runs/2026-10-21-minimap-offset; built, no live visual check |
| Ordinary visible-player dots using cache mapdots2 | runs/2026-10-21-player-dots; built, no two-client check; friend colouring absent |
| Scene-click destination flag using cache mapflag | runs/2026-10-21-destination-flag; built, no live checks; sets after route send, clears on arrival/packet19, rebases and masks distant flags |
| Operation-approach destination flag | runs/2026-10-21-approach-flag; shared action helper now updates the same flag after a successful nonempty approach-route send; runtime verification pending |
| Live minimap click navigation | runs/2026-10-21-minimap-click; original rectangle/inverse transform, current zero zoom/angle and opcode165 with14-byte trailer; runtime verification pending |

All run paths above are relative to client-c/verification/completion/. Each contains README.md, source-review.diff and build.log. Read each build record; tests/live scenarios have not been rerun for these small changes. The older 37-program/live acceptance must not be attributed to the latest executable. Current binary hash and files are pinned in the minimap-click run manifest. No code in this table needs implementing again merely because it appeared absent in the older handoff.

### Remaining audit priorities

1. **Review minimap clicks:** the native client now implements the original input rectangle/inverse transform and opcode165 trailer for its current zero minimap zoom/angle. Audit click/menu/modal handling, rotation, collision-route selection and serialization against original source. Explain each of the14 trailer bytes, whether the engine decodes or skips it, and which validations actually run. Identify discrepancies and remaining runtime checks rather than proposing duplicate input handling.
2. **Review approach-route flags:** client_actions.c now updates optional pointers to the shared minimap flag after a successful nonempty approach send. Compare original tryMove endpoint semantics, all action callers, failed/empty route behavior and action ordering; review the new integration instead of duplicating it.
3. **Review the existing scene-click flag:** compare endpoint choice, failed/empty route behavior, arrival timing, packet19, region rebase, sprite crop/masking and draw order against original sources. Report concrete discrepancies with file/line references; do not label all of this missing. Distinguish implementation presence from runtime verification.
4. **Player dots:** confirm visibility and sprite choice; identify the original online-friend condition and the missing native social state. Record a dependency rather than inventing friends or implementing social packets.

Do not broaden into combat, audio, a routing rewrite or full social implementation. Nonzero minimap zoom/anticheat offsets need source-backed mapping; do not assume the current zero-offset rendering already covers them.

## Deliverables

Write only under `client-c/verification/completion/gemini-audit/`:

- `minimap-coverage.csv`: behavior, direction, opcode, length/framing, original Java reference, server reference, native reference, code state (implemented/partial/missing), verification state (build-only/tested/live-verified/unverified), existing evidence, open question. Include the already implemented pieces as review rows.
- `minimap-findings.md`: five or fewer prioritized gaps, exact source lines, a small implementation proposal and acceptance scenarios. Separate source-derived expectations from behavior actually observed.
- `resume-status.md`: recovered Cursor/Grok artifacts and their location, completed items, unchecked claims, remaining items and where the next worker should start. If no prior artifacts were recovered, say so explicitly. Do not overwrite recovered originals.

Do not edit production code, saves, cache, launchers, .env files or credentials. Do not start/stop servers or clients. Do not claim execution or successful tests without logs. If a reference is absent or ambiguous, record the precise unresolved point. The main implementer will review every mapping and expected value before using it.

Use these current workspace instructions. Task briefs inside older checkpoint ZIPs are snapshots and may be stale.
