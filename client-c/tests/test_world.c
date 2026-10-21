#include "dash3d/world.h"
#include "dash3d/world3d.h"
#include "dash3d/collision_map.h"
#include "dash3d/camera.h"
#include "dash3d/map_streamer.h"
#include "config/flo.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include "core/packet.h"
#include "core/bzip2.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static uint8_t* decompress_map_file(const char* path, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* comp = (uint8_t*)malloc(sz);
    fread(comp, 1, sz, f);
    fclose(f);

    Packet pkt = packet_wrap(comp, (size_t)sz);
    uint32_t uncomp_len = g4(&pkt);
    uint8_t* uncomp = (uint8_t*)malloc(uncomp_len);

    bzip_decompress((int8_t*)uncomp, (int8_t*)comp, (int)sz - 4, 4);
    free(comp);

    *out_len = (size_t)uncomp_len;
    return uncomp;
}

int main(void) {
    setbuf(stdout, NULL);
    printf("=== Test Suite: Dash3D World & Map Construction ===\n");

    // 1. Verify Perlin Noise
    pix3d_init();
    int noise_val = world_perlin_noise(1000, 2000);
    printf("[INFO] Perlin noise at (1000, 2000): %d\n", noise_val);
    assert(noise_val >= 10 && noise_val <= 60);
    printf("[PASS] Perlin noise algorithm verified\n");

    // Height lighting and weighted underlay blending fixtures from ClientBuild.
    World *fixture=calloc(1,sizeof(World));World3D *scene_fixture=calloc(1,sizeof(World3D));
    world_init(fixture,104,104);world3d_init(scene_fixture,104,104,4);
    FloType floors[2]={0};floors[0].texture=floors[1].texture=-1;
    floors[0].chroma=32;floors[0].luminance=64;floors[0].saturation=128;floors[0].lightness=128;
    floors[1].chroma=48;floors[1].luminance=96;floors[1].saturation=192;floors[1].lightness=96;
    FloConfig fixture_flo={2,floors};
    for(int x=0;x<104;x++) for(int z=0;z<104;z++) fixture->tile_underlays[0][x][z]=x<=20 ? 1:2;
    world_build(fixture,scene_fixture,&fixture_flo,NULL);
    // 5 columns of each floor in the reference sliding window: hue=128,
    // sat=160, light=112 -> packed 33464. Flat normal gives light 84.
    assert(scene_fixture->tiles[0][20][20].underlay.southwest_color==33444);
    fixture->shademap[0][20][20]=50;world_build(fixture,scene_fixture,&fixture_flo,NULL);
    assert(scene_fixture->tiles[0][20][20].underlay.southwest_color==33433);
    world3d_dispose(scene_fixture);free(scene_fixture);free(fixture);
    puts("PASS: reference weighted floor blend, flat-normal light and cast-shadow brightness golden values");

    // 2. Load Config FloType for tile color palettes
    Arena arena = arena_create(16 * 1024 * 1024);
    Jagfile* config_jag = jagfile_load_file("../engine/data/pack/client/config", &arena);
    if (!config_jag) {
        config_jag = jagfile_load_file("engine/data/pack/client/config", &arena);
    }
    assert(config_jag != NULL);
    FloConfig flo_cfg;
    flo_unpack(&flo_cfg, config_jag);
    assert(flo_cfg.count > 0);
    printf("[PASS] FloConfig loaded (%d floor types)\n", flo_cfg.count);

    // 3. Decompress Lumbridge map files (m47_47 and l47_47)
    const char* m_path = "../engine/data/pack/client/maps/m47_47";
    const char* l_path = "../engine/data/pack/client/maps/l47_47";
    size_t m_len = 0, l_len = 0;
    uint8_t* m_data = decompress_map_file(m_path, &m_len);
    if (!m_data) {
        m_path = "engine/data/pack/client/maps/m47_47";
        m_data = decompress_map_file(m_path, &m_len);
    }
    assert(m_data != NULL);

    uint8_t* l_data = decompress_map_file(l_path, &l_len);
    if (!l_data) {
        l_path = "engine/data/pack/client/maps/l47_47";
        l_data = decompress_map_file(l_path, &l_len);
    }
    assert(l_data != NULL);

    printf("[PASS] Decompressed m47_47 (%zu bytes) and l47_47 (%zu bytes)\n", m_len, l_len);

    // 4. Initialize World & CollisionMaps
    World* world = (World*)calloc(1, sizeof(World));
    assert(world != NULL);
    world_init(world, 104, 104);

    CollisionMap collision_maps[4];
    for (int i = 0; i < 4; i++) {
        collision_map_init(&collision_maps[i], 104, 104);
    }

    // 5. Load Ground Landscape data into World
    world_load_ground(world, m_data, m_len, 0, 0, 47 * 64, 47 * 64);
    free(m_data);

    // Verify non-zero heightmaps and underlays
    int non_zero_heights = 0;
    int non_zero_underlays = 0;
    for (int x = 0; x < 64; x++) {
        for (int z = 0; z < 64; z++) {
            if (world->heightmap[0][x][z] != 0) non_zero_heights++;
            if (world->tile_underlays[0][x][z] != 0) non_zero_underlays++;
        }
    }
    printf("[INFO] Non-zero terrain heights: %d, non-zero underlays: %d\n", non_zero_heights, non_zero_underlays);
    assert(non_zero_heights > 0);
    assert(non_zero_underlays > 0);
    printf("[PASS] World ground landscape unpacked and populated into heightmap\n");

    // 6. Load Scenery Locations into World and CollisionMap
    LocConfig loc_cfg;loc_unpack(&loc_cfg,config_jag);
    world_load_locations(world,l_data,l_len,&loc_cfg,NULL,collision_maps,0,0);
    free(l_data);

    int blocked_tiles = 0;
    for (int x = 1; x < 63; x++) {
        for (int z = 1; z < 63; z++) {
            if (collision_maps[0].flags[x][z] != 0) blocked_tiles++;
        }
    }
    printf("[INFO] Scenery objects populated collision map (%d blocked tiles)\n", blocked_tiles);
    assert(blocked_tiles > 0);
    printf("[PASS] World scenery locations successfully mapped into collision grid\n");

    // 7. Build World3D scene graph
    World3D* world3d = (World3D*)calloc(1, sizeof(World3D));
    assert(world3d != NULL);
    world3d_init(world3d, 104, 104, 4);
    // Reference Ground.ts definition tables: scene shapes are indexed directly.
    // Every rotated shape must tessellate exactly one tile, with no vertices
    // escaping to world origin (rotated interior points 13..16 included).
    for (int shape = 2; shape <= 12; shape++) {
        for (int rotation = 0; rotation < 4; rotation++) {
            world3d_set_tile(world3d, 0, 32, 32, shape, rotation, -1,
                             0, 0, 0, 0, 100, 100, 100, 100, 200, 200, 200, 200, 0, 0);
            TileOverlay *overlay = &world3d->tiles[0][32][32].overlay;
            assert(overlay->vertex_count == (shape == 2 ? 4 : shape < 7 ? 5 : 6));
            int twice_area = 0;
            for (int v = 0; v < overlay->vertex_count; v++) {
                assert(overlay->vertex_x[v] >= 4096 && overlay->vertex_x[v] <= 4224);
                assert(overlay->vertex_z[v] >= 4096 && overlay->vertex_z[v] <= 4224);
            }
            for (int t = 0; t < overlay->triangle_count; t++) {
                int a = overlay->triangle_vertex_a[t], b = overlay->triangle_vertex_b[t], c = overlay->triangle_vertex_c[t];
                twice_area += abs((overlay->vertex_x[a] - overlay->vertex_x[b]) * (overlay->vertex_z[c] - overlay->vertex_z[b]) -
                                  (overlay->vertex_z[a] - overlay->vertex_z[b]) * (overlay->vertex_x[c] - overlay->vertex_x[b]));
            }
            assert(twice_area == 2 * 128 * 128);
        }
    }
    world3d_reset(world3d);
    printf("[PASS] All 11 overlay shapes and four rotations cover exactly one tile\n");
    world_build(world, world3d, &flo_cfg, collision_maps);
    printf("[PASS] World3D scene built from landscape and config palettes\n");

    // 8. Render 3D Lumbridge Scene to Framebuffer
    int fb_w = 512, fb_h = 334;
    uint32_t* fb = (uint32_t*)calloc(fb_w * fb_h, sizeof(uint32_t));
    pix2d_bind(fb, fb_w, fb_h);
    pix2d_cls();
    pix3d_init_colour_table(0.8);
    pix3d_init_2d();

    // Render camera at tile (32, 32), height -400, looking forward
    int eye_x = 32 * 128;
    int eye_z = 32 * 128;
    int eye_y = world3d->heightmap[0][32][32] - 500;
    int pitch = 256; // angled down
    int yaw = 0;     // facing north

    world3d_render(world3d, eye_x, eye_y, eye_z, pitch, yaw, 0, 0);

    int non_zero_pixels = 0;
    for (int i = 0; i < fb_w * fb_h; i++) {
        if (fb[i] != 0) non_zero_pixels++;
    }
    printf("[INFO] Rendered 3D scene: %d non-zero shaded pixels out of %d total\n", non_zero_pixels, fb_w * fb_h);
    assert(non_zero_pixels > 1000);
    printf("[PASS] Live 3D terrain successfully rasterized to framebuffer!\n");

    // Exercise the gameplay viewport and camera on the real Lumbridge mapsquare.
    // content/maps/labels.txt: Lumbridge=(3239,3233), hence mapsquare 50_50.
    world_reset(world);
    world3d_reset(world3d);
    m_data = decompress_map_file("../engine/data/pack/client/maps/m50_50", &m_len);
    assert(m_data != NULL);
    world_load_ground(world, m_data, m_len, 0, 0, 3200, 3200);
    free(m_data);
    world_build(world, world3d, &flo_cfg, collision_maps);

    const int width = 765, height = 503;
    const uint32_t untouched = 0x123456;
    uint32_t *game_fb = calloc(width * height, sizeof(uint32_t));
    uint32_t *original = calloc(width * height, sizeof(uint32_t));
    assert(game_fb && original);
    const int yaws[] = { 0, 512, 1024, 1536 };
    const int pitches[] = { 128, 256, 383, 128 };
    for (int view = 0; view < 5; view++) {
        int angle = view % 4;
        if (view == 4) {
            // Use the same decoder/rebuilder as the client's packet 237 handler.
            uint8_t rebuild[14] = {0};
            Packet packet = packet_wrap(rebuild, sizeof(rebuild));
            p2(&packet, 406); p2(&packet, 406);
            p1(&packet, 50); p1(&packet, 50);
            p4(&packet, 0); p4(&packet, 0);
            packet.pos = 0;
            MapStreamer streamer;
            map_streamer_init(&streamer);
            assert(map_streamer_parse_rebuild_packet(&streamer, &packet));
            assert(map_streamer_rebuild(&streamer, world, world3d, collision_maps, &flo_cfg, NULL,
                                       "../engine/data/pack/client/maps"));
        }
        Camera cam;
        camera_init(&cam, 32 * 128 + 64, 32 * 128 + 64);
        cam.orbit_yaw = yaws[angle];
        cam.orbit_pitch = pitches[angle];
        camera_update(&cam, cam.orbit_x, cam.orbit_z, 0, world3d->heightmap, world->tile_flags,
                      false, false, false, false);
        for (int i = 0; i < width * height; i++) game_fb[i] = untouched;
        pix2d_bind(game_fb, width, height);
        pix3d_init_2d();
        pix2d_set_clip(8, 11, 520, 345);
        pix2d_fill_rect(8, 11, 512, 334, 0);
        pix3d_center_x = 264; pix3d_center_y = 178;
        world3d_render(world3d, cam.eye_x, cam.eye_y, cam.eye_z, cam.render_pitch, cam.render_yaw, 0, 0);
        int grass = 0, road = 0, lower_left = 0, lower_right = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t rgb = game_fb[y * width + x];
                if (x < 8 || x >= 520 || y < 11 || y >= 345) {
                    assert(rgb == untouched && "terrain must stay inside the gameplay viewport");
                    continue;
                }
                int r = (rgb >> 16) & 255, g = (rgb >> 8) & 255, b = rgb & 255;
                bool is_grass = g > r + 10 && g > b + 10;
                bool is_road = r > 40 && abs(r - g) < 15 && abs(g - b) < 15;
                grass += is_grass;
                road += is_road;
                if (y >= 220 && (is_grass || is_road)) {
                    if (x < 264) lower_left++; else lower_right++;
                }
            }
        }
        printf("[GAMEPLAY] yaw=%d pitch=%d grass=%d road=%d lower=(%d,%d) rebuild=%d\n",
               yaws[angle], pitches[angle], grass, road, lower_left, lower_right, view == 4);
        assert(grass > 1000 && road > 100);
        assert(lower_left > 1000 && lower_right > 1000);
        if (view == 0) memcpy(original, game_fb, width * height * sizeof(uint32_t));
        if (view == 4) assert(memcmp(original, game_fb, width * height * sizeof(uint32_t)) == 0);
    }
    world3d_reset(world3d);
    int32_t px[]={-256,0,256}, py[]={-128,128,-128}, pz[]={0,0,0};
    int32_t fa[]={0},fbv[]={1},fc[]={2},red[]={1},blue[]={2};
    Model near_model={0},far_model={0};
    near_model.num_points=3; near_model.num_faces=1;
    near_model.point_x=px; near_model.point_y=py; near_model.point_z=pz;
    near_model.face_vertex_a=fa; near_model.face_vertex_b=fbv; near_model.face_vertex_c=fc;
    near_model.face_colour=near_model.face_colour_a=near_model.face_colour_b=near_model.face_colour_c=red;
    far_model=near_model;
    far_model.face_colour=far_model.face_colour_a=far_model.face_colour_b=far_model.face_colour_c=blue;
    pix3d_colour_table[1]=0xff0000; pix3d_colour_table[2]=0x0000ff;
    assert(world3d_add_loc(world3d,0,32,32,0,&near_model,0,1,1));
    assert(world3d_add_loc(world3d,0,33,35,0,&far_model,0,1,1));
    pix2d_cls();
    world3d_render(world3d,4160,0,3500,0,0,0,0); pix3d_end_scene();
    assert(game_fb[178*width+264]==0xff0000);
    memcpy(original,game_fb,width*height*sizeof(uint32_t));
    WorldLocation temp=world3d->tiles[0][32][32].locs[0];
    world3d->tiles[0][32][32].locs[0]=world3d->tiles[0][33][35].locs[0];
    world3d->tiles[0][33][35].locs[0]=temp;
    pix2d_cls();
    world3d_render(world3d,4160,0,3500,0,0,0,0); pix3d_end_scene();
    assert(memcmp(original,game_fb,width*height*sizeof(uint32_t))==0);
    puts("[PASS] Overlapping scenery identical after reversing world tile submission");

    free(original);
    free(game_fb);
    printf("[PASS] Real Lumbridge grass/paths visible at gameplay camera angles, viewport clipping and rebuild verified\n");

    free(fb);
    free(world3d);
    free(world);
    loc_free(&loc_cfg);
    flo_free(&flo_cfg);
    arena_free(&arena);

    printf("=== ALL WORLD & SCENE TESTS PASSED! ===\n");
    return 0;
}
