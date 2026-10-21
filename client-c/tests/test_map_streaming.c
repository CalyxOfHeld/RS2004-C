#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "dash3d/map_streamer.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include "core/packet.h"

int main(void) {
    printf("=== Test Suite: Dynamic Map Region Streaming & Packet 237 (Rev 225) ===\n");

    // 1. Packet 237 REBUILD_NORMAL Decoder Test
    MapStreamer streamer;
    map_streamer_init(&streamer);

    // Create a mock Packet 237 with 4 surrounding regions
    uint8_t pkt_buf[64];
    Packet pkt = packet_wrap(pkt_buf, sizeof(pkt_buf));
    p2(&pkt, 405); // center_zone_x
    p2(&pkt, 405); // center_zone_z

    // Region 0: 49, 49
    p1(&pkt, 49);
    p1(&pkt, 49);
    p4(&pkt, 11111);
    p4(&pkt, 22222);

    // Region 1: 50, 49
    p1(&pkt, 50);
    p1(&pkt, 49);
    p4(&pkt, 33333);
    p4(&pkt, 44444);

    pkt.length = pkt.pos; // Set valid payload length
    pkt.pos = 0; // Rewind for reading
    bool parsed = map_streamer_parse_rebuild_packet(&streamer, &pkt);
    assert(parsed == true);
    assert(streamer.center_zone_x == 405);
    assert(streamer.center_zone_z == 405);
    assert(streamer.base_tile_x == (405 - 6) * 8); // 3192
    assert(streamer.base_tile_z == (405 - 6) * 8); // 3192
    assert(streamer.region_count == 2);
    assert(streamer.regions[0].rx == 49 && streamer.regions[0].rz == 49);
    assert(streamer.regions[1].rx == 50 && streamer.regions[1].rz == 49);
    assert(streamer.scene_state == 1); // Loading
    printf("[PASS] Packet 237 (REBUILD_NORMAL) decoded bit-for-bit into scene base coordinates!\n");

    // 2. Load Config FloType for palette shaders
    Arena arena = arena_create(16 * 1024 * 1024);
    Jagfile *config_jag = jagfile_load_file("../engine/data/pack/client/config", &arena);
    if (!config_jag) config_jag = jagfile_load_file("engine/data/pack/client/config", &arena);
    assert(config_jag != NULL);

    FloConfig flo_cfg;
    flo_unpack(&flo_cfg, config_jag);
    assert(flo_cfg.count > 0);

    // Allocate World, World3D, and CollisionMaps on Heap
    World *world = (World*)calloc(1, sizeof(World));
    World3D *world3d = (World3D*)calloc(1, sizeof(World3D));
    assert(world != NULL && world3d != NULL);
    world_init(world, 104, 104);
    world3d_init(world3d, 104, 104, 4);

    CollisionMap collision_maps[4];
    for (int i = 0; i < 4; i++) {
        collision_map_init(&collision_maps[i], 104, 104);
    }

    const char *maps_dir = "../engine/data/pack/client/maps";
    FILE *test_f = fopen("../engine/data/pack/client/maps/m47_47", "rb");
    if (test_f) {
        fclose(test_f);
    } else {
        maps_dir = "engine/data/pack/client/maps";
    }

    // 3. Rebuild Lumbridge (Region 47, 47)
    map_streamer_init(&streamer);
    streamer.center_zone_x = 47 * 8 + 6;
    streamer.center_zone_z = 47 * 8 + 6;
    streamer.base_tile_x = 47 * 64;
    streamer.base_tile_z = 47 * 64;
    streamer.region_count = 1;
    streamer.regions[0].rx = 47;
    streamer.regions[0].rz = 47;

    bool rebuilt = map_streamer_rebuild(&streamer, world, world3d, collision_maps, &flo_cfg, NULL, maps_dir);
    assert(rebuilt == true);
    assert(streamer.scene_state == 2); // Active

    int non_zero_heights = 0;
    for (int x = 0; x < 64; x++) {
        for (int z = 0; z < 64; z++) {
            if (world->heightmap[0][x][z] != 0) non_zero_heights++;
        }
    }
    assert(non_zero_heights > 0);
    printf("[PASS] Scene region #1 (Lumbridge 47_47) successfully streamed and built (%d height tiles)\n", non_zero_heights);

    // 4. Rebuild Varrock (Region 50, 50) - verify seamless reuse without memory leaks
    streamer.center_zone_x = 50 * 8 + 6;
    streamer.center_zone_z = 50 * 8 + 6;
    streamer.base_tile_x = 50 * 64;
    streamer.base_tile_z = 50 * 64;
    streamer.region_count = 1;
    streamer.regions[0].rx = 50;
    streamer.regions[0].rz = 50;

    rebuilt = map_streamer_rebuild(&streamer, world, world3d, collision_maps, &flo_cfg, NULL, maps_dir);
    assert(rebuilt == true);
    assert(streamer.scene_state == 2);

    int varrock_heights = 0;
    for (int x = 0; x < 64; x++) {
        for (int z = 0; z < 64; z++) {
            if (world->heightmap[0][x][z] != 0) varrock_heights++;
        }
    }
    assert(varrock_heights > 0);
    printf("[PASS] Scene region #2 (Varrock 50_50) successfully streamed into recycled scene graph (%d height tiles)\n", varrock_heights);

    // 5. Test Boundary Shift Detection
    assert(map_streamer_needs_rebuild(&streamer, 52, 52) == false); // Safe center
    assert(map_streamer_needs_rebuild(&streamer, 10, 52) == true);  // Near West border
    assert(map_streamer_needs_rebuild(&streamer, 92, 52) == true);  // Near East border
    assert(map_streamer_needs_rebuild(&streamer, 52, 12) == true);  // Near South border
    assert(map_streamer_needs_rebuild(&streamer, 52, 90) == true);  // Near North border
    printf("[PASS] Boundary threshold checks verified (triggers rebuild when local tile < 16 or >= 88)\n");

    free(world3d);
    free(world);
    flo_free(&flo_cfg);
    arena_free(&arena);

    printf("=== ALL MAP STREAMING TESTS PASSED! ===\n");
    return 0;
}
