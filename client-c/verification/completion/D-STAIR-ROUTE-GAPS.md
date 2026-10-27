# Missing stair routes repaired — October 26, 2026

Following the user's explicit direction to fix the routes, seven branches were added for all five missing placements. The landing tiles below are recovered from existing source route templates and actual map geometry; they are not independently verified original historical landing coordinates. This supersedes the audit-only implementation TODO below.

| Stair placement | Repaired destination | Source basis |
| --- | --- | --- |
| `1_47_152_52_48` (1723) | `0_47_152_50_48` | Existing Falador mine entrance arrival |
| `2_50_54_2_16` (1738) | `3_50_54_4_16` | East tower pair translated 19 tiles west, same model and angle |
| `3_50_54_3_16` (1740) | `2_50_54_3_15` | Same translated east tower pair |
| `1_48_48_20_32` (1739) | Up `2_48_48_21_34`; down `0_48_48_21_34` | Lumbridge south pair translated to the matching Tutorial Island models and angle |
| `2_48_48_21_33` (1740) | `1_48_48_21_34` | Same translated Lumbridge south pair |

The Tutorial Island stair uses the existing `stair_options` dialogue for its first option and matching destinations for the direct second/third options. Falador retains its existing arrival-delay command. No generic teleport fallback or invented dialogue was added.

Both isolated and normal server compilation/load pass with 8033 scripts. The normal disk pack is rebuilt. The script-ID pack is unchanged. Eight interpreter executions verify the direct routes, both dialogue choices and Falador arrival suspension. Checks use actual map location objects and the real server ground/location collision loaders: all destination tiles are unblocked with a cardinal walking exit. Player movement and network output are captured at the boundary; these checks do not claim a live player traversal. The coverage inventory now has zero unmatched placements among the 236 coordinate-specific stairs. Generic ladder coverage remains outside that inventory.

Evidence: `runs/2026-10-26-stair-routes-sol/`, especially `normal-routes.log`, `normal-pack.log`, `coverage-after.json`, `changes.patch` and `identities.json`. The first harness launch failed due to an incorrect relative copy path; corrected execution passes. A normal/isolated pack byte-equality assertion failed, so the actual normal pack was checked directly and passes; no pack byte equivalence is claimed. Full client tests were not rerun for this server-content-only change.

No game server listener was present on port43595 at completion. Start the usual `start_local_server.bat`, then use `test_client.bat`. No live process was started/stopped or user save modified.

---

## Original audit (historical; implementation TODO superseded above)

# Revision-225 stair route source gaps — October 26, 2026

The source audit identified five static stair placements absent from their coordinate-specific handlers. It did not establish their intended landing tiles or prove that ordinary players can reach every placement. No gameplay code or map was changed.

## Findings

| Object | Map coordinate | Absolute x, z, level | Missing handler branch |
| --- | --- | --- | --- |
| 1723 | `1_47_152_52_48` | 3060, 9776, 1 | Climb-down |
| 1738 | `2_50_54_2_16` | 3202, 3472, 2 | Climb-up |
| 1739 | `1_48_48_20_32` | 3092, 3104, 1 | Climb dialogue, Climb-up, Climb-down |
| 1740 | `2_48_48_21_33` | 3093, 3105, 2 | Climb-down |
| 1740 | `3_50_54_3_16` | 3203, 3472, 3 | Climb-down |

The general stair handlers cover 231 of 236 static placements across 18 coordinate-specific oploc1 handlers. The three coordinate-specific ladder handlers cover all three static placements: both Wizard Tower ladders and the King Black Dragon entrance. The additional Monk's Friend ladder coordinate has no static placement, consistent with a dynamically placed location; that finding is not a missing-route claim. Generic angle-based and relative-coordinate ladders are outside this inventory's coverage count.

Searched all bundled content scripts for dedicated handlers for the five stairs; the general stairs file supplies the only matching handlers. The Tutorial Island stairs therefore have no separate named handler overriding these routes. The bundled sibling `Server-main` stairs/ladders files have identical gameplay branches; their only differences are the earlier diagnostic-to-console changes. They provide no additional destinations. The Java client sends location interactions and does not define server teleport destinations.

## Implementation boundary

Do not infer that the existence of an unhandled fallback means normal stairs generally fail. Most enumerated placements have explicit handlers. Do not invent landing coordinates from nearby tiles, rotational symmetry, or a later game revision. The current AGENT.md requires inspecting local source and leaving an explicit TODO when details are absent.

TODO: obtain revision-225 destination evidence for the five coordinates above before implementing those branches. For each destination, establish intended plane and landing tile, any access restrictions, and arrival behavior. The source audit is complete; route implementation remains unresolved. No claim of an in-game fix is made.

## Evidence and reproduction

Run from the rewrite root:

```powershell
C:/Python314/python.exe client-c/verification/completion/runs/2026-10-26-stair-routes-sol/audit.py
```

`runs/2026-10-26-stair-routes-sol/coverage.json` records all 21 handler inventories, five unmatched placements, source hashes and the extra dynamic-only coordinate. The tool reads actual JM2 locations using the LOC section format defined in `engine/tools/pack/map/Pack.js`, with object names resolved through `content/pack/loc.pack`. It inventories source coverage, not route correctness or runtime behavior. No client rebuild, content compile, broad test suite, live server restart or save modification was necessary for this read-only task.

The earlier recommendation to immediately fix stairs/ladders was premature: exact missing destinations had not yet been established. These are bundled server-content gaps; this audit does not identify a new C-client defect.
