# Native test grades (Phase A, step 2) — 2026-10-22

Historical snapshot: current overall position and evidence limits are indexed in [OVERALL-STATUS.md](OVERALL-STATUS.md) and [D-REMAINING.md](D-REMAINING.md). Old statuses/counts below are not fresh failures or current-suite totals.


What each of the 70 programs in `make test` actually proves (including the October 21 Sol Phase C continuation). Graded by reading each test's expectations and where they come from, not its PASS text. Many PASS lines claim "bit-for-bit" or "100% parity" for checks that are spot anchors; those claims are void.

| Grade | Meaning | Counts toward parity? |
| --- | --- | --- |
| **A** | Expected values are output of the unchanged bundled Java client (oracle fixture), compared exactly | Yes, for the cases covered |
| **B** | Real cache data or the engine's wire format, with independent spot anchors (a few counts, names, sizes or golden values) | Weakly; it proves decoding runs, not that every field matches |
| **C** | Expected values come from the C implementation's own model (smoke tests, "not zero", native-only state) | No |
| **D** | Asserts behavior the original does **not** have (depth-buffer renderer properties) | No; must be replaced at the Phase C renderer port |

## A — Java oracle (35)

| Test | Oracle |
| --- | --- |
| test_model_draw_reference | Unchanged bundled Model.drawSimple: 312 real-cache draws / 5,114,304 framebuffer and guard words; four face modes, alpha, priorities, both texture details and Gouraud modes. World-space draw/near clipping not covered. |
| test_model_world_reference | Unchanged bundled world-space Model.draw: 396 real-cache cases / 6,491,232 framebuffer and suffix-guard words, prefix guards and final alpha/hclip; four near-clipped face modes, triangle/quad cases, frustum rejection, both texture details and Gouraud modes. Picking/World3D not covered. |
| test_model_picking_reference | Unchanged Model.draw selection: 8,704 query/list results, frame checksums and raster states; 2,176 direct Model.pointWithinTriangle boundary/random cases. Coarse/fine modes, hover/bitset gates and near-clipped exclusions; production list integration remains open. |
| test_painter_trace_reference | Original observed World3D/Model command stream, validated against untraced source frames: C replay of 12 frames / 2,052,192 framebuffer/guard words, exact entry/final raster state; 12,282 commands, 547 source model snapshots. Source geometry and painter order supplied; native traversal/construction/picking remain open. |
| test_occlusion_reference | Unchanged World3D activation, point/tile/wall/object/location predicates: 192 groups / 886,380 results, 2,564 ordered active identities, all pool fields and 192 whole-cache hashes; real 140-occluder scene plus 20 source-constructor probes. All modes/orientations, boundaries, overflow and cache collisions; native occluder construction and traversal remain separate. |
| test_scene_selection_reference | Unchanged World3D.draw stopped by virtual drawTile override before traversal: 99 cameras / 1,236,807 tile states from 12,493 real-cache scene tiles, exact camera/window/cycle/count and retained selection flags. Four matrix profiles, clamp/level/height/overflow boundaries; occlusion, traversal and raster output excluded. |
| test_visibility_reference | Unchanged World3D.init/testPoint: six complete visibility matrices / 3,995,136 booleans, final viewport/trig state and context guards; 6,827 real-map/boundary/random/overflow projection queries. Reinitialization preserves source edges and yaw wrap. Scene tile eligibility, traversal and occlusion are outside this gate. |
| test_tile_construction_reference | Unchanged World3D.setTile and tile constructors: 8,788 calls / 416,668 fields; all shapes/rotations, real-map inputs, textures, signed midpoints and retained-layer updates. Native Ground allocation representation and scene traversal are outside this gate. |
| test_ground_reference | Unchanged World3D ground calls: 5,256 draws / 21,570,624 framebuffer and guard words, alpha/hclip/tile selections; 50 table entries, 600 lightness and 2,176 point-test cases. Original geometry inputs include real map families and source constructor/sentinel cases; native construction/scene traversal remains open. |
| test_actor_clock | ActorClockOracle |
| test_actor_models | ActorOracle |
| test_audio_reference | WaveOracle (`audio-reference.bin` passed as argv) |
| test_bridge_levels | BridgeLevelOracle |
| test_click_feedback | CrossOracle |
| test_collision_reference | CollisionOracle |
| test_effects | EffectsOracle (+ combat fixture) |
| test_exact_movement | ExactOracle |
| test_facing | FacingOracle |
| test_location_anim | LocationOracle |
| test_location_merge | MergeOracle |
| test_midi_data | MidiOracle (+ BZip2 boundaries) |
| test_minimap_reference | MinimapOracle |
| test_navigation_reference | NavigationOracle |
| test_scene_lighting | SceneLightingOracle |
| test_target_interactions | TargetOracle (+ loopback encrypted stream) |
| test_title_screen | TitleOracle (+ loopback login handshake) |
| test_wall_decor | WallDecorationOracle |
| test_data_reference | DataOracle: all 27,984 decoded cache entries (configs incl. VarpType, models, anims, fonts, components, sprite pixels) by per-object digest |
| test_pix3d_flat_reference | Pix3DFlatOracle: unchanged bundled Pix3D.flatTriangle/flatRaster, 5,032 cases and 9,931,859 framebuffer/guard words; direct and opt-in public paths, with failing legacy/alpha/clip controls |
| test_pix3d_gouraud_reference | Pix3DGouraudOracle: unchanged bundled Gouraud triangle/span methods, supplied palette, 5,796 cases / 11,331,826 framebuffer/guard words; both jagged settings, alpha, clip, reciprocal and public-entry controls |
| test_pix3d_palette_reference | Pix3DPaletteOracle: unchanged bundled Pix3D/Pix8 on real textures archive; 50 decoded texture images/palettes/averages and all 65,536 HSL colours with deterministic brightness input |
| test_pix3d_texture_span_reference | Pix3DTextureSpanOracle: unchanged bundled textureRaster, both detail modes and real archive texels, 768 cases / 142,080 framebuffer and guard words |
| test_pix3d_texture_triangle_reference | Pix3DTextureTriangleOracle: unchanged bundled textureTriangle, 1,440 cases / 4,036,320 framebuffer and guard words, including an integrated C real-cache path |
| test_pix3d_texture_cache_reference | Java real-cache prepared texels for eight texture/detail combinations, 327,680 words, compared with native gamma and texel preparation |
| test_pix3d_brightness_reference | Pix3DBrightnessOracle: seeded Java brightness adjustment, two brightness states, HSL palette and texture texels, 163,840 words |

