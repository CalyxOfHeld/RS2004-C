# Revision225 overhead chat acceptance — October 23, 2026

**Accepted for the controlled original/cache/candidate comparisons described here.** Current Codex engineering chat completed the resumed player/NPC overhead rendering goal. Real-server conversation and final combined long-session qualification remain open.

## Changes

- Added persistent50-slot overhead collection for local player, active remote players and active NPCs in original order, using actual visibility, public mode and case-insensitive friend/self rules. Self uses the actual local-player name rather than a possibly stale login label.
- Connected original world bounds/height projection and overlap placement, static/wave/scroll text, fixed/flash/glow colours, clipping and shadows using real b12 glyphs. Plain/wave draws retain the original tag-character behavior even though width calculation skips tags. Unknown style3 draws nothing when effects0; effects enabled draws static yellow.
- Candidate reads chat-effects clientcode6 and advances the overhead animation clock on ready scene draws, preserving it across login and wrapping signed32 bits explicitly. Loading does not advance it; repeated draws need no game tick.
- Preserved original postincrement style adjustment to the next slot. When styled50th speech would make original Java throw out of bounds, native returns false before any speech pixels, retaining the collected50-slot state. Persistent text is copied so unused slots do not retain dangling actor pointers.
- Existing registered test_chatbox extended. No new registered test program; the candidate integration executable is an opt-in diagnostic with an explicit Makefile target.

## Original comparison

Reference: unchanged bundled javaclient/build/libs/rs2client.jar, original client.draw2DEntityElements, projectFromGround and PixFont, real engine/data/pack/client/title b12. New OverheadChatOracle invokes those methods; no reimplementation supplies expected pixels. Fixture stores sparse differing pixels but native reconstructs and compares every viewport pixel, including background/shadow.

1426 frames, **243857408 viewport pixels per complete comparison**, all50 persistent slots and final clip exact. Original sparse changed total1193729; six expected styled50th exceptions become six safe native rejections. Native origin0,0 and actual candidate projection/render origin8,11 both pass independently; candidate checks outside-viewport background and framebuffer guards too. Counts are corpus sizes, not gameplay or completion percentages.

Coverage:

- 1040 combinations: colours0..12, styles0..3, timers150/149/101/100/99/51/50/49/1/0, effects0/1; varied scene cycles including negative and clipping.
- 256 wave cycles with mixed case, digits, spaces and tag characters.
- 40 ordered mixed local/remote/NPC frames: public modes0..3, both effect settings, friends/strangers/self/null name, invisible/null/empty actors and overlap.
- 36 cap frames:0/1/49/50/51/60 chats, three styles and both effect settings. Includes the six actual original exceptions.
- 54 world-boundary projection frames at x127/128/5999/6000/6008/6400/7000/13056/13057 and z127/128/5800/6400/13056/13057. Projection corpus uses flat terrain, camera6400,-500,5800,pitch256,yaw0. It is not an exhaustive terrain/camera proof.

## Candidate integration and native safety

Actual included client.c initializes real cache and runs its actual projection for the entire original fixture. Actual client_draw then proves setting6: style3/effects0 produces no speech, changing the real varp to1 produces252 verified yellow glyph pixels. Two draws advance scene cycle while game_ticks stays fixed; a loading draw holds it; INT32_MAX wraps to INT32_MIN. This is an offline diagnostic, not physical-window typing or server conversation.

Ten malformed native count/friend/text/colour/timer controls reject without pixel writes and keep guards intact. Separate copied expectations with first x position changed and first glyph colour changed are rejected with exit3. Original fixture remains unchanged. Native nonempty text or empty text with positive timer represents present Java speech; empty text with nonpositive timer represents absence. Arbitrary non-null empty Java speech at timer0, Unicode and pathological corrupted actor coordinates/metadata are not newly accepted.

## Fresh checks

Evidence package: runs/2026-10-23-overhead-chat-sol/.

| Evidence | Result |
| --- | --- |
| reproduce1-wrapper.exit / reproduce1/result.txt |0/PASS; regenerate unchanged original expectations, SHA equals bundled fixture, native and candidate controls pass |
| suite1.exit / suite1.log |0/ALL TESTS PASSED; current registered suite plus separate rs2client225/rs2replay225 builds; no compiler warning/error markers |
| build3.exit / build3.log |0; registered chatbox and candidate diagnostic, no warnings |
| strict-overhead/font/chatbox/legacy build logs and exits |0; -Wall -Wextra -Werror |
| private/public incoming and outgoing public supplemental builds |0, strict and current client source; no warnings |
| regression-private_message |2634 original states,8 candidate framed packets; wrapper exit0 |
| regression-public_chat |3237 original mask states,155 ticks/four actors,9 framed packets; wrapper exit0 |
| regression-public_send |3521 original input states,3295 candidate transmissions, native allocation/capacity faults; wrapper exit0 |
| regression-chat_display |144 images,31246848 pixels,21456 hover queries; wrapper exit0 |
| negative-controls.json |altered state and pixel expectation copies rejected with exit3 |

Reproduce after building current targets from client-c:

    make -j4 bin/test_chatbox.exe bin/overhead_chat_candidate_control.exe bin/rs2client225.exe bin/rs2replay225.exe
    powershell -NoProfile -ExecutionPolicy Bypass -File verification/completion/tools/overhead_chat_parity.ps1 -OutDir <fresh-output-directory>

The verifier requires a fresh output directory and restores its environment variable. It does not build or install automatically.

## Retained failures and diagnostic corrections

oracle-build1 failed five API compile errors; corrected diagnostic bind/bounds against unchanged Pix2D/PixMap (negative sentinel-657). oracle2 had no pixels because viewport centre was not initialized; oracle3 lacked overflow coverage because mixed cap actors reordered the styled actor. Diagnostic corrected viewport centre and ordered cap construction; oracle4 requires actual cases1426/changed pixels/six exceptions. Native expectations originate only from that actual original invocation. build2 emitted one misleading-indentation warning in the new reader; corrected, subsequent strict/full builds clean.

candidate1 failed scene draw because diagnostic build omitted the revision225 texture macro; rebuilt with both production candidate macros. candidate2/3 placed the test actor/camera outside its intended inspection rectangle; corrected diagnostic camera initialization and actor height. candidate4 and reproducible candidate pass. These failures and logs remain retained; no production mismatch was hidden by changing expected outputs.

## Preservation and limits

All118 save hashes match the previous accepted public-send baseline. Normal installed SHA A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4; unchanged original jar SHA57EF9307E2B31674CB55F0F48C1643CDED751E1A97D5035B28F75DB7FD1E6CC7. Production43595/PID17880 still listening. Source/before snapshots/task-only diff/identities and four separate binaries packaged. Candidate diagnostics finish and isolated regression sockets close; no credentials or background jobs retained. No install/restart/commit/deletion/delegation.

The full historical C5/recovery screen corpora were not rerun for these changes. Installed normal client does not receive the candidate addition. Incoming handlers, public sender and overhead renderer now have separate controlled acceptance; private/social sending and real-server public conversation still need work. PhaseD continues; PhaseE remains pending.
