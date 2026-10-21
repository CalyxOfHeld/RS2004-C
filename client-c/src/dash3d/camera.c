#include "dash3d/camera.h"
#include "gfx/pix3d.h"
#include <string.h>

void camera_init(Camera *cam, int initial_x, int initial_z) {
    if (!cam) return;
    memset(cam, 0, sizeof(Camera));
    cam->orbit_x = initial_x;
    cam->orbit_z = initial_z;
    cam->orbit_yaw = 0;
    cam->orbit_pitch = 128; // Authentic RS2 default pitch
    cam->yaw_velocity = 0;
    cam->pitch_velocity = 0;
    cam->pitch_clamp = 32768;
}

int camera_get_height(int level, int x, int z, int heightmap[4][105][105], uint8_t tile_flags[4][104][104]) {
    int tx = x >> 7;
    int tz = z >> 7;
    if (tx < 0 || tx >= 103 || tz < 0 || tz >= 103) {
        return 0;
    }
    int eff_level = level;
    if (eff_level < 3 && tile_flags && (tile_flags[1][tx][tz] & 0x2) == 2) {
        eff_level = level + 1;
    }
    int rx = x & 0x7F;
    int rz = z & 0x7F;

    int h00 = heightmap[eff_level][tx][tz];
    int h10 = heightmap[eff_level][tx + 1][tz];
    int h01 = heightmap[eff_level][tx][tz + 1];
    int h11 = heightmap[eff_level][tx + 1][tz + 1];

    int h0 = (h00 * (128 - rx) + h10 * rx) >> 7;
    int h1 = (h01 * (128 - rx) + h11 * rx) >> 7;
    return (h0 * (128 - rz) + h1 * rz) >> 7;
}

void camera_update(Camera *cam, int target_x, int target_z, int current_level,
                   int heightmap[4][105][105], uint8_t tile_flags[4][104][104],
                   bool key_left, bool key_right, bool key_up, bool key_down) {
    if (!cam) return;

    // 1. Smoothly track camera target (client.java:6658-6670)
    if (cam->orbit_x - target_x < -500 || cam->orbit_x - target_x > 500 ||
        cam->orbit_z - target_z < -500 || cam->orbit_z - target_z > 500) {
        cam->orbit_x = target_x;
        cam->orbit_z = target_z;
    }
    if (cam->orbit_x != target_x) {
        cam->orbit_x += (target_x - cam->orbit_x) / 16;
    }
    if (cam->orbit_z != target_z) {
        cam->orbit_z += (target_z - cam->orbit_z) / 16;
    }

    // 2. Keyboard Velocity Update (client.java:6671-6684)
    if (key_left) {
        cam->yaw_velocity += (-24 - cam->yaw_velocity) / 2;
    } else if (key_right) {
        cam->yaw_velocity += (24 - cam->yaw_velocity) / 2;
    } else {
        cam->yaw_velocity /= 2;
    }

    if (key_up) {
        cam->pitch_velocity += (12 - cam->pitch_velocity) / 2;
    } else if (key_down) {
        cam->pitch_velocity += (-12 - cam->pitch_velocity) / 2;
    } else {
        cam->pitch_velocity /= 2;
    }

    cam->orbit_yaw = (cam->orbit_yaw + cam->yaw_velocity / 2) & 0x7FF;
    cam->orbit_pitch += cam->pitch_velocity / 2;

    if (cam->orbit_pitch < 128) {
        cam->orbit_pitch = 128;
    }
    if (cam->orbit_pitch > 383) {
        cam->orbit_pitch = 383;
    }

    // 3. Terrain collision & pitch clamping (client.java:6694-6725)
    int var4 = cam->orbit_x >> 7;
    int var5 = cam->orbit_z >> 7;
    int var6 = camera_get_height(current_level, cam->orbit_x, cam->orbit_z, heightmap, tile_flags);
    int var7 = 0;
    int var8 = 0;

    if (var4 > 3 && var5 > 3 && var4 < 100 && var5 < 100) {
        for (var8 = var4 - 4; var8 <= var4 + 4; var8++) {
            for (int var9 = var5 - 4; var9 <= var5 + 4; var9++) {
                int var10 = current_level;
                if (var10 < 3 && tile_flags && (tile_flags[1][var8][var9] & 0x2) == 2) {
                    var10++;
                }
                int var11 = var6 - heightmap[var10][var8][var9];
                if (var11 > var7) {
                    var7 = var11;
                }
            }
        }
    }
    var8 = var7 * 192;
    if (var8 > 98048) var8 = 98048;
    if (var8 < 32768) var8 = 32768;

    if (var8 > cam->pitch_clamp) {
        cam->pitch_clamp += (var8 - cam->pitch_clamp) / 24;
    } else if (var8 < cam->pitch_clamp) {
        cam->pitch_clamp += (var8 - cam->pitch_clamp) / 80;
    }

    // 4. Calculate final orbit camera angles & distance (client.java:2185)
    int pitch = cam->orbit_pitch;
    if (cam->pitch_clamp / 256 > pitch) {
        pitch = cam->pitch_clamp / 256;
    }
    int yaw = cam->orbit_yaw;
    int dist = pitch * 3 + 600;
    int target_y = camera_get_height(current_level, target_x, target_z, heightmap, tile_flags) - 50;

    // 5. Orbit Math (client.java:3326-3357)
    int var8_ang = (2048 - pitch) & 0x7FF;
    int var9_ang = (2048 - yaw) & 0x7FF;
    int var10 = 0;
    int var11 = 0;
    int var12 = dist;

    if (var8_ang != 0) {
        int sinP = pix3d_sin_table[var8_ang];
        int cosP = pix3d_cos_table[var8_ang];
        int var15 = (var11 * cosP - dist * sinP) >> 16;
        var12 = (var11 * sinP + dist * cosP) >> 16;
        var11 = var15;
    }

    if (var9_ang != 0) {
        int sinY = pix3d_sin_table[var9_ang];
        int cosY = pix3d_cos_table[var9_ang];
        int var15 = (var12 * sinY + var10 * cosY) >> 16;
        var12 = (var12 * cosY - var10 * sinY) >> 16;
        var10 = var15;
    }

    cam->eye_x = cam->orbit_x - var10;
    cam->eye_y = target_y - var11;
    cam->eye_z = cam->orbit_z - var12;
    cam->render_pitch = pitch;
    cam->render_yaw = yaw;
}
