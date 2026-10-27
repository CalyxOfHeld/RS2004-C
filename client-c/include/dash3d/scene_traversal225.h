#ifndef RS2_SCENE_TRAVERSAL225_H
#define RS2_SCENE_TRAVERSAL225_H
#include "dash3d/occlusion225.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Isolated revision-225 scene snapshot. Identity indices preserve shared
 * Ground/Location objects and LinkList's unlink-and-move queue semantics. */
typedef struct {bool present;int32_t x,y,z,bitset,type_a,type_b;int32_t max_y_a,max_y_b;uint8_t model_a[32],model_b[32];} SceneWall225;
typedef struct {bool present;int32_t x,y,z,bitset,type,angle,max_y;uint8_t model[32];} SceneDecor225;
typedef struct {bool present;int32_t x,y,z,bitset,max_y;uint8_t model[32];} SceneGroundDecor225;
typedef struct {bool present;int32_t x,y,z,bitset,offset;uint8_t bottom[32],middle[32],top[32];} SceneGroundObject225;
typedef struct {
    int32_t id,level,x,z,occlude_level,draw_level;
    int32_t loc_count,loc_spans,check_loc_spans,block_loc_spans,inverse_block_loc_spans,back_wall_types;
    bool visible,update,contains_locs,underlay,overlay;
    int32_t bridge,prev,next;
    SceneWall225 wall;SceneDecor225 decor;SceneGroundDecor225 ground_decor;SceneGroundObject225 ground_obj;
    int32_t locs[5],loc_span[5];
} SceneGround225;
typedef struct {
    int32_t id,level,x,y,z,yaw,min_x,max_x,min_z,max_z,distance,cycle,bitset,info,max_y;
    int32_t entity_resolution;uint8_t model[32];
} SceneLocation225;
typedef enum {SCENE225_UNDERLAY=1,SCENE225_OVERLAY=2,SCENE225_MODEL=3} SceneCallKind225;
typedef struct {
    SceneCallKind225 kind;int32_t ground_id,level,x,z,yaw,dx,dy,dz,bitset;
    uint8_t model[32];
} SceneCall225;
typedef bool (*SceneEmit225)(void *user,const SceneCall225 *call);
/* Called only when Location.model is absent, at the source entity.draw(true)
 * point before occlusion. The resolved model stays valid through that draw. */
typedef bool (*SceneEntity225)(void *user,const SceneLocation225 *location,
    uint8_t model[32],int32_t *max_y);
typedef struct {
    int32_t max_level,grid_x,grid_z,min_level,cycle,tiles_remaining;
    int32_t eye_x,eye_y,eye_z,eye_tile_x,eye_tile_z,top_level;
    int32_t min_x,max_x,min_z,max_z;
    SceneGround225 *grounds;size_t ground_count;
    SceneLocation225 *locations;size_t location_count;
    /* Grid indexes Ground identities; -1 means no tile. */
    int32_t *grid;
    Occlusion225 occlusion;
    bool taking_input;
    SceneEmit225 emit;void *emit_user;
    SceneEntity225 resolve_entity;void *entity_user;
} SceneTraversal225;

/* Traverses the source drawTile queue from a captured first-entry state.
 * This is separate from installed world3d_render and never owns graph data. */
bool world3d_traverse225(SceneTraversal225 *scene,int32_t root_id,bool first_pass);
/* Runs both source camera-quadrant sweeps from a captured drawTile entry. */
bool world3d_traverse_frame225(SceneTraversal225 *scene,int32_t expected_root_id);
#endif
