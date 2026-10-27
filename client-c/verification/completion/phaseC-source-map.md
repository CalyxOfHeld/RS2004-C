# Bounded C5 candidate accepted — October 23, 2026

Current source/evidence: C5-ACCEPTANCE.md and runs/2026-10-23-c5-acceptance-sol/. Final source identities cover client integration, original225 texture cache, current-plane pick filtering, entity lifetime/bounds and existing controls. Original getNpcPos/getNpcPosOldVis cycle lifetime maps to entity_sync.c; updateEntity bounds maps to pathing_entity_validate225; drawGame retained areaMapback, draw2DEntityElements projection/default height and updateTextures map to candidate-only client.c paths. Geometry, scene callbacks and generated picking remain in the native225 runtime modules.

Current gates:6796 original screens/169900 regional hashes exact across integrated/stair/item/bridge controls; protocol and strict replay match; C2/C3/C4/runtime and registered suite pass. See report for exact identities and bounded limits. Normal executable unchanged; deployment and Phase D remain separate. Earlier unverified/active entries below are historical for their recorded binaries.

---

# C5 startup and design preview verified - October 22, 2026

Original startup ordering/RNG and content327 design compose/recolor/pose/light, content324/325 images and PixFont.drawStringTooltip now have candidate-only production paths. Seeds225/226 match post-load RNG/palette/floor fingerprints; final design session has continuous complete viewport equality16..279 and matching protocol. Full app/minimap/loading qualification remains open. See runs/2026-10-22-scene-startup-sol/README.md and runs/2026-10-22-design-preview-sol/README.md. Installed renderer unchanged.

---

# C5 runtime integration checkpoint - October 22, 2026

New scene_runtime225 composes original pushPlayers/pushNpcs/pushProjectiles/pushSpotanims, getTopLevel and drawScene with the verified construction/preparation/traversal/raster modules. Candidate client integration uses original512x334 coordinates, generated picks and verified drawSimple previews. Latest source/evidence/limits: runs/2026-10-22-scene-runtime-sol/README.md. Fresh C2/C3/C4/runtime gates pass; full production/replay frame parity and pick-to-menu equivalence remain open. Separate candidate only; normal client unchanged.

---

## October 22 C4 checkpoint

The bounded ordered scene-picking gate passes in `runs/2026-10-22-scene-picking-sol/README.md`: twelve original queries, 12,000 list slots and final flags/tiles exact; 25 appended hits. Twelve worlds plus twelve previews remain exact (4,104,384 pixel/guard words), with 11,014 callbacks and 4,534 native model comparisons including Model.pickable. Model.java:1641 world hover now maps through model_render_world_bounds_pick225 to prepared scene bounds; SceneRaster225 appends hits in draw order. World3D.java:1185 click setup, :1310/:1356 input completion and :1887/:1911/:1974 terrain picks map to existing Ground225 selection plus scene input propagation. Duplicate/order/disabled/signed-bitset/miss cases pass; an order-only expectation fault rejects without changing pixels. C2/C3 and existing model/ground regressions pass. Production input offsets, menus/actions, wider bridge/camera coverage and playable integration remain C5 qualification work.

## October 22 C3 checkpoint

The bounded dynamic scene gate passes in `runs/2026-10-22-scene-dynamic-sol/README.md`: twelve world frames plus twelve previews, 4,104,384 pixel/guard words, 11,014 callbacks and 4,534 model/bounds comparisons exact. World3D.drawTile resolves absent direct models through Entity.draw(true) before normal-location occlusion; scene_traversal225 now follows that timing. World3D.addTemporary/clearTemporaryLocs map to scene_build225 temporary APIs; native location_anim_tick plus scene refresh track replacement pointers/bounds. Model.drawSimple and World3D.draw share projection/raster state. Pix3D.clearTexels/unpackTextures/setBrightness motivate cache invalidation generation and context palette restoration. A bridge identity control and old-context pixel fault reject before their fixes. Static scene and existing actor/traversal checks pass. This is isolated and bounded; ordered picking is C4, playable integration and broader dynamic/bridge coverage remain C5.

# Phase C renderer source map — October 21, 2026

Primary source: the bundled `javaclient/src/main/java` revision-225 tree.
`webclient/src/dash3d/Pix3D.ts` is a secondary cross-check; it does not supply
expectations. No OSRS or later-revision source was opened. Line numbers refer to
the bundled Java at this checkpoint.

