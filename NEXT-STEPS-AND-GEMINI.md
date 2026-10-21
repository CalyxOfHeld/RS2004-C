# Current next move and external-agent work

Updated October 21, 2026. After the test checkpoint, the user authorized one small task at a time. Six small changes are complete; no new implementation task is running during this documentation update. The server was left available for manual testing. Last account usage check:81% of the five-hour window used,13% weekly. Write the next full runnable handoff at95% used/5% remaining, as requested. Current progress and run notes supersede the older paused handoff where they differ.

## Resume order

1. Read the user's checkpoint feedback and reproduce/fix reported failures first. Preserve all accounts, history, cache and server behavior.
2. Complete minimap click navigation from the bundled revision-225 reference: input hit area/inverse transform and original route serialization. Scene-click destination rendering/arrival/packet19/rebase is now implemented, as are dot coordinate rounding and ordinary player dots. Approach routes now update the shared flag after a successful nonempty send. Friend colouring needs synchronized social state. All recent changes need scoped verification before claiming runtime parity; natural route, cancellation and region-crossing evidence remains open.
3. Close remaining item-use/spell and connected combat/skilling/social/effect gaps in dependency order from AUTONOMOUS_PLAN.md. Unsupported zone families 23/42/69/191 need source-backed coverage.
4. Continue original-render comparisons for roofs, bridges, occlusion and ground piles, then sustained session checks. Passing the current suite does not establish complete parity.

## Work suitable for Gemini

These are bounded assignments with outputs that can be checked against local source. The user reports that Cursor/Grok partially worked on the audit and then exhausted usage. Partial results have not yet been located or reviewed here; the expected gemini-audit directory is empty. Recover its files or last response before assigning duplicate work. The original Gemini-named brief remains the continuation entry point for compatibility. Model choice alone does not establish correctness.

| Assignment | Deliverable | Review boundary |
| --- | --- | --- |
| Protocol/interface coverage audit | CSV mapping original Java, server codecs and native handlers with exact file/line references | Read-only audit; mark unknowns explicitly |
| Checkpoint manual QA | Reproduction steps, screenshots/logs and observed quantities for bank/shop/doors/stairs | Separate disposable account; no save edits or server changes |
| Evidence/document audit | Scenario-to-evidence index, stale claims and proposed corrections | Distinguish actual server acceptance from fixture preparation |
| Cache/definition inventory | Missing item/NPC/animation coverage report from bundled real definitions | No invented assets or later-revision references |
| Small regression proposals | Boundary cases and independently derived expected values for a defined helper | Review before adding code; do not copy implementation as the oracle |

Searching for Cursor/Grok's partial output is deferred at the user's request. Revisit it only when the user supplies its location/output or requests recovery. No external audit results have been reviewed or accepted in this chat yet. Follow GEMINI-AUDIT-TASK.md when the audit resumes: review the current scene/approach destination implementations and focus remaining proposals on minimap click serialization. Keep one narrow assignment active at a time. Encryption/framing, entity rebasing, renderer ordering and audio arithmetic need close implementation review because errors can affect many systems. A later coding assignment should name one function, its reference behavior, allowed files and acceptance criteria.

## Checkpoint boundaries

Latest installed native SHA-256: `2BB6B9E47EBA7C008A89B759429AB17ADE7979586296D9351812A9D881DAF104`. Latest cumulative update: `song-packet-client-update.zip`, with matching .sha256 sidecar. It includes MIDI failure reporting, dot rounding, ordinary player dots the scene-click flag operation-approach flag integration and MIDI song packet framing. All six built with zero warnings; tests/live checks were not rerun for these updates.

The earlier world-loops-test-checkpoint.zip preserves SHA-256 `3035DF94DAFA31A55C038CB1F25DEC795118BD7513EBA85AA463B3ACF0077919`, the executable used for the accepted live scenarios. Its37-program suite and the later Makefile shared-object suite passed. Those results do not certify the newer binary. Read the latest task instructions from the workspace; copies in older ZIPs are historical snapshots.
Existing server PID 3128 owned ports 43595/8889/8899 at packaging. Verify ownership again before starting any server. New test saves stack211a, stack211b, bank211 and bank212 are persistent and must be preserved too.
No Git remote is configured; delivery is the local test ZIP and installed executable. No commit or remote push was made.
