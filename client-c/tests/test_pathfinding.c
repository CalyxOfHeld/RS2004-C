#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "dash3d/collision_map.h"
#include "dash3d/pathfinder.h"
#include "dash3d/world3d.h"
#include "entity/pathing_entity.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"

int main(void) {
    printf("=== Test Suite: Viewport Ground Click & BFS Pathfinding (Rev 225) ===\n");

    // 1. Basic Pathfinding on Open Ground
    CollisionMap cm;
    collision_map_init(&cm, 104, 104);

    Pathfinder pf;
    pathfinder_init(&pf);

    PathResult res;
    bool found = pathfinder_find_path(&pf, &cm, 30, 30, 35, 30, true, &res);
    assert(found == true);
    assert(res.reached_destination == true);
    assert(res.step_count == 5);
    assert(res.steps_x[res.step_count - 1] == 35);
    assert(res.steps_z[res.step_count - 1] == 30);
    printf("[PASS] Straight-line BFS pathfinding verified (5 steps)\n");

    // 2. Obstacle Avoidance (Navigate around a solid wall)
    // Wall placed at x=32 from z=28 to z=32, blocking straight route
    collision_map_add_loc(&cm, 32, 28, 1, 5, true);

    found = pathfinder_find_path(&pf, &cm, 30, 30, 35, 30, true, &res);
    assert(found == true);
    assert(res.reached_destination == true);
    assert(res.steps_x[res.step_count - 1] == 35);
    assert(res.steps_z[res.step_count - 1] == 30);

    // Verify path never stepped through the blocked wall
    for (int i = 0; i < res.step_count; i++) {
        int sx = res.steps_x[i];
        int sz = res.steps_z[i];
        bool is_wall = (sx == 32 && sz >= 28 && sz <= 32);
        assert(!is_wall);
    }
    printf("[PASS] Obstacle avoidance verified (routed around solid wall with %d steps)\n", res.step_count);

    // 3. Blocked Target Tile with find_nearest Fallback
    // Target (40, 40) is completely blocked
    collision_map_add_loc(&cm, 40, 40, 1, 1, true);

    found = pathfinder_find_path(&pf, &cm, 35, 40, 40, 40, true, &res);
    assert(found == true);
    assert(res.reached_destination == false);
    assert(res.reached_nearest == true);

    int final_x = res.steps_x[res.step_count - 1];
    int final_z = res.steps_z[res.step_count - 1];
    int dx = abs(final_x - 40);
    int dz = abs(final_z - 40);
    assert(dx <= 1 && dz <= 1 && (dx != 0 || dz != 0)); // Adjacent tile in 8 directions
    printf("[PASS] find_nearest fallback verified (reached adjacent tile at %d, %d)\n", final_x, final_z);

    // 4. Ground Click Raycasting in World3D
    uint32_t *fb = (uint32_t*)calloc(512 * 334, sizeof(uint32_t));
    pix2d_bind(fb, 512, 334);
    pix2d_cls();
    pix3d_init();
    pix3d_init_colour_table(0.8);
    pix3d_init_2d();
    pix3d_center_x = 256;
    pix3d_center_y = 167;

    World3D *world = (World3D*)calloc(1, sizeof(World3D));
    assert(world != NULL);
    world3d_init(world, 104, 104, 4);

    // Set flat ground at (34, 34)
    world3d_set_tile(world, 0, 34, 34, 0, 0, -1,
                     0, 0, 0, 0,
                     5000, 5000, 5000, 5000,
                     0, 0, 0, 0,
                     0x00FF00, 0x00FF00);

    // Click at tile (34, 34) screen projection
    world3d_click(world, 256, 245);
    int eye_x = 34 * 128 + 64;
    int eye_z = 34 * 128 + 64 - 500;
    int eye_y = -300;

    world3d_render(world, eye_x, eye_y, eye_z, 128, 0, 0, 0);
    assert(world->click_tile_x == 34);
    assert(world->click_tile_z == 34);
    printf("[PASS] World3D ground click raycasting accurately hit tile (34, 34)!\n");

    free(world);
    free(fb);

    // 5. PathingEntity Movement Advancement & Animation State
    PathingEntity pe;
    pathing_entity_init(&pe);
    pe.seq_stand_id = 808;
    pe.seq_walk_id = 819;
    pe.seq_run_id = 824;
    pe.path_tile_x[0] = 30;
    pe.path_tile_z[0] = 30;
    pe.x = 30 * 128 + 64;
    pe.z = 30 * 128 + 64;

    int path_x[] = { 31, 32, 33 };
    int path_z[] = { 30, 30, 30 };
    pathing_entity_queue_path(&pe, path_x, path_z, 3, false);
    assert(pe.path_length == 3);

    // Simulate movement cycles
    for (int cycle = 0; cycle < 150; cycle++) {
        pathing_entity_update_movement(&pe);
        pathing_entity_update_facing(&pe);
        if (pe.path_length > 0) {
            assert(pe.secondary_seq_id == pe.seq_walk_id);
        }
    }

    assert(pe.path_length == 0);
    assert(pe.x == 33 * 128 + 64);
    assert(pe.z == 30 * 128 + 64);
    assert(pe.secondary_seq_id == pe.seq_stand_id);
    assert(pe.dst_yaw == 1536); // Facing East
    assert(pe.yaw == 1536);
    printf("[PASS] PathingEntity smoothly navigated 3 tiles east, updated walking seq, and settled into idle seq!\n");

    printf("=== ALL PATHFINDING TESTS PASSED! ===\n");
    return 0;
}
