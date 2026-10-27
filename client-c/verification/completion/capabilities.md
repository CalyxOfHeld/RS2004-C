# Current status — software checking complete; listening pending

The finite FINISH-LINE.md software plan is complete for the declared Windows225 profile. Audio/settings software, gameplay/interfaces, actual server/two-client workflows andextended play are accepted; final package technical audit passes. The full goal is NOT complete until a human verifies Windows speaker/headphone music andsound start/stop/volume. No new software task is claimed while waiting. See D-RELEASE-CANDIDATE.md and releases/2026-10-25-gate5-final-sol/README.md for candidate/checkindex/knownlimits/listening steps. The installed normal client remains unchanged; no installation or replacement occurred.

Final candidate019714A43B7E21ED37E81E95EA2B45AA653BC65A78368FB96D4CAE3E291086A0. All81 freshly rebuilt objects/both executables/SDL2 reproduce SHA-exact from included555-file source archive. Gate4 discloses27m56s continuous plus8m03s supplement for natural stairs0->1->0,combatXP160/52 andnormal logout/relog/source CRC/items/stats/position. Same-candidate reconnect15 passed; memorybounded; no rendererfailure in final profile. Earlier failed candidate andhistoricalcamera exclusions retain their actuallimits. Alltestprocessesclosed,server4/5exit0;118main saves/828maps/normal/originals preserved.

---

## Historical checkpoint notes — superseded where they conflict with the current status above

# Initial capability audit

Historical snapshot: current overall position and evidence limits are indexed in [OVERALL-STATUS.md](OVERALL-STATUS.md) and [D-REMAINING.md](D-REMAINING.md). Old statuses/counts below are not fresh failures or current-suite totals.


Evidence level: source inspection only unless explicitly stated. Historical captures do not verify gameplay.

## Incoming protocol coverage

| Name | Opcode | Length | C framing | SDL dispatch | Reference |
| --- | --- | --- | --- | --- | --- |
| IF_OPENCHAT | 14 | 2 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:3 |
| IF_OPENMAIN_SIDE | 28 | 4 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:4 |
| IF_CLOSE | 129 | 0 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:5 |
| IF_SETTAB | 167 | 3 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:6 |
| IF_SETTAB_ACTIVE | 84 | 1 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:7 |
| IF_OPENMAIN | 168 | 2 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:8 |
| IF_OPENSIDE | 195 | 2 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:9 |
| IF_SETCOLOUR | 2 | 4 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:12 |
| IF_SETHIDE | 26 | 3 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:13 |
| IF_SETOBJECT | 46 | 6 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:14 |
| IF_SETMODEL | 87 | 4 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:15 |
| IF_SETRECOL | 103 | 6 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:16 |
| IF_SETANIM | 146 | 4 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:17 |
| IF_SETPLAYERHEAD | 197 | 2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:18 |
| IF_SETTEXT | 201 | -2 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:19 |
| IF_SETNPCHEAD | 204 | 4 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:20 |
| IF_SETPOSITION | 209 | 6 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:21 |
| TUT_FLASH | 126 | 1 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:24 |
| TUT_OPEN | 185 | 2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:25 |
| UPDATE_INV_STOP_TRANSMIT | 15 | 2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:28 |
| UPDATE_INV_FULL | 98 | -2 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:29 |
| UPDATE_INV_PARTIAL | 213 | -2 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:30 |
| CAM_LOOKAT | 74 | 6 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:33 |
| CAM_SHAKE | 13 | 4 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:34 |
| CAM_MOVETO | 3 | 6 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:35 |
| CAM_RESET | 239 | 0 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:36 |
| NPC_INFO | 1 | -2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:39 |
| PLAYER_INFO | 184 | -2 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:40 |
| FINISH_TRACKING | 133 | 0 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:43 |
| ENABLE_TRACKING | 226 | 0 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:44 |
| MESSAGE_GAME | 4 | -1 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:47 |
| UPDATE_IGNORELIST | 21 | -2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:48 |
| CHAT_FILTER_SETTINGS | 32 | 3 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:49 |
| MESSAGE_PRIVATE | 41 | -1 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:50 |
| UPDATE_FRIENDLIST | 152 | 9 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:51 |
| UNSET_MAP_FLAG | 19 | 0 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:54 |
| UPDATE_RUNWEIGHT | 22 | 2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:55 |
| HINT_ARROW | 25 | 6 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:56 |
| UPDATE_REBOOT_TIMER | 43 | 2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:57 |
| UPDATE_STAT | 44 | 6 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:58 |
| UPDATE_RUNENERGY | 68 | 1 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:59 |
| RESET_ANIMS | 136 | 0 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:60 |
| UPDATE_PID | 139 | 2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:61 |
| LAST_LOGIN_INFO | 140 | 9 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:62 |
| LOGOUT | 142 | 0 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:63 |
| P_COUNTDIALOG | 243 | 0 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:64 |
| SET_MULTIWAY | 254 | 1 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:65 |
| DATA_LOC_DONE | 20 | 2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:68 |
| DATA_LAND_DONE | 80 | 2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:69 |
| DATA_LAND | 132 | -2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:70 |
| DATA_LOC | 220 | -2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:71 |
| REBUILD_NORMAL | 237 | -2 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:72 |
| VARP_SMALL | 150 | 3 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:75 |
| VARP_LARGE | 175 | 6 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:76 |
| RESET_CLIENT_VARCACHE | 193 | 0 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:77 |
| SYNTH_SOUND | 12 | 5 | matches | branch present; behavior unverified | engine/src/network/game/server/ServerGameProt.ts:80 |
| MIDI_SONG | 54 | -1 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:81 |
| MIDI_JINGLE | 212 | -2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:82 |
| UPDATE_ZONE_PARTIAL_FOLLOWS | 7 | 2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:85 |
| UPDATE_ZONE_FULL_FOLLOWS | 135 | 2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:86 |
| UPDATE_ZONE_PARTIAL_ENCLOSED | 162 | -2 | matches | no branch in SDL loop | engine/src/network/game/server/ServerGameProt.ts:87 |
| LOC_MERGE | 23 | 14 | MISSING/DIFFERENT | no branch in SDL loop | engine/src/network/game/server/ServerGameZoneProt.ts:5 |
| LOC_ANIM | 42 | 4 | MISSING/DIFFERENT | no branch in SDL loop | engine/src/network/game/server/ServerGameZoneProt.ts:6 |
| OBJ_DEL | 49 | 3 | MISSING/DIFFERENT | no branch in SDL loop | engine/src/network/game/server/ServerGameZoneProt.ts:7 |
| OBJ_REVEAL | 50 | 7 | MISSING/DIFFERENT | no branch in SDL loop | engine/src/network/game/server/ServerGameZoneProt.ts:8 |
| LOC_ADD_CHANGE | 59 | 4 | MISSING/DIFFERENT | no branch in SDL loop | engine/src/network/game/server/ServerGameZoneProt.ts:9 |
| MAP_PROJANIM | 69 | 15 | MISSING/DIFFERENT | no branch in SDL loop | engine/src/network/game/server/ServerGameZoneProt.ts:10 |
| LOC_DEL | 76 | 2 | MISSING/DIFFERENT | no branch in SDL loop | engine/src/network/game/server/ServerGameZoneProt.ts:11 |
| OBJ_COUNT | 151 | 7 | MISSING/DIFFERENT | no branch in SDL loop | engine/src/network/game/server/ServerGameZoneProt.ts:12 |
| MAP_ANIM | 191 | 6 | MISSING/DIFFERENT | no branch in SDL loop | engine/src/network/game/server/ServerGameZoneProt.ts:13 |
| OBJ_ADD | 223 | 5 | MISSING/DIFFERENT | no branch in SDL loop | engine/src/network/game/server/ServerGameZoneProt.ts:14 |