| Dependency | Java source / methods | Native counterpart / gap | Gate |
|---|---|---|---|
| Surface and clipping | `graphics/Pix2D.java` bind/setClipping; `Pix3D.java:114` init2D | `gfx/pix2d.c`, `gfx/pix3d.c` scanline/clip globals | Flat oracle exercises actual Java clip globals |
| Flat triangles and spans | `graphics/Pix3D.java:1014` flatTriangle, `:1413` flatRaster | New `gfx/pix3d_flat225.c`, `gfx/pix3d_flat225.h`; legacy body retained in `pix3d.c` | **PASS isolated**, 5,032 cases / 9,931,859 words; default path remains legacy |
| Gouraud | `graphics/Pix3D.java:402` gouraudTriangle, `:885` gouraudRaster | New `gfx/pix3d_gouraud225.c`; public adapter preserves existing native 8.8 inputs; legacy body retained | **PASS isolated**, 5,796 cases / 11,331,826 words; both jagged settings, alpha and clip paths |
| Texture storage, palettes and light | `graphics/Pix3D.java:150–400` clearTexels/initPool/unpackTextures/getTexels/setBrightness/setGamma; `graphics/Pix8.java` crop/shrink | `pix3d_texture_cache225.c`, `pix3d.c`, `pix8.c`; real-cache texel preparation and gamma/palette lifecycle; separate startup/varp opt-in | **PASS isolated:** 50 prepared textures and HSL palette; 8 cache records / 327,680 texel words; 2 brightness states / 163,840 palette and texel words. Java pool/LRU allocation lifecycle and full startup random stream remain open |
| Textured triangles and spans | `graphics/Pix3D.java:1476` textureTriangle, `:2073` textureRaster | New `pix3d_texture225.c`, `pix3d_texture_triangle225.c`; legacy `pix3d.c` scene_raster/scene_triangle still use double depth and UV sampling | **PASS isolated:** 768 spans / 142,080 words and 1,440 triangles / 4,036,320 words, both detail modes and opaque/translucent paths. Integrated real-cache triangle and separate model face dispatch pass; production scene integration pending |
| Model transform, visibility, picking | `graphics/Model.java:1588` drawSimple, `:1641` draw, `:2132` pointWithinTriangle | Separate model_render_simple225, model_render_world225 and model_render_world_pick225; installed paths retain existing calls | **PASS bounded:** preview 312 cases / 5,114,304 words; world 396 cases / 6,491,232 words; picking 8,704 original query/list results plus frame checksums/raster states and 2,176 direct boundary cases. Scene/list integration remains open. See `runs/2026-10-21-phaseC-model-picking-sol/README.md` |
| Model painter ordering and face dispatch | `graphics/Model.java:1754` draw(boolean,…), `:1924` drawFace, `:1970` drawNearClippedFace | Separate verified model paths use original integer bucket bounds, stable ordering, priority-10/11 interleaving, direct face primitives, and z=50 clipping with inherited alpha | **PASS bounded** for four face modes/alpha/priorities and clipped triangles/quads. Reciprocal, quad, inherited-alpha and boundary mutations reject. Installed scenes retain the legacy depth renderer; World3D integration pending |
| Visibility setup and projection | `dash3d/World3D.java:1088` init, `:1162` testPoint | Separate `dash3d/visibility225.c` context; original signed integer projection, yaw wrap and retained matrix edges | **PASS isolated:** six matrices / 3,995,136 booleans, final state/guards and 6,827 point queries; ten faults reject. Installed scene remains legacy; draw eligibility is separately checked, occlusion not covered. See `runs/2026-10-21-phaseC-visibility-sol/README.md` |
| Scene camera and tile eligibility | `dash3d/World3D.java:1196-1265` draw setup/selection | Separate `dash3d/scene_selection225.c` camera/cycle/map slice/window and tile-state updates | **PASS isolated:** 99 cameras / 1,236,807 tile states from 12,493 original real-cache scene tiles; exact setup/count, metadata and guards. Twelve source faults plus damaged/truncated fixtures reject. Source draw is intercepted at first virtual drawTile dispatch; occlusion and traversal are excluded. See `runs/2026-10-21-phaseC-selection-sol/README.md` |
| Original painter trace / ordered raster integration | Private Java-8 entry/return observation of original World3D.draw/ground calls and final Model.draw, source files unchanged | Existing supplied-order comparator plus native scene_raster225 callback integration | **PASS static integration:** C1 generated traversal and graph geometry, then C2 native cache/map construction, both match all 12 complete frames / 2,052,192 words and entry/final raster state. 12,282 calls / 547 drawn-model identities. Supplied-order replay remains an existing regression. See `runs/2026-10-22-scene-raster-sol/README.md` and `runs/2026-10-22-scene-build-sol/README.md` |
| Scene visibility and draw order | `dash3d/World3D.java:1196` draw, `:1366` drawTile | scene_prepare225 composes verified visibility/selection/occlusion; scene_traversal225 generates native order; scene_raster225 draws directly | **PASS static native path:** all 12 native-prepared frames/order/state exact, without supplied per-frame traversal state. Legacy world3d_render's historical diagnostic is 0/12 (1,051,048 differences); that separate installed path was not switched. Dynamic entities/picking and playable-renderer qualification remain open. See `runs/2026-10-22-scene-build-sol/README.md` |
| Tile construction | `dash3d/World3D.java:347` setTile; `dash3d/type/TileOverlay.java` and TileUnderlay | `world3d.c` world3d_set_tile: original shape-0 flags and explicit Java midpoint arithmetic | **PASS bounded:** 8,788 calls / 416,668 output fields; all 13 shapes/four rotations, real-map heights/colors, texture choices, signed boundaries and retained-layer updates; eight fault controls reject. Native Ground allocation representation and full scene construction remain open. |
| Ground geometry/raster calls | `dash3d/World3D.java:1824` drawTileUnderlay, `:1929` drawTileOverlay, `:1995` mulLightness, `:2010` pointInsideTriangle | Separate `dash3d/ground225.c` underlay/overlay entry points and original helpers; installed world3d.c retains legacy calls | **PASS isolated:** 5,256 draws / 21,570,624 words and final input/raster states; full 50-entry table, 600 lightness and 2,176 picking cases. Eleven faults reject. Original object geometry is supplied as input; native construction is checked separately; scene integration remains open. See `runs/2026-10-21-phaseC-ground-sol/README.md` |
| Occluders and cached predicates | `dash3d/World3D.java:2028` updateActiveOccluders; `:2158-2424` tileVisible/wallVisible/visible/locVisible/occluded | Separate `dash3d/occlusion225.c` original activation/point/cached predicates; caller supplies source pool/heights/cycles | **PASS bounded:** 192 groups / 886,380 queries, 2,564 ordered active entries, all pool fields and 192 whole-cache hashes; all modes/orientations, threshold/overflow/cache collisions; fourteen faults reject. Native original occluder construction and painter integration remain open. Full scene still fails. See `runs/2026-10-21-phaseC-occlusion-sol/README.md` |
| Bridges, walls and decorations | `dash3d/World3D.java:299` setBridge; drawTile; `dash3d/World.java:258` addLoc | world.c native metadata/models and scene_build225 original Ground allocation, shared spans, wall/decor buckets and bridge relinking | **PASS static construction:** 12,533 Ground identities and 1,046 shared locations match original structure/geometry/metadata; native-generated callbacks/frames exact. Model pointers are borrowed until rebuild; animation refresh/lifetime is C3. |
| Build, roofs, shared lighting | `dash3d/World.java:507` build; `World3D.java:858–1027` buildModels/mergeNormals | world.c opt-in build225 seeded hue/light/roof masks; scene_build225 original occluder coalescing; existing scene_lighting/model normal merging | **PASS accepted static corpus:** 44,100 heights, geometry, all four occluder pools (9/32/49/50), raster-relevant fields/bounds of all 547 drawn model identities, native preparation and 12 complete frames exact. Existing normal/lighting and tile-construction checks pass. Other region/lifecycle/full-client qualification remains open. See `runs/2026-10-22-scene-build-sol/README.md` |
| Replay acceptance | `reference/src/ParityOracle.java`, `tools/parity.ps1` | Fresh texture checkpoint `soltex22`: default C exact for 440 frames; opt-in stream replay completes 440 with 339 changed framebuffer frames | Java protocol MATCH over 345 in-game frames; viewport DIFFERS on all 345; full-frame Java gate remains open. See `runs/2026-10-21-phaseC-texture-sol/README.md` |

