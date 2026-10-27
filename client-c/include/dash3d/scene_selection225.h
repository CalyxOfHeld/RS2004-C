#ifndef RS2_SCENE_SELECTION225_H
#define RS2_SCENE_SELECTION225_H
#include "dash3d/visibility225.h"

/* Separate original scene selection state; not the installed GroundTile. */
typedef struct {
    bool present;
    int32_t draw_level,height,loc_count;
    bool visible,update,contains_locs;
    int32_t check_loc_spans;
} SceneTileState225;
typedef struct {
    int32_t max_x,max_z,max_level,min_level;
    /* Indexed (level * max_x + x) * max_z + z. */
    SceneTileState225 *tiles;
} SceneGrid225;
typedef struct {
    int32_t cycle,sin_pitch,cos_pitch,sin_yaw,cos_yaw;
    int32_t eye_x,eye_y,eye_z,eye_tile_x,eye_tile_z,top_level;
    int32_t min_x,min_z,max_x,max_z,tiles_remaining,map_pitch,map_yaw;
} SceneSelection225;

/* World3D.draw camera setup and tile eligibility only. Caller supplies
 * initialized trig/visibility tables and prior per-tile state/cycle.
 * Accepts original addressable pitch range 97..383 and yaw 0..2047.
 * Excludes updateActiveOccluders, drawTile dispatch and input completion. */
bool world3d_select_tiles225(const SceneGrid225 *grid,const Visibility225 *visibility,
    SceneSelection225 *view,int32_t eye_x,int32_t eye_y,int32_t eye_z,
    int32_t pitch,int32_t yaw,int32_t top_level);
#endif
