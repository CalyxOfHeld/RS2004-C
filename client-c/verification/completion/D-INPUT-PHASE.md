# Revision-225 input processing and normal-build correction

Completed October 26, 2026, following the user's instruction to fix the remaining input differences.

## Changes

The native event handler previously dispatched interface clicks and inventory releases before `updateGame` read server packets. Original Java's GameShell records the latest press and current held button; game logic then processes them in a defined order. Some modal-message click consumption was already implemented and was preserved.

The revision-225 native loop now:

- Records the latest press and held-button state, while retaining every raw event for input tracking.
- Advances existing item drags and releases them after actor/cross updates, before CYCLELOGIC3. Movement thresholds are sampled at this phase rather than at every intermediate mouse-motion event.
- Handles normal presses after CYCLELOGIC3 and the previously selected walk result, then the deferred world operation, minimap and click counter.
- Consumes a drag-release or modal-dismissal click before later click processing. Initial drag presses start at cycle zero; a release at cycle five can swap the item IDs and counts together.
- Clears the added input state on a fresh login/logout.

The normal Makefile previously linked `client.o` with `RS2_SCENE225_RUNTIME=0`, while the qualified candidate used `client225.o`. The usual `test_client.bat` requested that normal target. This was a delivery mismatch: the normal launcher was not building the same loop/render path as the revision-225 candidate. Normal client and replay now link `client225.o`; the launcher name and workflow are unchanged. No user process was stopped or server content changed.

Two existing control harnesses that intentionally exercise isolated handlers now invoke the deferred mouse/drag phase explicitly. Their original expected states and bytes were unchanged. A missing dependency rule for the public-chat control was added so it rebuilds against current client code instead of silently retaining an old executable.

## Verification

- The pre-edit client snapshot fails the new check on case zero: event collection already sent an action packet. This is retained in `baseline.log` with exit1.
- A verifier-only copy of original `client.java` replaces only `updateGame` with its unchanged drag-through-chat-settings input slice, plus one local declaration originally outside that slice. All called mouse/minimap/tab/chat methods remain original. Real interface/font cache is loaded. This is an original input-phase reference, **not a full Java tick**.
- Ten real-cache native full-update cases match that reference's encrypted packet bytes/order, click counter, modal/drag state and menu visibility. Cases include normal clicks, modal dismissal, short drag release, a held drag, press/release within one collection, multiple presses, right clicks and no click.
- An additional queued inventory drag uses cached inventory geometry, holds through four cycles, releases on the fifth, swaps IDs/counts, and sends the original INV_BUTTOND tuple before CYCLELOGIC3. A newly latched press is consumed by that release.
- Existing mouse/settings, Continue, abuse-report, social actions, private/public sending and input-tracking controls all pass against rebuilt executables. Fixtures were not changed.
- The full registered `make test` suite passes. Final normal/candidate/replay builds and test builds have no compiler warnings. No broad live gameplay campaign was run.
- The actual normal executable completed a two-frame offline SDL software/dummy capture and clean shutdown. The image was inspected. This is a render/startup check, not live gameplay or physical audio acceptance.

## Evidence and limits

Evidence: `runs/2026-10-26-input-phase-sol/`. It contains before snapshots, `changes.patch`, `prepare_reference.py`, `InputPhaseOracle.java`, original phase text, reference fixture, baseline/current logs, seven affected control logs, suite/build logs, normal capture and SHA-256 identities. Run the reference preparation from the rewrite root, compile its generated `source/deob/client.java` and oracle against `javaclient/build/libs/rs2client.jar`, then run the oracle with the original cache directory. Build/run `bin/input_phase_candidate_control.exe` with the resulting fixture.

Retained setup failures: first Java build lacked its output directory; the next used incorrect deob API signatures and lacked a local variable from outside the extracted phase. Corrected compilation succeeds. An unquoted JVM property was parsed incorrectly by PowerShell; quoted invocation succeeds. The added inventory check initially encoded the CYCLELOGIC3 constant incorrectly; it was corrected to compare the source constant4991788 directly. Dummy SDL initially requested an accelerated renderer; explicitly selecting the software driver succeeds. These failed attempts are not counted as acceptance.

The installed normal executable is updated, SHA-256 `05046e6dadab75f30351d00ec504dfffbe7bda00c8b11b22fa7c2a73d8acfb56`. Use the usual `test_client.bat`. The old bridge executable is older and is not selected by the launcher. Earlier release archives remain obsolete. Historical whole-scene/recovery recordings were not rerun or relabelled as evidence for this binary. Physical audio, original external-host fade/timbre limits and the old synthetic renderer fixture retain their prior scope.
