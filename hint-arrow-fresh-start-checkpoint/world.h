#ifndef RS2_WORLD_H
#define RS2_WORLD_H

#include <stdint.h>
#include <stdbool.h>
#include "dash3d/world3d.h"
#include "dash3d/collision_map.h"
#include "config/flo.h"
#include "config/loc.h"

#define WORLD_SIZE_X 104
#define WORLD_SIZE_Z 104
#define WORLD_LEVELS 4

typedef struct {
    int max_tile_x;
    int max_tile_z;
    int heightmap[WORLD_LEVELS][WORLD_SIZE_X + 1][WORLD_SIZE_Z + 1];
    uint8_t tile_flags[WORLD_LEVELS][WORLD_SIZE_X][WORLD_SIZE_Z];
    uint8_t tile_underlays[WORLD_LEVELS][WORLD_SIZE_X][WORLD_SIZE_Z];
    uint8_t tile_overlays[WORLD_LEVELS][WORLD_SIZE_X][WORLD_SIZE_Z];
    uint8_t tile_overlay_shapes[WORLD_LEVELS][WORLD_SIZE_X][WORLD_SIZE_Z];
    uint8_t tile_overlay_rotations[WORLD_LEVELS][WORLD_SIZE_X][WORLD_SIZE_Z];
    int8_t  shademap[WORLD_LEVELS][WORLD_SIZE_X + 1][WORLD_SIZE_Z + 1];
    int blend_direction_map[WORLD_LEVELS][WORLD_SIZE_X][WORLD_SIZE_Z];
} World;

void world_init(World *w, int max_x, int max_z);
void world_reset(World *w);
int  world_perlin_noise(int x, int z);
void world_load_ground(World *w, const uint8_t *data, size_t len, int x_offset, int z_offset, int base_x, int base_z);
void world_load_locations(World *w, const uint8_t *data, size_t len, LocConfig *loc_cfg, World3D *world3d, CollisionMap *cm, int x_offset, int z_offset);
int world_location_layer(int shape);
bool world_change_location(World *w,World3D *scene,LocConfig *cfg,CollisionMap *cm,int level,int x,int z,int id,int shape,int rotation);
void world_build(World *w, World3D *world3d, FloConfig *flo_cfg, CollisionMap *cm);

#endif // RS2_WORLD_H
