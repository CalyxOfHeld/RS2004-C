# Gemini assignment: revision-225 minimap protocol audit

Produce a read-only audit to prepare the next native client task. Do not implement features. Read AGENT.md, the current top of HANDOFF.md, NEXT-STEPS-AND-GEMINI.md, and AUTONOMOUS_PLAN.md first.

Use only the bundled revision-225 Java client, TypeScript port and engine/content references. Do not use OSRS, later revisions, web guesses or generated behavior. Keep all access and outputs inside this workspace. Do not start agents or delegate to GPT.

## Scope

Trace main-scene walking versus minimap walking through original input handling, click-coordinate transformation, camera rotation/zoom, route selection, serialization and destination marker clearing. Locate the exact original/server/native definitions rather than trusting opcode numbers in prose. Explain where each extra minimap payload byte originates and which handler validates it. Identify the native input/render hooks still missing. Do not broaden into combat or rewrite routing.

## Deliverables

Write only under `client-c/verification/completion/gemini-audit/`:

- `minimap-coverage.csv`: behavior, direction, opcode, length/framing, original Java reference, server reference, native reference, implemented/partial/missing/unverified, existing evidence, open question.
- `minimap-findings.md`: five or fewer prioritized gaps, exact source lines, a small implementation proposal and acceptance scenarios. Separate source-derived expectations from behavior actually observed.

Do not edit production code, saves, cache, launchers, .env files or credentials. Do not start/stop servers or clients. Do not claim execution or successful tests without logs. If a reference is absent or ambiguous, record the precise unresolved point. The main implementer will review every mapping and expected value before using it.
