#ifndef RS2_CAMERA_H
#define RS2_CAMERA_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int orbit_x;
    int orbit_z;
    int orbit_yaw;
    int orbit_pitch;
    int yaw_velocity;
    int pitch_velocity;
    int pitch_clamp;
    // client.cameraAnticheatOffsetX/Z and cameraAnticheatAngle (0 unless logged in).
    int anticheat_x;
    int anticheat_z;
    int anticheat_yaw;

    // Computed eye position & orientation for 3D rasterizer
    int eye_x;
    int eye_y;
    int eye_z;
    int render_pitch;
    int render_yaw;
} Camera;

void camera_init(Camera *cam, int initial_x, int initial_z);

int camera_get_height(int level, int x, int z, int heightmap[4][105][105], uint8_t tile_flags[4][104][104]);

void camera_update(Camera *cam, int target_x, int target_z, int current_level,
                   int heightmap[4][105][105], uint8_t tile_flags[4][104][104],
                   bool key_left, bool key_right, bool key_up, bool key_down);

#endif // RS2_CAMERA_H
