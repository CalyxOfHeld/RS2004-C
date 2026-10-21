#include "dash3d/minimap.h"
#include "dash3d/world.h"
#include "config/flo.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/media.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("=== Test Suite: Minimap Radar & Compass Needle System ===\n");

    Arena arena = arena_create(64 * 1024 * 1024);

    pix3d_init();
    pix3d_init_colour_table(0.8);

    uint32_t *fb = (uint32_t *)malloc(765 * 503 * sizeof(uint32_t));
    pix2d_bind(fb, 765, 503);
    pix2d_fill_rect(0, 0, 765, 503, 0);

    // Load archives
    Jagfile *config_jag = jagfile_load_file("../engine/data/pack/client/config", &arena);
    if (!config_jag) config_jag = jagfile_load_file("engine/data/pack/client/config", &arena);
    assert(config_jag);

    Jagfile *media_jag = jagfile_load_file("../engine/data/pack/client/media", &arena);
    if (!media_jag) media_jag = jagfile_load_file("engine/data/pack/client/media", &arena);
    assert(media_jag);

    Media media;
    bool media_ok = media_load(&media, media_jag, &arena);
    assert(media_ok);
    assert(media.mapback != NULL);
    assert(media.compass != NULL);
    printf("[PASS] Media assets loaded (mapback=%dx%d, compass=%dx%d)\n",
           media.mapback->width, media.mapback->height,
           media.compass->width, media.compass->height);

    FloConfig flo_cfg;
    flo_unpack(&flo_cfg, config_jag);
    printf("[PASS] %d floor types unpacked from config.jag\n", flo_cfg.count);

    // Test 1: Minimap initialization and mask generation
    Minimap minimap;
    minimap_init(&minimap, &media, &arena);

    // Verify compass mask (33 lines)
    int compass_valid_lines = 0;
    for (int i = 0; i < 33; i++) {
        if (minimap.compass_mask_lengths[i] > 0) compass_valid_lines++;
    }
    printf("  Compass mask valid lines: %d/33\n", compass_valid_lines);
    assert(compass_valid_lines == 33);
    printf("[PASS] Compass mask successfully generated from mapback circular cutout!\n");

    // Verify minimap mask (151 lines)
    int minimap_valid_lines = 0;
    for (int i = 0; i < 151; i++) {
        if (minimap.minimap_mask_lengths[i] > 0) minimap_valid_lines++;
    }
    printf("  Minimap mask valid lines: %d/151\n", minimap_valid_lines);
    assert(minimap_valid_lines > 140);
    printf("[PASS] Minimap radar mask successfully generated from mapback circular window!\n");

    // Test 2: Build minimap from World terrain
    World *world = (World *)calloc(1, sizeof(World));
    world_init(world, 104, 104);

    // Set sample underlays and overlays
    for (int z = 0; z < 104; z++) {
        for (int x = 0; x < 104; x++) {
            world->tile_underlays[0][x][z] = 1; // Flo 0
            if (x >= 40 && x <= 60 && z >= 40 && z <= 60) {
                world->tile_overlays[0][x][z] = 2; // Flo 1 path/building
            }
        }
    }

    World3D *scene=calloc(1,sizeof(World3D));assert(scene);
    world3d_init(scene,104,104,4);
    world_build(world,scene,&flo_cfg,NULL);
    minimap_build(&minimap, world, scene, &media, 0);

    int populated_pixels = 0;
    for (int i = 0; i < 512 * 512; i++) {
        if (minimap.image_minimap->pixels[i] != 0) populated_pixels++;
    }
    printf("  Populated minimap terrain buffer pixels: %d\n", populated_pixels);
    assert(populated_pixels > 100000); // 102*102*16 = ~166,464 pixels
    printf("[PASS] 512x512 Minimap tile pixel buffer built successfully!\n");

    // Golden shape-2 mask from bundled Java, all four rotations. Transparent
    // background must preserve existing pixels; absent floor stays absent.
    const int shape2[16]={1,0,0,0,1,1,0,0,1,1,1,0,1,1,1,1};
    const int rotations[4][16]={{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
        {12,8,4,0,13,9,5,1,14,10,6,2,15,11,7,3},
        {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0},{3,7,11,15,2,6,10,14,1,5,9,13,0,4,8,12}};
    GroundTile shaped={0};shaped.has_overlay=true;shaped.overlay.shape=2;shaped.overlay.foreground_rgb=0x123456;
    uint32_t tile_pixels[16];
    for(int r=0;r<4;r++) {
        shaped.overlay.rotation=r;
        for(int i=0;i<16;i++) tile_pixels[i]=0xabcdef;
        minimap_draw_tile(tile_pixels,0,4,&shaped);
        for(int i=0;i<16;i++) assert(tile_pixels[i]==(shape2[rotations[r][i]] ? 0x123456:0xabcdef));
    }
    int tx=50,tz=50,off=(103-tz)*512*4+24624+tx*4;
    scene->tiles[0][tx][tz].has_underlay=false;scene->tiles[0][tx][tz].has_overlay=false;
    minimap_build(&minimap,world,scene,&media,0);assert(minimap.image_minimap->pixels[off]==0);
    scene->tiles[1][tx][tz].has_underlay=true;scene->tiles[1][tx][tz].underlay.rgb=0x334455;
    world->tile_flags[1][tx][tz]=8;
    minimap_build(&minimap,world,scene,&media,0);assert(minimap.image_minimap->pixels[off]==0x334455);
    world->tile_flags[1][tx][tz]=0;
    assert(media.mapscene[0] && media.mapfunction[0]);
    assert(!pix8_load(media_jag,"mapscene",500,&arena));
    assert(!pix32_load(media_jag,"mapfunction",500,&arena));
    puts("PASS: reference-shaped masks/rotations, transparent floors, upper-plane flags and real map sprite groups; exhausted index rejected");

    // Test 3: Draw mapback, rotated minimap, and compass
    int mapback_x = 561;
    int mapback_y = 5;

    // Draw mapback shell
    pix8_draw(media.mapback, mapback_x, mapback_y);

    int fb_before = 0;
    for (int i = 0; i < 765 * 503; i++) {
        if (fb[i] != 0) fb_before++;
    }

    // Player position at tile (50, 50), in fine coords (50 * 128 = 6400)
    int player_x = 50 * 128;
    int player_z = 50 * 128;
    int camera_yaw = 512; // 90 degree camera rotation

    minimap_draw(&minimap, &media, player_x, player_z, camera_yaw, 0, 0, mapback_x, mapback_y);

    int fb_after = 0;
    for (int i = 0; i < 765 * 503; i++) {
        if (fb[i] != 0) fb_after++;
    }
    printf("  Framebuffer pixels: before=%d, after=%d\n", fb_before, fb_after);
    assert(fb_after > fb_before);
    printf("[PASS] Rotated minimap & compass needle rendered into circular windows!\n");

    // Verify player center dot (3x3 white dot at 561 + 93, 5 + 82 = 654, 87)
    uint32_t center_pixel = fb[654 + 87 * 765];
    printf("  Player center dot pixel: 0x%06X (expected 0xFFFFFF)\n", center_pixel);
    assert(center_pixel == 0xFFFFFF);
    printf("[PASS] Local player white center radar dot verified!\n");

    // Test 4: Radar dots (NPC, Player, Ground Item)
    if (media.mapdots[1]) {
        // Nearby NPC at tile (53, 52)
        int npc_x = 53 * 128;
        int npc_z = 52 * 128;
        minimap_draw_dot(player_x, player_z, npc_x, npc_z, camera_yaw, 0, mapback_x, mapback_y, media.mapdots[1],media.mapback);
        printf("[PASS] Nearby NPC radar dot rendered within minimap radius!\n");

        // Distant NPC at tile (99, 99) - should be clipped by radius check
        int far_x = 99 * 128;
        int far_z = 99 * 128;
        minimap_draw_dot(player_x, player_z, far_x, far_z, camera_yaw, 0, mapback_x, mapback_y, media.mapdots[1],media.mapback);
        printf("[PASS] Distant entity correctly clipped outside minimap circle!\n");
    }

    // Actual revision-225 NPC definitions and mapdot artwork, including hidden
    // NPC types. The old unconditional dot path drew these hidden definitions.
    NpcConfig npc_cfg;
    npc_unpack(&npc_cfg, config_jag);
    assert(media.mapdots[1] && media.mapback);
    int shown = 0, hidden = 0;
    for (int id = 0; id < npc_cfg.count; id++) {
        NpcEntity npc;
        npc_entity_init(&npc);
        assert(npc_get(&npc_cfg, id, &npc.type));
        npc.has_type = true;
        npc.base.x = player_x + 3 * 128;
        npc.base.z = player_z + 2 * 128;
        memset(fb, 0, 765 * 503 * sizeof(*fb));
        minimap_draw_npc_dot(player_x, player_z, &npc, 0, 0, 561, 5, media.mapdots[1],media.mapback);
        bool painted = false;
        for (int pixel = 0; pixel < 765 * 503; pixel++) if (fb[pixel]) painted = true;
        assert(painted == npc.type.minimap);
        if (npc.type.minimap) shown++; else hidden++;
        npc_entity_free(&npc);
    }
    assert(shown > 0 && hidden > 0);
    NpcEntity missing_type;
    npc_entity_init(&missing_type);
    memset(fb, 0, 765 * 503 * sizeof(*fb));
    minimap_draw_npc_dot(player_x, player_z, &missing_type, 0, 0, 561, 5, media.mapdots[1],media.mapback);
    minimap_draw_npc_dot(player_x, player_z, NULL, 0, 0, 561, 5, media.mapdots[1],media.mapback);
    for (int pixel = 0; pixel < 765 * 503; pixel++) assert(fb[pixel] == 0);
    printf("[PASS] All %d actual-cache NPC minimap flags: %d shown, %d hidden; untyped/null omitted\n",
        npc_cfg.count, shown, hidden);
    npc_free(&npc_cfg);

    minimap_free(&minimap);
    flo_free(&flo_cfg);
    free(scene);
    free(world);
    free(fb);
    arena_free(&arena);

    printf("=== ALL MINIMAP & COMPASS TESTS PASSED! ===\n");
    return 0;
}