## First primitive contract and evidence

`Pix3DFlatOracle.java` compiles **unchanged bundled Pix3D.java** into a private
class directory, ahead of the unchanged jar dependency. It calls flatTriangle
and flatRaster directly; no raster algorithm or patched Java source is in the
oracle. Source-built and jar-built output hashes agree. Fixtures include
permutations, horizontal/vertical/degenerate triangles, offscreen vertices,
signed 16.16 overflow, all span-length remainders, nonzero clip-left/top,
full viewport/framebuffer dimensions, fixed random inputs, alpha 0/1/64/128/255/256,
and both hclip settings with valid destination coordinates.

Java clips vertical coordinates to zero/boundBottom, ignores boundLeft/boundTop,
and clips span ends to safeWidth when hclip is enabled. Translucent pixels read
the next destination word and truncate source/background channels separately.
The C port preserves these details, with defined Java-int overflow and shifts.
The 24 raster dispatch sites and all edge branches are mechanically retained;
`tools/port_pix3d_flat.py --check` detects drift from the generated source.

Flags: `-DRS2_PIX3D_REV225=1` routes both public flat/Gouraud triangle entries to
the verified primitives. Individual `RS2_PIX3D_REV225_FLAT` and
`RS2_PIX3D_REV225_GOURAUD` switches can override the umbrella for isolated checks.
Defaults are zero. These flags **do not enable a complete original world
renderer**: scene triangles still use the legacy double-depth path. Keep them
off in the installed client while Model and scene ordering are pending.

