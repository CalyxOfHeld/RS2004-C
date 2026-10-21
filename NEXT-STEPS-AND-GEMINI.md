# Paused checkpoint and next move

Paused October 21, 2026 at the user's request. No further feature implementation is running. The server is left available for manual testing. Last account usage check: 71% of the five-hour window used, 11% weekly. Write another runnable handoff at 95% used / 5% remaining if work resumes and reaches that threshold. The present handoff is written now.

## Resume order

1. Read the user's checkpoint feedback and reproduce/fix reported failures first. Preserve all accounts, history, cache and server behavior.
2. Complete minimap navigation from the bundled revision-225 reference: inverse click transform, original route serialization, destination marker/clear behavior, then natural route, cancellation and region-crossing evidence. Only preparation snapshots and a Makefile build refactor were made for this phase; minimap navigation has not been implemented.
3. Close remaining item-use/spell and connected combat/skilling/social/effect gaps in dependency order from AUTONOMOUS_PLAN.md. Unsupported zone families 23/42/69/191 need source-backed coverage.
4. Continue original-render comparisons for roofs, bridges, occlusion and ground piles, then sustained session checks. Passing the current suite does not establish complete parity.

## Work suitable for Gemini

These are bounded assignments with outputs that can be checked against local source. No Gemini session has been started and no GPT delegate has been used for this assessment. Model choice alone does not establish correctness.

| Assignment | Deliverable | Review boundary |
| --- | --- | --- |
| Protocol/interface coverage audit | CSV mapping original Java, server codecs and native handlers with exact file/line references | Read-only audit; mark unknowns explicitly |
| Checkpoint manual QA | Reproduction steps, screenshots/logs and observed quantities for bank/shop/doors/stairs | Separate disposable account; no save edits or server changes |
| Evidence/document audit | Scenario-to-evidence index, stale claims and proposed corrections | Distinguish actual server acceptance from fixture preparation |
| Cache/definition inventory | Missing item/NPC/animation coverage report from bundled real definitions | No invented assets or later-revision references |
| Small regression proposals | Boundary cases and independently derived expected values for a defined helper | Review before adding code; do not copy implementation as the oracle |

Start with the protocol audit in GEMINI-AUDIT-TASK.md. Give one narrow assignment at a time. Keep encryption/framing, entity rebasing, renderer ordering and audio arithmetic under close implementation review because errors can affect many systems. A later coding assignment should name one function, its reference behavior, allowed files and acceptance criteria.

## Checkpoint boundaries

Native executable SHA-256: 3035DF94DAFA31A55C038CB1F25DEC795118BD7513EBA85AA463B3ACF0077919.
The executable is the one used for the accepted live scenarios. The later Makefile refactor compiled shared objects once and passed the same 37-test suite; the packaged executable was retained rather than rebuilt after pausing.
Existing server PID 3128 owned ports 43595/8889/8899 at packaging. Verify ownership again before starting any server. New test saves stack211a, stack211b, bank211 and bank212 are persistent and must be preserved too.
No Git remote is configured; delivery is the local test ZIP and installed executable. No commit or remote push was made.
