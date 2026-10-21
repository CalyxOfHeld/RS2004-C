# Native coverage of the revision-225 Java client

Generated 2026-10-21 by `tools/coverage_map.py` from source alone. A present branch/encoder/function means native code exists, not that it is a faithful or verified port; see `test-grades.md` for what is actually verified.

## Server packets handled by `client.read()`: 44 of 62 have a native branch

| Opcode | Name | client.java | Native |
| --- | --- | --- | --- |
| 20 | DATA_LOC_DONE | 10122 | **absent** |
| 21 | UPDATE_IGNORELIST | 10689 | **absent** |
| 41 | MESSAGE_PRIVATE | 10485 | **absent** |
| 43 | UPDATE_REBOOT_TIMER | 9883 | **absent** |
| 46 | IF_SETOBJECT | 10625 | **absent** |
| 74 | CAM_LOOKAT | 10555 | **absent** |
| 80 | DATA_LAND_DONE | 9889 | **absent** |
| 87 | IF_SETMODEL | 10533 | **absent** |
| 103 | IF_SETRECOL | 10364 | **absent** |
| 132 | DATA_LAND | 10464 | **absent** |
| 133 | FINISH_TRACKING | 10226 | **absent** |
| 136 | RESET_ANIMS | 10668 | **absent** |
| 140 | LAST_LOGIN_INFO | 10281 | **absent** |
| 152 | UPDATE_FRIENDLIST | 9834 | **absent** |
| 220 | DATA_LOC | 10206 | **absent** |
| 226 | ENABLE_TRACKING | 10258 | **absent** |
| 239 | CAM_RESET | 10697 | **absent** |
| 254 | SET_MULTIWAY | 10331 | **absent** |
| 1 | NPC_INFO | 9905 | branch |
| 2 | IF_SETCOLOUR | 10658 | branch |
| 3 | CAM_MOVETO | 10429 | branch |
| 4 | MESSAGE_GAME | 10590 | branch |
| 7 | UPDATE_ZONE_PARTIAL_FOLLOWS | 10357 | branch |
| 12 | SYNTH_SOUND | 10336 | branch |
| 13 | CAM_SHAKE | 10768 | branch |
| 14 | IF_OPENCHAT | 10404 | branch |
| 15 | UPDATE_INV_STOP_TRANSMIT | 10271 | branch |
| 19 | UNSET_MAP_FLAG | 10138 | branch |
| 22 | UPDATE_RUNWEIGHT | 10760 | branch |
| 25 | HINT_ARROW | 10066 | branch |
| 26 | IF_SETHIDE | 10682 | branch |
| 28 | IF_OPENMAIN_SIDE | 10153 | branch |
| 32 | CHAT_FILTER_SETTINGS | 10376 | branch |
| 44 | UPDATE_STAT | 10734 | branch |
| 54 | MIDI_SONG | 10103 | branch |
| 59 | ? | 10148 | branch |
| 68 | UPDATE_RUNENERGY | 10547 | branch |
| 84 | IF_SETTAB_ACTIVE | 10582 | branch |
| 98 | UPDATE_INV_FULL | 10238 | branch |
| 126 | TUT_FLASH | 10305 | branch |
| 129 | IF_CLOSE | 10705 | branch |
| 135 | UPDATE_ZONE_FULL_FOLLOWS | 10444 | branch |
| 139 | UPDATE_PID | 10143 | branch |
| 142 | LOGOUT | 10117 | branch |
| 146 | IF_SETANIM | 10187 | branch |
| 150 | VARP_SMALL | 9814 | branch |
| 162 | UPDATE_ZONE_PARTIAL_ENCLOSED | 10750 | branch |
| 167 | IF_SETTAB | 10194 | branch |
| 168 | IF_OPENMAIN | 10637 | branch |
| 175 | VARP_LARGE | 10172 | branch |
| 184 | PLAYER_INFO | 10800 | branch |
| 185 | TUT_OPEN | 10540 | branch |
| 193 | RESET_CLIENT_VARCACHE | 10522 | branch |
| 195 | IF_OPENSIDE | 10385 | branch |
| 197 | IF_SETPLAYERHEAD | 10060 | branch |
| 201 | IF_SETTEXT | 10724 | branch |
| 204 | IF_SETNPCHEAD | 10349 | branch |
| 209 | IF_SETPOSITION | 10419 | branch |
| 212 | MIDI_JINGLE | 10318 | branch |
| 213 | UPDATE_INV_PARTIAL | 10781 | branch |
| 237 | REBUILD_NORMAL | 9911 | branch |
| 243 | P_COUNTDIALOG | 10263 | branch |

