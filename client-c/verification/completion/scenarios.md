# Acceptance scenarios

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