## Cross-system coverage

| Capability | Initial state | Production evidence / gap | Gate |
| --- | --- | --- | --- |
| Session framing | Failing by inspection | protocol_poll_packet enters blocking reads after one byte; no output capacity; zone packet lengths absent | NET-01..04 |
| Entity state/render | Partial | SDL dispatch expects NPC opcode 134, server sends 1; rendered demonstration NPC is separate from synchronized NPCs | ENT-01 |
| Movement | Failing by inspection | SDL sends adjacent-step deltas and scene-local origins; server expects start-relative deltas and world coordinates | MOVE-01 |
| Interaction dispatch | Failing by inspection | Numeric action-ID ranges overlap cache component IDs; item slot hardcoded to 0; default sidebar ignores item action | UI-01..03 |
| Inventory receive | Partial | Production handlers exist; full updates do not clear trailing slots; malformed quantity payload handling needs audit | INV-01 |
| Combat/equipment | Unverified/incomplete | Dependent on complete widget/action/varp/inventory/appearance paths | GAME-01 |
| Terrain/materials | Partial | Cache textures are loaded; no matched Java-client scene comparison yet; blending/lighting incomplete | VIS-01 |
| Minimap | Partial | minimap_build fills tiles with one RGB; wall/mapscene/shaped-floor fidelity unverified | VIS-02 |
| World operations and zone changes | Incomplete | Several outgoing encoders exist; world picking/action chain and zone dispatch not complete | WORLD-01 |
| Social/audio/effects | Unverified/incomplete | Some assets/helpers exist; full protocol and gameplay loops unaudited | REMAIN-01 |
| Test validity | Mixed | test_net_sync assigns state directly; test_full_ui renders fixtures; neither proves real input/server round trip | QA-01 |

## Reference launch boundary

The existing javaclient/build/libs/rs2client.jar is present. sign/signlink.java:180 searches system folders for its cache. It has NOT been launched because that would violate the project-only file-access boundary. Prepare an isolated reference harness with an explicit in-project cache before running it. Source comparison is available; visual parity is unverified.
