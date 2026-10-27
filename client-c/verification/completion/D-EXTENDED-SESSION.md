# Gate4 accepted — October25,2026

The fixed software gate is accepted by runs/2026-10-25-extended-session-sol/completion-audit.json (20checks). Final019714 candidate ran27min56sec continuously, stopped for the mandatory usage floor, then the same saved character/candidate completed an8min03sec follow-up and normal logout/relog. The approximately30minute criterion is met with its actual measured duration disclosed; there is no exact30minute or uninterrupted36minute claim. Earlier incomplete checkpoint notes below are historical.

Natural stairs1738/1739 completed0->1->0. Rat combat gained rawAttackXP160/HitpointsXP52. Normal logout2458 produced the server logout packet/title reset; fresh login retained position3214,3210,plane0, sword1277 worn, tinderbox590,axe1351,coins100 and all stats. Original source decoder/CRC verifies the final1920-byte save (SHA1f63e68c4577b0320a20e15411a628ecd47e3dd8f2ca1ed3999a9e2d4c0efcf6). No fixture or teleport changed this character during the follow-up. Actual screenshots are followup/review-images; source-states/events and saved-state.log provide authoritative state.

The continuous client private memory rose331452416->335765504 bytes during warmup (peak335790080); last25minutes retained292handles and no nonresponding samples. Client165/server165 ten-second samples show bounded windows, not unbounded growth. Planned disconnect produced the sole receive-error line, reconnect reply15, then ordinary walking. Follow-up stderr contains only one retained helper no-resolved-cursor attempt; corrected later normal actions succeeded. Original helper1738 had selected the wrong staircase before; the corrected south-door approach resolves actual natural stairs. Initial can't-reach paths/failed helper attempts are excluded, not application failures hidden as passes.

Final deferred-frame source correction/full suite remain accepted. First A141 error6/kind1 causal state is unproved; final candidate does not reproduce it in this declared profile. Historical error6/kind3 remains its exact earlier excluded fixture, not a blanket-fix claim. All test processes closed normally; server4/server5 exit0; ports terminal;118main saves/828maps/normal/jar/pinned originals freshly preserved. Physical audio listening remains pending before full completion. Gate5 final package audit is next; no install or normal replacement.

---

# Revision225 combined extended session — October 25, 2026

Status: usage-paused after1676.06seconds/27min56sec. Gate4 remains unaccepted: stairs/combat/final relog are unfinished. All test processes closed normally; server4 exit0. Attempt2 had no renderer failure; its sole receive error was the planned disconnect followed by reply15 recovery. Read the latest run HANDOFF.md.

Fixed scenarios were declared before execution in `runs/2026-10-25-extended-session-sol/SCENARIOS.md`. Attempt2 uses the normal candidate019714A43B7E21ED37E81E95EA2B45AA653BC65A78368FB96D4CAE3E291086A0, ordinary disposable g4solb, original source/server/profile and initial3217,3239. Only source-save fixture setup occurs outside ordinary gameplay. The original installed executable, accounts, main saves and assets remain protected.

## Retained first failure and correction

The first A141 candidate session stopped after666.75seconds with `[Scene225] Draw failed error=6 kind=1`. Last complete capture was29750, world3221,3222. That run is failed evidence, not a partial30-minute pass. Ordinary tree/fire/equipment and walking had completed. Four separate diagnostic renderer runs did not reproduce the stop; their camera/origin variants are recorded and excluded from acceptance. The printed kind is the last callback, not a diagnosis of the failing model.

Source inspection found an independent relevant mismatch: native scene_raster225.c rejected a normal traversal return whenever tiles_remaining was nonzero. Unchanged original World3D.draw:1355–1363 returns normally after both sweeps even with deferred tiles, retaining takingInput. DeferredFrameOracle executes that unchanged full draw method with an explicitly deferred drawTile fixture boundary and confirms remaining1/inputtrue/normal return. The native before control rejects with error6; the corrected control matches the source contract while still rejecting actual observer and invalid-input failures. The added fatal assertion was removed; actual traversal failures remain fatal. This is source-backed correction, not permission to hide failures.

Direct causation of the first live stop has not been captured, and no blanket fix for historical error6/kind3 is claimed. The fresh combined run must pass before this gate can be accepted. The final registered suite passed after the correction, including the original painter-command/frame checks. TEST_BINS is unchanged.

## Test-control boundaries

Normal SDL dummy/software candidate, original input tracking and normal mouse events are used; no activity platform and no disabled tracking. The controller appends operator mouse movement every20seconds and routes requested normal UI actions. Its initial follow-chunk timing mistake in attempt1 skipped the first walk click. The corrected controller leaves a full10-frame reader-poll gap between chunks. An initial wrong-PID monitoring setup in attempt2 ended without samples and is excluded; the corrected monitor observes actual client12272/server17540.

Walking requests calculate ordinary minimap click coordinates from observed native state and its actual orbit yaw. Early attempts used the drifting rendered yaw or aimed beyond closed castle doors; those attempts are retained and do not prove completed travel. Actual visible Open actions for the two closed double-door pairs produce original source handler acceptance and the expected native scene updates1517/1520. Their internal names containing “open” are not their configured action: the original definitions specify Open/closed categories. No teleport or collision override is used.

## Evidence and remaining checks

Attempt2 began20:11:06 EDT. At the latest checkpoint, actual tree chop/fire and remove/re-equip, all six settings controls and natural walking through populated scenes/door opening completed. No native errors have been reported; client private memory reached a stable approximately319MiB/292handles window. Stairs/plane changes, rat combat, controlled reconnect, final logout/relog, approximately30minutes of wall-clock operation and final resource/preservation audit remain pending.

`attempt2/session_evidence.py` summarizes the original server4 JSON states/accepted handlers, actual transport-fault event, native errors/reconnect logs and10-second resource samples. `controller-events.jsonl` records every scheduled input; completed capture names drive scheduling. Input-follow is not claimed as a deterministic full-session recording. The single planned TCP fault invokes socket.destroy only on g4solb; original TCP close/reconnect handlers determine all subsequent state. Isolated ports43596/45096/43599 bind loopback; production43595 is untouched.

Final candidate packaging is staged separately in `releases/2026-10-25-gate5-final-sol`. Its included source archive has been cleanly extracted and rebuilt: all81 compiled objects, both executables and SDL2.dll reproduce SHA256-exact. This does not itself accept the extended session or physical audio. Installation/replacement requires a separate user direction.