## Client packets sent by the Java client: 46 of 71 have a native encoder

| Opcode | Name | Java send sites | Native |
| --- | --- | --- | --- |
| 7 | ANTICHEAT_OPLOGIC1 | 1 | **absent** |
| 11 | FRIENDLIST_DEL | 1 | **absent** |
| 17 | ANTICHEAT_OPLOGIC7 | 1 | **absent** |
| 30 | ANTICHEAT_OPLOGIC3 | 1 | **absent** |
| 66 | ANTICHEAT_OPLOGIC6 | 1 | **absent** |
| 79 | IGNORELIST_ADD | 1 | **absent** |
| 81 | EVENT_TRACKING | 2 | **absent** |
| 85 | ANTICHEAT_CYCLELOGIC5 | 1 | **absent** |
| 88 | ANTICHEAT_OPLOGIC2 | 1 | **absent** |
| 118 | FRIENDLIST_ADD | 1 | **absent** |
| 146 | ANTICHEAT_CYCLELOGIC2 | 1 | **absent** |
| 148 | MESSAGE_PRIVATE | 1 | **absent** |
| 150 | REBUILD_GETMAPS | 1 | **absent** |
| 158 | MESSAGE_PUBLIC | 1 | **absent** |
| 171 | IGNORELIST_DEL | 1 | **absent** |
| 175 | TUT_CLICKSIDE | 1 | **absent** |
| 176 | ANTICHEAT_OPLOGIC4 | 1 | **absent** |
| 189 | EVENT_CAMERA_POSITION | 1 | **absent** |
| 190 | REPORT_ABUSE | 1 | **absent** |
| 215 | ANTICHEAT_CYCLELOGIC3 | 1 | **absent** |
| 219 | ANTICHEAT_CYCLELOGIC6 | 1 | **absent** |
| 220 | ANTICHEAT_OPLOGIC5 | 1 | **absent** |
| 233 | ANTICHEAT_CYCLELOGIC1 | 1 | **absent** |
| 236 | ANTICHEAT_CYCLELOGIC4 | 1 | **absent** |
| 238 | ANTICHEAT_OPLOGIC9 | 1 | **absent** |
| 2 | ANTICHEAT_OPLOGIC8 | 1 | encoder |
| 4 | CLIENT_CHEAT | 1 | encoder |
| 6 | INV_BUTTON5 | 1 | encoder |
| 8 | OPNPC2 | 1 | encoder |
| 27 | OPNPC3 | 1 | encoder |
| 31 | INV_BUTTON1 | 1 | encoder |
| 38 | INV_BUTTON4 | 1 | encoder |
| 40 | OPOBJ2 | 1 | encoder |
| 48 | OPHELDT | 1 | encoder |
| 52 | IDK_SAVEDESIGN | 1 | encoder |
| 53 | OPPLAYER2 | 1 | encoder |
| 59 | INV_BUTTON2 | 1 | encoder |
| 70 | IDLE_TIMER | 1 | encoder |
| 71 | OPHELD2 | 1 | encoder |
| 93 | MOVE_OPCLICK | 1 | encoder |
| 100 | OPNPC5 | 1 | encoder |
| 108 | NO_TIMEOUT | 5 | encoder |
| 113 | OPNPC4 | 1 | encoder |
| 130 | OPHELDU | 1 | encoder |
| 133 | OPHELD3 | 1 | encoder |
| 134 | OPNPCT | 1 | encoder |
| 138 | OPOBJT | 1 | encoder |
| 140 | OPOBJ1 | 1 | encoder |
| 155 | IF_BUTTON | 3 | encoder |
| 157 | OPHELD4 | 1 | encoder |
| 159 | INV_BUTTOND | 1 | encoder |
| 164 | OPPLAYER1 | 2 | encoder |
| 165 | MOVE_MINIMAPCLICK | 1 | encoder |
| 177 | OPPLAYERT | 1 | encoder |
| 178 | OPOBJ4 | 1 | encoder |
| 181 | MOVE_GAMECLICK | 1 | encoder |
| 185 | OPPLAYER3 | 1 | encoder |
| 194 | OPNPC1 | 1 | encoder |
| 195 | OPHELD1 | 1 | encoder |
| 200 | OPOBJ3 | 1 | encoder |
| 202 | OPNPCU | 1 | encoder |
| 206 | OPPLAYER4 | 2 | encoder |
| 211 | OPHELD5 | 1 | encoder |
| 212 | INV_BUTTON3 | 1 | encoder |
| 231 | CLOSE_MODAL | 1 | encoder |
| 235 | RESUME_PAUSEBUTTON | 1 | encoder |
| 237 | RESUME_P_COUNTDIALOG | 1 | encoder |
| 239 | OPOBJU | 1 | encoder |
| 244 | CHAT_SETMODE | 5 | encoder |
| 247 | OPOBJ5 | 1 | encoder |
| 248 | OPPLAYERU | 1 | encoder |