Reproduction (from the rewrite root):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File client-c/verification/completion/tools/verify_pix3d_flat.ps1 -OutDir client-c/build/new-flat-evidence
```

Choose a new output directory; the script refuses to overwrite one. It rebuilds
the fixture, reproduces the legacy failure, verifies the direct and opt-in paths,
and checks alpha-neighbor/clip-edge negative controls. `make test` includes the
stored Java fixture comparison. The fixture is grade A; the legacy rasterizer's
self-referential tests remain separate evidence.

## Gouraud primitive checkpoint

`Pix3DGouraudOracle.java` compiles the unchanged bundled Pix3D.java ahead of the
jar dependency and supplies an explicit 65,536-entry palette as function input.
It does not duplicate edge stepping or raster logic. The fixture compares all
framebuffer and guard words for 5,796 cases: six vertex permutations, both
`jagged` settings, alpha 0/1/64/128/255/256, clip/no-clip, nonzero clip-left/top,
fractional 8.8 span colours, increasing/decreasing gradients, all four tail
lengths, constant palette endpoints 0/65535, fixed-point edge overflow, fixed
random inputs, real viewport widths, and the reciprocal table's final valid
group index. Palette construction and brightness are verified in the later
palette and texture checkpoints below.

The port preserves Java's four-pixel jagged groups and reciprocal-table rounding,
per-pixel smooth mode, original next-destination alpha reads and safeWidth edge.
Its 24 triangle dispatch sites and all edge branches are mechanically retained.
The native public API still accepts 8.8 triangle colours; its opt-in adapter
converts those whole palette indices to Java's raw colour arguments.

Reproduce with `tools/verify_pix3d_gouraud.ps1 -OutDir <new-directory>`.
It verifies source generation, rebuilds the Java fixture, reproduces the legacy
failure and rejects four deliberate alpha/clip/jagged/reciprocal faults. An
initial long span which makes Java itself index outside the palette was removed
from the valid-input corpus; its source and failure remain in the run evidence.

The replay tool's explicit `--allow-frame-differences` mode permits changed
renderer pixels while retaining stream/cycle checks. It reports **PASS STREAM
REPLAY**, with the exact number and first frame of differences, and supports
the existing Java region-hash comparison. The default remains strict full-frame
replay. `tools/verify_renderer_live.ps1` makes a fresh disposable recording and
runs both modes plus outgoing-byte/truncation/wrong-password controls while the
password is still in memory. Both default/opt-in Java image comparisons remain
red; pixel mismatches are not converted into parity success.

## Texture and brightness checkpoint

`Pix3DTextureSpanOracle.java` and `Pix3DTextureTriangleOracle.java` compile the
unchanged bundled `Pix3D` and `Pix8` and load the real textures archive. The
native span/triangle ports compare every framebuffer and guard word across
768 span and 1,440 triangle cases in both detail modes. A second triangle
comparison obtains its texels from the native real-cache preparation path.
`port_pix3d_texture_triangle.py --check` guards the 24 generated raster calls
and edge branches.

`Pix3DBrightnessOracle.java` seeds the original Java `Math.random` and checks
startup 0.8 and changed 0.6 brightness, including all HSL colours and texture
17's four shaded texel planes. The C test also checks that the live preview
helper computes the same next value without advancing the protocol RNG.
Native opt-in handles brightness VarpType updates, but live varp parity and
Java's complete startup RNG order remain open. The new triangle has a separate
entry point and is not yet called by Model/World3D. See
`runs/2026-10-21-phaseC-texture-sol/README.md` for the full evidence and
reproduction commands.
