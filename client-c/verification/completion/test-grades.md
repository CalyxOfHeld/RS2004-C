# Native test grades (Phase A, step 2) — 2026-10-22

What each of the 54 programs in `make test` actually proves. Graded by reading each test's expectations and where they come from, not its PASS text. Many PASS lines claim "bit-for-bit" or "100% parity" for checks that are spot anchors; those claims are void.

| Grade | Meaning | Counts toward parity? |
| --- | --- | --- |
| **A** | Expected values are output of the unchanged bundled Java client (oracle fixture), compared exactly | Yes, for the cases covered |
| **B** | Real cache data or the engine's wire format, with independent spot anchors (a few counts, names, sizes or golden values) | Weakly; it proves decoding runs, not that every field matches |
| **C** | Expected values come from the C implementation's own model (smoke tests, "not zero", native-only state) | No |
| **D** | Asserts behavior the original does **not** have (depth-buffer renderer properties) | No; must be replaced at the Phase C renderer port |

## A — Java oracle (19)

| Test | Oracle |
| --- | --- |
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

The 3D-renderer oracles (scene lighting, wall decor, location) compare geometry, lighting and poses, not rasterized pixels. Nothing in the suite compares World3D/Pix3D output pixels against Java.

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

- **Data layer (B).** Decoding of the whole cache is checked only through a handful of anchors. A complete decoder oracle is the cheapest high-value step before Phase B: Java dumps every model, config type, sprite, font, interface component and animation frame, and C compares all fields. It will find any wrong field the foundation models introduced.
- **Rendering (C, D).** No Java-pixel coverage exists for World3D/Pix3D/Model. Phase C needs rasterizer oracles first (triangle routines on fixed inputs, then `Model.draw`, then scenes). At the port, the D tests are retired and replaced by them.
- **UI (C).** Interface rendering and scripts have no Java comparison. They need a component-render oracle (Java `drawInterface` into a PixMap) in Phase D.
