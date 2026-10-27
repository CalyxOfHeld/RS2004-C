#ifndef RS2_OCCLUSION225_H
#define RS2_OCCLUSION225_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    int32_t min_tile_x,max_tile_x,min_tile_z,max_tile_z,type;
    int32_t min_x,max_x,min_z,max_z,min_y,max_y,mode;
    int32_t min_delta_x,max_delta_x,min_delta_z,max_delta_z,min_delta_y,max_delta_y;
} Occluder225;
typedef struct {
    Occluder225 *pool;
    size_t count,active_count;
    uint16_t active[500];
    int32_t eye_x,eye_y,eye_z,eye_tile_x,eye_tile_z,cycle;
    int32_t max_x,max_z,levels;
    /* Heights and cycles: [levels][max_x+1][max_z+1], as in Java. */
    const int32_t *heights;
    int32_t *tile_cycles;
} Occlusion225;

/* Separate source updateActiveOccluders/occluded and cached predicates.
 * Valid pools contain at most 500 entries; caller owns storage and supplies
 * a selected 51x51 visibility map. Query coordinates must be valid tiles.
 * As in Java, predicate true means occluded, despite source "Visible" names. */
bool world3d_occlusion_activate225(Occlusion225 *state,const bool map[51][51]);
bool world3d_occluded225(const Occlusion225 *state,int32_t x,int32_t y,int32_t z);
bool world3d_tile_occluded225(Occlusion225 *state,int32_t level,int32_t x,int32_t z);
bool world3d_wall_occluded225(Occlusion225 *state,int32_t level,int32_t x,int32_t z,int32_t type);
bool world3d_object_occluded225(Occlusion225 *state,int32_t level,int32_t x,int32_t z,int32_t height);
bool world3d_loc_occluded225(Occlusion225 *state,int32_t level,int32_t min_x,int32_t max_x,int32_t min_z,int32_t max_z,int32_t height);
#endif
