# Revision225 connection recovery accepted — October 23, 2026

The C candidate now presents the retained connection-lost frame and reconnects synchronously inside its update. The old socket remains owned until the new login attempt completes. Successful recovery continues that same update; rejection returns to the title screen. Initial title login remains unchanged.

A fresh disposable live session survived two client-only disconnects. Strict native replay reproduces1800 frames. The unchanged original client matches all1799 final game screens and outgoing protocol bytes, both intermediate connection-lost presentations, and all three login RNG states without recovery reseeding. The comparison uses an explicit controlled original receive IOException at the recorded native loss boundary; it does not claim equal real TCP EOF detection latency.

## Evidence

| Requirement | Result |
| --- | --- |
| Accepted live recoveries |2; original protocol/final frames/intermediate images exact |
| Repeated outcome controls |12 native and12 original cases;10 accepted including2 retries,2 rejected |
| Old socket and update ordering |Actual client-loop controls check lifetime through handshake and same-update counters/idle continuation |
| Settings after reconnect |Original varCache restoration implemented; signed/large/predicted/idempotent/malformed controls pass |
| Regressions |Full registered suite0, no compiler warnings/errors; legacy client Werror compile0 |
| Verifier negative controls |Altered intermediate P hash rejects at first loss; altered login opcode rejects |
| Preservation |118 starting saves unchanged; normal client/original jar hashes unchanged; server43595/PID17880 reused |

Final reusable tool result: runs/2026-10-23-phaseD-varcache-sol/live/reusable-final2/result.txt. Its original log contains12 PASS ORIGINAL RECOVERY OUTCOME records. Full requirement audit, source snapshots/diff, candidate binaries/hashes and save evidence: runs/2026-10-23-phaseD-acceptance-sol/.

## Changes

- Added immediate platform presentation and strict recorded P events, including replay image capture.
- Moved recovery login into update and retained old stream until completion, including receive watchdog ordering.
- Preserved server-cached settings separately from predicted local values. Original opcode193 restores those settings rather than zeroing them; this avoids extra brightness RNG consumption after reconnect.
- Added reusable recovery comparison tooling: reference/src/RecoveryParityOracle.java, RecoveryTransport.java, RecoveryOutcomeControls.java and tools/recovery_parity.ps1. The initial-login-only oracle keeps its explicit unsupported-recovery guard.

## Reproduction and limits

Build candidate/replay with their separate225 targets. Record a fresh disposable session using tools/record_live.ps1, the saved two-drop proxy and input script in phaseD-varcache-sol/live/. Keep the disposable password only in the process environment until comparisons finish. Run tools/recovery_parity.ps1 with -Log, a new -OutDir, and optional -StateControls pointing at phaseD-recovery-outcomes-final-sol. The wrapper compiles the adapter, strictly replays, compares original I/O/presentations/final regions and audits per-login RNG. Outcome fixture sync/pw and zero CRCs are public isolated loopback test setup, not production credentials. Old live passwords were cleared; rerun with new credentials rather than altering recorded expectations.

Original TCP clean EOF reports available0 and may reveal failure on a later read/write; controlled-I/O parity is a distinct, stated contract. Independent outcome controls compare state/handshakes, not unseeded title flames. This is bounded revision225 recovery acceptance, not exhaustive world/extended-session coverage or completion of every PhaseD feature. The optional registered credential-gated login check skipped; the separate live record above ran. The candidate was not installed.

All failed evidence remains: old deferred-recovery corpus, diagnostic hash-formula mistake, pre-varCache divergence, initial PowerShell launcher restriction and repeated-control missing-old-stream setup. Corrected final controls pass with original sources/expectations unchanged. Diagnostic shell61705 exited0 after clearing both current and retained prior disposable credentials. No commit, restart, installation, deletion or delegation occurred.
