# Revision225 sound-queue lifecycle — October 25, 2026

Follow-up: [D-MUSIC.md](D-MUSIC.md) qualifies music settings andcontrolled shared sound/MIDI requests;physical listening remains pending. Earlier scope statements below remain historical tothis report.

The separate candidate now follows the original queue's tick delays, strict overlap/wall-clock decision, same-track replay, one busy retry at delay-5, exception/drop ordering and count-only normal-login reset. Reconnect retains the full queue and last-sound metadata. Controlled original/native/candidate acceptance is complete; physical device timing and audibility are not claimed.

Evidence: [sound-queue run](runs/2026-10-25-sound-queue-sol/). Remaining work follows the [finite acceptance plan](FINISH-LINE.md). Phase D is active and E pending. Source routes remain71/71; this is not overall completion.

## What changed

A shared C99 queue core retains all50 slots, including inactive tail entries and the original last-id/loops/length/start state. It uses the original strict `now + length/22 > previous_start + previous_length/22` comparison, Java signed64-bit wrapping addition and exactly the source clock-call order. New generations rejected by overlap drop; same-track replay does not generate or read the clock. Busy requests retry once at-5; exceptions drop with the original preceding metadata updates retained.

The actual client ticks sound after packet reads/recovery and before tracking flush. Thus a newly received sound starts its delay countdown in that cycle. Off suppresses admission while an already queued sound continues. Normal replies2/18 clear only the count;15 retains everything. Framed142 logs out before sound processing and retains the unticked queue until a later normal login. Cold initialization and shutdown reset the native bridge.

The SDL bridge has a bounded50ms worker and five RAM save slots. It preserves busy acknowledgement, slot rotation, replay and the borrowed generated-buffer alias: a later completed generation can replace the pending bytes before service. A mutex serializes generation/service; shutdown stops and joins the worker before freeing sounds. SDL output uses the existing accepted gain conversion. Resource-failure/headless operation services at tick end when there is no worker.

Sound uses the existing recorded wall-clock boundary. Main-thread AQ records retain request kind, length, full generated-WAV hash and acceptance outcome; replay consumes those records and verifies each tick's entire queue state without consulting a live clock or worker. This qualifies logical queue and generated request/outcome replay, rather than physical PCM playback reconstruction.

## Current evidence

| Requirement | Evidence and result |
| --- | --- |
| Source fidelity | `reproduce-final/source/deob/client.java` retains every other original method and replaces updateGame only with its exact contiguous queue slice. Four external call boundaries are substituted: generation, two clock calls, save and replay. Real save/replay delegates remain original except controlled exception injection. `queue-slice.diff` records the extraction. |
| Queue state/order | `reproduce-final/reference.bin` regenerates SHA-exact from original source. Native and actual client_update match2,326 complete50-slot states and ordered generation/data-hash/time/save/replay traces. Cases cover positive/due/negative delays, strict overlap equality, signed clock wrap, busy retry/drop, exceptions, multiple entries, compaction and stale tail. |
| Real cache |1,740 cases generate all435 available tracks with loops0/1/2/255 and compare full WAV hashes. The original/native controlled LCG noise input is fixed; this is a test input, not a production RNG change. |
| Original worker | `reproduce-final/backend.bin` regenerates SHA-exact from the original signlink worker with only cache/UID/file-output/sleep boundaries isolated.67 request/service states check five slots, wrap, busy, empty replay, null save, oversize rejection and pending shared-buffer overwrite, including rejected-save alias effects. Native actual slot bytes and captured SDL output match. |
| Worker lifetime | Actual SDL worker starts once, services a pending request and joins on close. Output/device calls are captured. |
| Recorded timing/outcomes | Three actual candidate ticks roundtrip with two recorded wall-clock reads, three AQ outcomes/full WAV hashes and all50 slots/metadata per tick. A deliberately wrong live clock remains unused. |
| Actual integration | Framed12 same-cycle countdown,150 Off admission, injected native normal2/mod18 count-only reset,15 retention and actual framed142 logout retention pass in an isolated loopback. Injected login replies are not authentication or server/account qualification. |
| Native guards | Null/invalid counts, full enqueue retention and empty callback suppression pass inside the existing registered test_audio binary. No registered binary is added. |
| Negative controls | `negative-final/`: fresh-process replay and backend positives pass first; eight copied count/time/inactive-tail/generated-hash/slot-hash/request-hash/outcome/clock corruptions reject3. Both jar-first wrong-class guards reject1 without writing fixtures. |
| Build/regressions | `suite-final` passes the full registered suite and current playable/replay candidates. `compat-build-final` rebuilds affected opt-in controls and four manually linked diagnostics. Five strict translation units pass-Werror; opt-in candidate rules also enforce it. All16 regression-final wrappers exit0: incoming private/public, outgoing public/private, social, report, request, display, overhead, receive batch, map, last-login, tracking, Continue, mouse mode and wave setting. |
| Preservation/package |118 saves,828 maps, installed normal, original jar and16 named original source/cache identities match the previous accepted baseline. See preservation JSON, identities, task-only.diff, current snapshots, separate binaries and completion-audit.json. |

Reproduce from client-c with PowerShell7:

```powershell
./verification/completion/tools/sound_queue_parity.ps1 -OutDir <new-evidence-directory>
```

Build the current test_audio and sound_queue_candidate_control first. The verifier refuses an existing evidence directory and never rewrites fixtures. `negative-final.py` records the copied-corruption checks. Initial candidate2 exit3 was an observer assertion that incorrectly required ingame after the intentional logout; candidate3 and reproduce-final pass after that harness correction. The initial strict compiler invocation used a missing executable path; strict-final uses the available compiler and passes. Failed evidence is retained. Whole-tree git diff --check exits2 on CRLF/trailing-whitespace findings across the already dirty tree; task-only added lines have zero horizontal trailing-whitespace errors (task-diff-check.json). No unrelated formatting cleanup was attempted. Java reports its existing deprecated-API note; current C build logs have no warnings. Credential-gated live login skips authentication and CRC passes. Historical C5/recovery image corpora were not rerun.

## Adapter choices and remaining limits

The native bridge uses five RAM slots instead of Java temporary WAV files; it does not write cache/saves. Generated native requests are bounded to441,000 bytes. The worker mutex does not emulate unsafe partially torn Java array races; the generator's existing safety guards can avoid Java partial writes on oversize exceptions. Logical queue drops and the tested completed-generation alias effects match. No arbitrary unsafe-buffer side-effect parity is claimed.

The original signlink worker shares request priority/storage with MIDI/DNS/cache operations. The sound-only request/service controls do not qualify that full shared scheduling. Music/clientcode3 and ordinary sound/music coexistence are part of finite gate1. The no-worker fallback has different physical service pacing. AQ replay does not recreate asynchronous audible output, hardware latency, live attenuation of already submitted SDL buffers or arbitrary device behavior. The later gates cover ordinary combined gameplay and a short human device check.

Installed normal SHA256: A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4. Original jar:57EF9307E2B31674CB55F0F48C1643CDED751E1A97D5035B28F75DB7FD1E6CC7. No installation, commit, production server/account, user-file deletion, delegation or human messaging occurred. Production43595 is absent at the final check. No further goal is claimed by this acceptance.
