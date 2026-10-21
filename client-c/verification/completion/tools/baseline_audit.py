from pathlib import Path
import re,json,hashlib,shutil
root=Path('../..').resolve(); rewrite=root/'2004scape C Rewrite'; c=rewrite/'client-c'; base=c/'verification/completion'; run=base/'runs/2026-10-20-foundation'
for rel in ['src/net/protocol.c','src/net/client_stream.c','include/net/protocol.h','include/net/client_stream.h','platform/sdl2_main.c','src/ui/component.c','src/ui/context_menu.c']:
    target=base/'baseline'/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(c/rel,target)
shutil.copy2(c/'bin/rs2client.exe',base/'baseline/rs2client.exe')
main=(c/'platform/sdl2_main.c').read_text(); proto=(c/'src/net/protocol.c').read_text()
handled={int(x) for x in re.findall(r'opcode == (\d+)',main)}
sizes={int(k):int(v) for k,v in re.findall(r'server_packet_sizes\[(\d+)\]\s*=\s*(-?\d+)',proto)}
rows=[]
for kind in ['ServerGameProt','ServerGameZoneProt']:
    source=rewrite/f'engine/src/network/game/server/{kind}.ts'
    for number,line in enumerate(source.read_text().splitlines(),1):
        m=re.search(r'static readonly (\w+) = new \w+\((\d+), (-?\d+)\)',line)
        if not m:continue
        name,op,length=m.group(1),int(m.group(2)),int(m.group(3))
        rows.append({'name':name,'opcode':op,'length':length,'reference':str(source.relative_to(rewrite)).replace('\\','/')+':'+str(number),'framing':'matches' if sizes.get(op)==length else 'MISSING/DIFFERENT','handler':'branch present; behavior unverified' if op in handled else 'no branch in SDL loop'})
