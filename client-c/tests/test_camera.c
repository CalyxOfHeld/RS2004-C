#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "dash3d/camera.h"
#include "gfx/pix3d.h"

#include "../verification/completion/tools/camera_reference_control.h"

int main(void) {
    setbuf(stdout, NULL);
    printf("=== Test Suite: Viewport Camera Orbit & Tracking (Rev 225) ===\n");

    pix3d_init();

    // 1. Initialize camera at Lumbridge courtyard (tile 32, 32 -> coord 4160, 4160)
    Camera cam;
    camera_init(&cam, 4160, 4160);
    assert(cam.orbit_x == 4160);
    assert(cam.orbit_z == 4160);
    assert(cam.orbit_pitch == 128); // Authentic RS2 default pitch
    assert(cam.orbit_yaw == 0);
    printf("[PASS] Camera initialized at tile (32, 32) with authentic default pitch 128\n");

    // Mock heightmap and tile_flags
    int heightmap[4][105][105];
    uint8_t tile_flags[4][104][104];
    memset(heightmap, 0, sizeof(heightmap));
    memset(tile_flags, 0, sizeof(tile_flags));

    // Ground height at tile (32, 32) is -800
    for (int l = 0; l < 4; l++) {
        for (int x = 0; x < 105; x++) {
            for (int z = 0; z < 105; z++) {
                heightmap[l][x][z] = -800;
            }
        }
    }

    // 2. Test bilinear height sampling
    int h = camera_get_height(0, 4160, 4160, heightmap, tile_flags);
    assert(h == -800);
    printf("[PASS] Bilinear ground height sampling verified (h=%d)\n", h);

    // 3. Test camera update with static player
    camera_update(&cam, 4160, 4160, 0, heightmap, tile_flags, false, false, false, false);
    assert(cam.render_pitch == 128);
    assert(cam.render_yaw == 0);
    // At pitch 128 and yaw 0, dist = 128 * 3 + 600 = 984
    // Eye is behind the player (negative Z offset in world space)
    assert(cam.eye_z < 4160);
    assert(cam.eye_y < -800); // Eye Y is elevated above ground
    printf("[PASS] Static orbit position verified: eye=(%d, %d, %d), pitch=%d, dist=%d\n",
           cam.eye_x, cam.eye_y, cam.eye_z, cam.render_pitch, 128 * 3 + 600);

    // 4. Test pitch clamping [128, 383]
    // Press down repeatedly to attempt to lower pitch below 128
    for (int i = 0; i < 50; i++) {
        camera_update(&cam, 4160, 4160, 0, heightmap, tile_flags, false, false, false, true);
    }
    assert(cam.orbit_pitch == 128);
    printf("[PASS] Lower pitch limit strictly clamped to 128\n");

    // Press up repeatedly to raise pitch to maximum
    for (int i = 0; i < 100; i++) {
        camera_update(&cam, 4160, 4160, 0, heightmap, tile_flags, false, false, true, false);
    }
    assert(cam.orbit_pitch <= 383);
    assert(cam.orbit_pitch > 350);
    printf("[PASS] Upper pitch limit strictly clamped to 383 (reached %d)\n", cam.orbit_pitch);

    // 5. Test yaw rotation and 2048 wrap-around
    for (int i = 0; i < 100; i++) {
        camera_update(&cam, 4160, 4160, 0, heightmap, tile_flags, true, false, false, false);
    }
    assert(cam.orbit_yaw >= 0 && cam.orbit_yaw < 2048);
    printf("[PASS] Yaw rotation and 2048-angle bitmask wrap verified (yaw=%d)\n", cam.orbit_yaw);

    // 6. Test smooth player tracking
    // Teleport player 200 units away
    int target_x = 4360;
    int target_z = 4360;
    int prev_dist = abs(cam.orbit_x - target_x) + abs(cam.orbit_z - target_z);
    for (int i = 0; i < 10; i++) {
        camera_update(&cam, target_x, target_z, 0, heightmap, tile_flags, false, false, false, false);
        int cur_dist = abs(cam.orbit_x - target_x) + abs(cam.orbit_z - target_z);
        assert(cur_dist < prev_dist);
        prev_dist = cur_dist;
    }
    // After 60 cycles, camera has converged within 16 units of target (integer division threshold)
    for (int i = 0; i < 60; i++) {
        camera_update(&cam, target_x, target_z, 0, heightmap, tile_flags, false, false, false, false);
    }
    assert(abs(cam.orbit_x - target_x) < 16);
    assert(abs(cam.orbit_z - target_z) < 16);
    printf("[PASS] Smooth camera orbit convergence verified (diff_x=%d, diff_z=%d)!\n",
           abs(cam.orbit_x - target_x), abs(cam.orbit_z - target_z));

    const char *reference=getenv("RS2_CAMERA_ORACLE");
    camera_reference_run(reference && *reference?reference:"tests/fixtures/camera-packets-reference.bin",&cam,heightmap,tile_flags,NULL);
    camera_fault_controls();
    printf("=== ALL CAMERA TESTS PASSED! ===\n");
    return 0;
}
