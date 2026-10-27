#ifndef RS2_SCENE_BUILD225_H
#define RS2_SCENE_BUILD225_H
#include "dash3d/world.h"
#include "dash3d/scene_raster225.h"
typedef struct SceneBuild225 SceneBuild225;
typedef const SceneModel225 *(*SceneEntityDraw225)(void *user);
/* Construct original shared scene topology from native cache/map results.
 * Enable World.build225 before loading locations and building terrain/models;
 * construct after that build and before animation can replace borrowed models.
 * The World/World3D and their prepared models outlive this non-owning adapter. */
SceneBuild225 *world3d_construct225(World *world,World3D *native,LocConfig *locations);
void world3d_destroy_scene225(SceneBuild225 *build);
SceneTraversal225 *world3d_scene225(SceneBuild225 *build);
const SceneGeometry225 *world3d_scene_geometry225(SceneBuild225 *build);
Occluder225 *world3d_scene_pool225(SceneBuild225 *build,int32_t level,size_t *count);
const SceneModel225 *world3d_scene_resolve225(void *build,const uint8_t key[32]);
/* Refresh after native location animation replaces/frees borrowed models.
 * Recomputes bounds even when the allocator reused the same model address.
 * Structural changes/removal/reallocation of owner slots require a rebuild;
 * the borrowed owner slots must remain alive through refresh and drawing. */
bool world3d_scene_refresh225(SceneBuild225 *build);
/* Original addTemporary radius/yaw expansion and shared Ground references.
 * A direct model wins over draw_entity, exactly as Location.model does.
 * Call clear before releasing a frame arena or changing temporary storage. */
bool world3d_scene_temporary225(SceneBuild225 *build,int32_t level,int32_t x,int32_t y,int32_t z,
    int32_t radius,int32_t yaw,bool expand,int32_t bitset,const SceneModel225 *direct,
    SceneEntityDraw225 draw_entity,void *entity_user);
void world3d_scene_clear_temporary225(SceneBuild225 *build);
/* Original explicit-footprint overload, used by a player's location overlay. */
bool world3d_scene_temporary_tiles225(SceneBuild225 *build,int32_t level,int32_t x,int32_t y,int32_t z,
    int32_t yaw,int32_t bitset,const SceneModel225 *direct,SceneEntityDraw225 draw_entity,void *user,
    int32_t min_x,int32_t max_x,int32_t min_z,int32_t max_z);
SceneModel225 world3d_scene_model225(Model *model,bool pickable);
/* Frame-local original GroundObject; clear_temporary releases these keys too. */
bool world3d_scene_objects225(SceneBuild225 *build,int32_t level,int32_t x,int32_t y,int32_t z,
    Model *top,Model *bottom,Model *middle);
#endif