(run/'server-opcodes-baseline.json').write_text(json.dumps(rows,indent=2)+'\n')
lines=['# Initial capability audit','', 'Evidence level: source inspection only unless explicitly stated. Historical captures do not verify gameplay.','', '## Incoming protocol coverage','', '| Name | Opcode | Length | C framing | SDL dispatch | Reference |','| --- | --- | --- | --- | --- | --- |']
for r in rows:lines.append(f"| {r['name']} | {r['opcode']} | {r['length']} | {r['framing']} | {r['handler']} | {r['reference']} |")
lines+=['','## Cross-system coverage','','| Capability | Initial state | Production evidence / gap | Gate |','| --- | --- | --- | --- |', '| Session framing | Failing by inspection | protocol_poll_packet enters blocking reads after one byte; no output capacity; zone packet lengths absent | NET-01..04 |','| Entity state/render | Partial | SDL dispatch expects NPC opcode 134, server sends 1; rendered demonstration NPC is separate from synchronized NPCs | ENT-01 |','| Movement | Failing by inspection | SDL sends adjacent-step deltas and scene-local origins; server expects start-relative deltas and world coordinates | MOVE-01 |','| Interaction dispatch | Failing by inspection | Numeric action-ID ranges overlap cache component IDs; item slot hardcoded to 0; default sidebar ignores item action | UI-01..03 |','| Inventory receive | Partial | Production handlers exist; full updates do not clear trailing slots; malformed quantity payload handling needs audit | INV-01 |','| Combat/equipment | Unverified/incomplete | Dependent on complete widget/action/varp/inventory/appearance paths | GAME-01 |','| Terrain/materials | Partial | Cache textures are loaded; no matched Java-client scene comparison yet; blending/lighting incomplete | VIS-01 |','| Minimap | Partial | minimap_build fills tiles with one RGB; wall/mapscene/shaped-floor fidelity unverified | VIS-02 |','| World operations and zone changes | Incomplete | Several outgoing encoders exist; world picking/action chain and zone dispatch not complete | WORLD-01 |','| Social/audio/effects | Unverified/incomplete | Some assets/helpers exist; full protocol and gameplay loops unaudited | REMAIN-01 |','| Test validity | Mixed | test_net_sync assigns state directly; test_full_ui renders fixtures; neither proves real input/server round trip | QA-01 |','', '## Reference launch boundary','', 'The existing javaclient/build/libs/rs2client.jar is present. sign/signlink.java:180 searches system folders for its cache. It has NOT been launched because that would violate the project-only file-access boundary. Prepare an isolated reference harness with an explicit in-project cache before running it. Source comparison is available; visual parity is unverified.','']
(base/'capabilities.md').write_text('\n'.join(lines),encoding='utf-8')
(base/'scenarios.md').write_text('''# Acceptance scenarios

All statuses start UNVERIFIED. Test fixtures are explicitly identified and do not count as live gameplay.

| ID | Setup/input | Required observed result | Evidence layer |
| --- | --- | --- | --- |
| NET-01 | Local TCP peer sends an ISAAC-encrypted opcode, split variable-length header and split payload on separate writes | Poll returns immediately until complete, emits exactly one packet, preserves cipher state | Real loopback sockets + production parser |
| NET-02 | Same TCP stream coalesces fixed/variable/zero-length/zone packets | All complete payloads and following opcodes decode exactly | Real loopback sockets + production parser |
| NET-03 | Length exceeds destination capacity; unknown opcode; peer closes mid-packet | Explicit failure/disconnect, untouched guard bytes, no hang or fabricated empty packet | Real loopback sockets + production parser |
| NET-04 | Real game server login and sustained receive loop | Actual client stays responsive; capture records handled/unhandled packets separately | Live server + actual SDL client |
| MOVE-01 | Actual route with a turn and nonzero region base | Wire waypoints reconstruct world positions per MoveClickDecoder; server confirms local position | Encoder regression then actual client/server |
| ENT-01 | Live local player and NPC updates, then cross a region/plane | Correct active entities, positions, models and animation; no demonstration entities in live view | Server + actual client |
| UI-01 | Hover/click nested widget with component ID above 1000 | Correct exact component reaches IF_BUTTON; no range collision | Production menu/dispatcher then SDL input/live server |
| UI-02 | Right/default-click two item slots with different operations | Correct operation, component, slot and item reach server; actual state reply displayed | Production dispatcher + live client |
| UI-03 | Scroll, switch tabs, open/close modal, continue dialog | Same action path; correct clipping and input priority | Actual SDL input/live server |
| INV-01 | Full inventory update shrinks, partial update clears and changes large stack | No stale slots; malformed update rejected without partial state mutation | Production receive handler + live client |
| GAME-01 | Equip/unequip two weapon types and change supported combat styles | Server inventory/equipment/appearance/varp replies agree; persistence after relog | Live server + actual client |
| VIS-01 | Matched Java/native location, camera, plane, detail mode and cache | Materials, original underlay shading, textured overlays, scenery and lighting compared; differences recorded | Reference + native captures |
| VIS-02 | Same scene minimap at rotations and region boundaries | Reference masks, shaped floors, walls/icons/dots/coverage compared; explain every black region | Reference + native captures |
| WORLD-01 | Reachable/blocked objects, NPCs and ground items; doors/stairs | Correct action/reach/routing and server scene/interface updates | Live server + actual client |
| REMAIN-01 | Banking, shops, combat/skilling, dialog/quests, social/audio | Capability-specific complete sessions, no fake success state | Live server + actual client |
| QA-01 | Audit each existing PASS assertion | Label fixture/unit/integration/live/reference correctly; no blanket parity claims | Source + recorded executions |
''',encoding='utf-8')
(base/'progress.md').write_text('''# Autonomous execution ledger

Status: ACTIVE — initial audit completed; phase 1 transport foundation in progress.

Boundaries: all project files/evidence/temp files remain inside 2004scape-LostCityRS.
No new visual assets, textures or gameplay rules are being invented. Unknowns remain unknown.

Confirmed by inspection: missing zone packet lengths; blocking partial packet reads; no destination capacity; wrong NPC dispatch opcode; local/adjacent walking deltas; overlapping menu action ranges and lost item slot; incomplete minimap scene representation; fixture tests overstate live coverage.

Reference binary is available but its cache search crosses the directory boundary. Do not launch it unchanged. Prepare an isolated in-project reference harness later; no visual reference parity claim is currently supported.

Next: add a bounded incremental receive state machine, include every bundled server/zone framing definition, and exercise fragmented/coalesced/closed TCP streams through real loopback sockets. Then fix movement/production receive/action foundations using source-derived scenarios.

No phase is complete merely because its source compiles. Log each verification and remaining limitation in this file.
''',encoding='utf-8')
print(f'Audited {len(rows)} incoming definitions; {sum(r["framing"]!="matches" for r in rows)} missing/different framing entries; {sum(r["handler"].startswith("no") for r in rows)} without SDL dispatch branch.')
print('Free bytes:',shutil.disk_usage(c).free)
