# AGENT DIRECTIVES & REWRITE RULES

## VERSION CONTROL
- This repository uses Git from the repository root. Check `git status` before editing and review your diff before finishing.
- Do not rewrite shared history or discard existing user changes. Commit only when the user asks.

## ACTIVE TASK HANDOFF
- See `HANDOFF.md` for the current progress, completed milestones, and immediate focus (3D ground terrain/land visibility in `world3d.c`).

## STRICT GROUND RULES - ZERO EXCEPTIONS

### 1. NO INVENTING OR GUESSING
- **NEVER invent dialogue, mechanics, formulas, animations, packets, opcodes, or features under any circumstances.**
- **NEVER guess data structures, packet formats, or logic.**
- If any detail is unclear or unencountered, inspect the local source code directly (`javaclient/src/main/java/` and `engine/src/`). If it is not present in the local codebase, ask the user or leave an explicit TODO referencing the exact ambiguity.

### 2. STRICTLY NO OSRS OR LATER RS2 SOURCES
- **DO NOT USE OSRS (Old School RuneScape) sources, references, wikis, or modern protocols.**
- **DO NOT USE post-May 2004 RS2 sources or revisions (e.g. 317, 377, 459, etc.).**
- This codebase is strictly targeted at **Revision 225 (May 18, 2004)**.
- All client logic must be 100% sourced from:
  1. `javaclient/src/main/java/` (the deobfuscated Revision 225 client)
  2. `webclient/src/` (TypeScript client port)
  3. `engine/src/` (LostCity Revision 225 game engine server)

### 3. VERIFIED BIT-FOR-BIT PARITY
- Every module ported to C must be validated against real cache pack files in `engine/data/pack/client/` or against live server interaction (`127.0.0.1:43594`).
- All tests must pass cleanly with 0 warnings and zero regressions.

### 4. ARCHITECTURAL & ENVIRONMENT GUIDELINES
- Target platform: Pure C99 with SDL2 and Winsock2 on Windows (MinGW/GCC).
- Console ports (Wii / Wii U) are deferred for later phases and must NOT distract from core PC C99 parity.
- Keep dependencies minimal: no heavy external cryptographic or utility libraries when a clean, portable C implementation suffices.