## Java methods with a name-matched native function (upper bound)

Name matching misses native code with different names (for example `useMenuOption` lives in `client_actions.c`/`mouse_picking.c`, and most of `updateGame`/`drawGame` is inlined in `platform/sdl2_main.c` `main()`), so the `client` row understates; every other row overstates fidelity.

| Class | Methods matched | Lines matched | Largest unmatched |
| --- | --- | --- | --- |
| deob.client | 38/138 | 3656/9430 | useMenuOption(578), updateGame(266), drawGame(241), drawInterface(229), handleInterfaceInput(199), handleInputKey(199) |
| jagex2.graphics.Pix3D | 5/17 | 1084/2348 | textureTriangle(595), textureRaster(401), setBrightness(79), getTexels(63), initPool(27), getAverageTextureRGB(22) |
| jagex2.dash3d.World3D | 17/54 | 1051/2086 | updateActiveOccluders(128), wallVisible(127), drawTileUnderlay(103), occluded(67), drawTileOverlay(64), mergeLocNormals(41) |
| jagex2.graphics.Model | 18/31 | 1370/1810 | drawNearClippedFace(160), drawSimple(51), drawFace(44), calculateBoundsAABB(42), calculateBoundsCylinder(26), addVertex(22) |
| jagex2.wordenc.WordFilter | 4/38 | 189/1089 | getEmulatedSize(180), filterTld(136), filterDomain(63), filterFragments(43), isBadFragment(34), getTldDotFilterStatus(33) |
| jagex2.dash3d.World | 13/16 | 941/996 | clearLandscape(23), adjustLightness(20), mulHSL(12) |
| jagex2.dash3d.CollisionMap | 9/13 | 537/560 | setBlocked(7), removeBlocked(7), remCMap(6), addCMap(3) |
| jagex2.io.BZip2 | 4/8 | 470/518 | createDecodeTables(33), makeMaps(9), getUnsignedChar(3), getBit(3) |
| jagex2.graphics.Pix32 | 10/16 | 316/517 | copyPixelsMasked(46), drawAlpha(44), copyPixels(43), scale(27), copyPixels(22), copyPixelsAlpha(19) |
| jagex2.client.GameShell | 9/38 | 105/428 | run(76), keyReleased(41), drawProgress(34), mousePressed(21), shutdown(15), mouseMoved(14) |
| jagex2.config.ObjType | 10/10 | 407/407 |  |
| jagex2.graphics.PixFont | 6/13 | 111/350 | PixFont(58), drawCharAlpha(38), drawMask(37), drawChar(35), drawStringTooltip(29), drawMaskAlpha(23) |
| jagex2.config.LocType | 6/6 | 301/301 |  |
| sign.signlink | 0/16 | 0/274 | run(72), startpriv(26), findcachedir(22), cachesave(20), getuid(19), gethash(16) |
| jagex2.client.InputTracking | 2/14 | 46/268 | mouseMoved(38), keyReleased(34), mousePressed(23), mouseReleased(22), focusLost(18), focusGained(18) |
| jagex2.config.Component | 3/4 | 246/263 | getImage(17) |
| jagex2.dash3d.entity.PlayerEntity | 4/5 | 178/255 | getSequencedModel(77) |
| jagex2.io.Packet | 28/31 | 208/255 | gBit(20), rsaenc(14), gstrbyte(13) |
| jagex2.graphics.Pix8 | 5/8 | 123/223 | copyPixels(45), Pix8(36), shrink(19) |
| jagex2.dash3d.type.TileOverlay | 1/1 | 212/212 |  |
| jagex2.sound.Tone | 4/4 | 182/182 |  |
| jagex2.config.NpcType | 5/6 | 137/176 | getSequencedModel(39) |
| jagex2.io.Isaac | 2/4 | 146/163 | Isaac(10), takeNextValue(7) |
| jagex2.sound.Wave | 5/6 | 122/145 | getWave(23) |
| jagex2.config.FloType | 4/4 | 137/137 |  |
| jagex2.graphics.Pix2D | 6/8 | 88/125 | setClipping(25), resetClipping(12) |
| jagex2.io.ClientStream | 5/7 | 66/120 | run(41), ClientStream(13) |
| jagex2.datastruct.JString | 3/7 | 57/105 | toSentenceCase(20), hashCode(12), toAsterisks(10), formatIPv4(6) |
| jagex2.datastruct.LinkList | 2/9 | 22/94 | prev(14), tail(12), addHead(11), head(10), removeHead(9), addTail(9) |
| jagex2.dash3d.entity.PathingEntity | 3/3 | 88/88 |  |
| jagex2.config.IdkType | 4/4 | 83/83 |  |
| jagex2.config.SeqType | 2/2 | 83/83 |  |
| jagex2.dash3d.entity.ProjectileEntity | 2/4 | 43/82 | updateVelocity(22), ProjectileEntity(17) |
| jagex2.graphics.AnimFrame | 1/1 | 78/78 |  |
| jagex2.wordenc.WordPack | 1/2 | 39/75 | pack(36) |
| jagex2.config.SpotAnimType | 3/3 | 70/70 |  |
| jagex2.io.Jagfile | 2/3 | 57/64 | Jagfile(7) |
| jagex2.dash3d.entity.SpotAnimEntity | 2/3 | 51/63 | SpotAnimEntity(12) |
| jagex2.graphics.PixMap | 3/10 | 27/57 | addConsumer(7), setPixels(6), removeConsumer(5), startProduction(3), requestTopDownLeftRightResend(3), isConsumer(3) |
| jagex2.dash3d.entity.NpcEntity | 2/3 | 35/56 | getSequencedModel(21) |
| jagex2.config.VarpType | 2/2 | 51/51 |  |
| jagex2.sound.Envelope | 3/3 | 45/45 |  |
| jagex2.datastruct.LruCache | 3/4 | 32/40 | LruCache(8) |
| jagex2.datastruct.HashTable | 2/3 | 22/34 | HashTable(12) |
| jagex2.graphics.AnimBase | 1/1 | 29/29 |  |
| jagex2.datastruct.DoublyLinkList | 0/3 | 0/25 | push(9), pop(9), DoublyLinkList(7) |
| jagex2.client.ViewBox | 1/4 | 3/22 | ViewBox(11), getGraphics(5), paint(3) |
| jagex2.dash3d.entity.LocEntity | 0/1 | 0/18 | LocEntity(18) |
| jagex2.dash3d.entity.LocMergeEntity | 0/1 | 0/13 | LocMergeEntity(13) |
| jagex2.dash3d.type.TileUnderlay | 0/1 | 0/9 | TileUnderlay(9) |
| jagex2.datastruct.DoublyLinkable | 0/1 | 0/8 | uncache(8) |
| jagex2.datastruct.Linkable | 0/1 | 0/8 | unlink(8) |
| jagex2.dash3d.entity.Entity | 1/1 | 6/6 |  |
| jagex2.dash3d.type.Ground | 1/1 | 5/5 |  |