The scene lighting, wall decor and location oracles compare geometry, lighting and poses. Flat, Gouraud and textured primitive oracles compare Pix3D pixels exactly in the covered cases. Whole-scene World3D and Model.draw rasterization remain unverified.

## B — real data or wire format, spot anchors (16)

| Test | What it really checks |
| --- | --- |
| test_isaac | 5 values for seed {1,2,3,4}; provenance of the constants is unrecorded |
| test_rsa | One 8-byte block. The constants equal client.java's RSA_EXPONENT/RSA_MODULUS, and the cipher was independently re-derived with Python big integers on 2026-10-22 |
| test_packet | g1/g2/g3/g4/gsmart/gbit/gjstr on hand-built bytes, plus bounds rejection |
| test_jagfile | title.jag file count, title.dat size and JPEG magic |
| test_sprites | 3 sprite sizes and palette counts, non-zero pixels |
| test_model | model 0 counts, first two vertices, first face, one colour; label groups |
| test_config | counts > 0; NPC 0 "Hans", obj 1277/995 names, coins stackable |
| test_anim | counts > N, two sequence frame counts, delays > 1 |
| test_audio | wave decode/mix and a jingle file load |
| test_protocol_transport | real loopback sockets, production parser, framing errors (NET-01..03) |
| test_net_login | CRC checks only; live login **skips** without opt-in credentials |
| test_net_sync | inventory/varp/stat packets through `component_apply_packet` with hand-built payloads |
| test_entity_sync | PLAYER_INFO/NPC_INFO bitstreams hand-built from the engine encoder layout; base37 |
| test_zone_sync | zone packets through the production handlers, hand-built payloads |
| test_ground_items | production zone packets and an encrypted loopback Take operation |
| test_world | map m47_47/l47_47 decode, overlay-shape coverage; "reference" blend/light goldens of unrecorded provenance |

## C — self-referential or smoke (17)

test_bank_interface, test_chatbox, test_component, test_context_menu, test_dialogue_head, test_full_ui, test_interactions, test_interface_scripts, test_item_icon, test_map_streaming, test_media_ui, test_minimap, test_mouse_picking, test_pathfinding, test_camera, test_pix2d, test_rasterizer.

Typical patterns:
- "pixel != 0" or "rendered to framebuffer" checks.
- Hard-coded native layout constants.
- `test_camera` asserts the native orbit-camera model (orbit_x 4160 and so on), not the client.java camera.
- `test_pix2d` asserts one blend value and calls it "100% parity".
- `test_rasterizer` checks four trig anchors and "centre pixel non-zero".

These still catch crashes and gross regressions, so they stay in the suite.

## D — asserts non-original renderer behavior (2)

| Test | Non-original property asserted |
| --- | --- |
| test_scene_rasterizer | Submission-order independence via depth buffer; alpha deferred until after opaque geometry; perspective-correct UV; near-plane polygon clipping |
| test_coplanar_visibility | Coplanar priority detail survives a shared double-precision depth test (64-ULP tolerance) |

## Consequences for the plan

The separate diagnostic `test_scene_reference` is **not** in the passing-program count. Its original real Lumbridge 12-frame oracle reproduces, but current native scene parity fails all 12 views with 1,051,048/2,052,192 word differences. Run `tools/measure_static_scene.ps1`; inspect `runs/2026-10-21-phaseC-scene-oracle-sol/README.md`. Passing unit/oracle tests do not close this scene gate.

- **Data layer (A/B).** DataOracle now checks all 27,984 decoded entries by digest. The grade-B spot anchors remain weaker, separate evidence.
- **Rendering (A, C, D).** Primitive, model and isolated ground drawing have bounded Java-pixel coverage; model/ground picking has original-result coverage. Native tile construction now has bounded Java-object coverage. Visibility matrix setup/projection now have bounded Java coverage; scene tile eligibility now has separate real-scene Java coverage. Occluder activation/cached predicates now have separate real-scene Java coverage. Original painter order is now recorded and C raster replay matches its 12 complete frames; native generation of that order, occluder construction and full integration remain open. The static scene diagnostic still fails all 12 frames. Keep the D tests while the legacy depth renderer remains available; retire them when its replacement is verified.
- **UI (C).** Interface rendering and scripts have no Java comparison. They need a component-render oracle (Java `drawInterface` into a PixMap) in Phase D.
