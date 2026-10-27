# AGENT DIRECTIVES & REWRITE RULES

## VERSION CONTROL
- This repository uses Git from the repository root. Check `git status` before editing and review your diff before finishing.
- Do not rewrite shared history or discard existing user changes. Commit only when the user asks.

## ACTIVE TASK HANDOFF
- Read `WORK-QUEUE.md` for current priorities, ownership, acceptance gates and the update protocol. It supersedes historical next-task wording.
- Read the newest section of `HANDOFF.md` and `client-c/verification/completion/progress.md` for current progress and the next task. Older sections are historical.
- Read `SOURCE-INVENTORY.md` for the additional workspace directories and the distinction between the original Java client, TypeScript port, native C client, server and supporting tools.

## ENGINEERING AND SUPPORT ROLES — USER DIRECTION, OCTOBER 22, 2026
- Engineers: the user's current Codex engineering chat and Claude. They own implementation, technical design, code review, and changes to source, tests, scripts and build configuration.
- Other model sessions may run existing approved scripts/tests, inspect logs, collect evidence, organize artifacts and update factual documentation. They may not author or modify code, tests, scripts, build configuration or patches. Earlier implementation assignments to other models are superseded.
- Support workers report failures with exact commands, exit status, evidence paths and observed differences. Engineers diagnose and fix them. A support task involving a needed code change returns to an engineer.
- Before work, read the queue and claim one bounded task with session identity and status. At every meaningful checkpoint, update `WORK-QUEUE.md`; synchronize `HANDOFF.md`, `CLAUDE-HANDOFF.md` and `client-c/verification/completion/progress.md` when evidence, limits or the next task changes. Record which checks actually ran; retain failed evidence and distinguish historic results from fresh results.
- Do not start duplicate work or silently reassign ownership. No model may infer engineering permission from its name or an old handoff. The current 5% weekly checkpoint parks new implementation until the user resumes it; documentation and planning may continue.

## STRICT GROUND RULES - ZERO EXCEPTIONS

### 1. NO INVENTING OR GUESSING
- **NEVER invent dialogue, mechanics, formulas, animations, packets, opcodes, or features under any circumstances.**
- **NEVER guess data structures, packet formats, or logic.**
- If any detail is unclear or unencountered, inspect the local source code directly (`javaclient/src/main/java/` and `engine/src/`). If it is not present in the local codebase, ask the user or leave an explicit TODO referencing the exact ambiguity.

### 2. REVISION-SCOPED SOURCES
- **DO NOT USE OSRS (Old School RuneScape) sources, references, wikis, or modern protocols.**
- The **current active target is Revision 225 (May 18, 2004)**. Do not use later-revision sources or behavior to implement or validate 225.
- All client logic must be 100% sourced from:
  1. `javaclient/src/main/java/` (the deobfuscated Revision 225 client)
  2. `webclient/src/` (TypeScript client port)
  3. `engine/src/` (LostCity Revision 225 game engine server)
- When work on a later revision begins, first identify that revision's own client, server, cache and tests. Keep its implementation and evidence separate from 225; never silently mix behavior between revisions.

## VERSION ROADMAP
- Build and verify **225 first**. The planned order after 225 is **244**, **245.2**, **254**, then **274**.
- **289 is still in development by the 2004scape project maintainer.** Treat it as a future possibility, not an active or completed target. Do not invent its behavior or start porting it from incomplete or unrelated sources.
- This roadmap does not change the current 225 acceptance criteria. Advance the active target only when the user directs work on the next revision.
- Preserve original 225 visual behavior even when it looks odd. In particular, the green shoreline markings on the minimap also appear in the bundled 225 Java client; the user chose to leave them.

### 3. VERIFIED BIT-FOR-BIT PARITY
- Every module ported to C must be validated against real cache pack files in `engine/data/pack/client/` or against live server interaction (`127.0.0.1:43594`).
- All tests must pass cleanly with 0 warnings and zero regressions.

### 4. ARCHITECTURAL & ENVIRONMENT GUIDELINES
- Target platform: Pure C99 with SDL2 and Winsock2 on Windows (MinGW/GCC).
- Console ports (Wii / Wii U) are deferred for later phases and must NOT distract from core PC C99 parity.
- Keep dependencies minimal: no heavy external cryptographic or utility libraries when a clean, portable C implementation suffices.
