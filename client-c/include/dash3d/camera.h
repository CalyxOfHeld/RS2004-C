#ifndef RS2_CAMERA_H
#define RS2_CAMERA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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
    bool cutscene;
    int src_x,src_z,src_height,move_speed,move_acceleration;
    int dst_x,dst_z,dst_height,rotate_speed,rotate_acceleration;
    bool modifier_enabled[5];
    int modifier_jitter[5],modifier_scale[5],modifier_speed[5],modifier_cycle[5];
} Camera;

void camera_init(Camera *cam, int initial_x, int initial_z);

int camera_get_height(int level, int x, int z, int heightmap[4][105][105], uint8_t tile_flags[4][104][104]);

void camera_update(Camera *cam, int target_x, int target_z, int current_level,
                   int heightmap[4][105][105], uint8_t tile_flags[4][104][104],
                   bool key_left, bool key_right, bool key_up, bool key_down);

void camera_prepare_draw(Camera *cam, int player_x, int player_z, int current_level,
                         int heightmap[4][105][105], uint8_t tile_flags[4][104][104]);

bool camera_packet225(Camera *cam,int opcode,const uint8_t *payload,size_t length,int level,
                      int heightmap[4][105][105],uint8_t flags[4][104][104]);
void camera_cutscene_tick225(Camera *cam,int level,int heightmap[4][105][105],uint8_t flags[4][104][104]);
void camera_modifier_tick225(Camera *cam);
void camera_shake225(const Camera *cam,Camera *view);
int camera_cutscene_top225(const Camera *cam,int level,int heightmap[4][105][105],uint8_t flags[4][104][104]);
#endif // RS2_CAMERA_H
