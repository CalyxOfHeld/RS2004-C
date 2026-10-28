#ifndef RS2_SCENE_RUNTIME225_H
#define RS2_SCENE_RUNTIME225_H
#include "dash3d/scene_build225.h"
#include "dash3d/effects.h"
#include "entity/player_entity.h"
#include "entity/npc_entity.h"

typedef struct SceneRuntime225 SceneRuntime225;
typedef struct {
    PlayerEntity *local;NpcEntity *offline_npc;EntitySyncContext *entities;SceneEffects *effects;
    int cycle,level,eye_x,eye_y,eye_z,pitch,yaw,top_level;
    bool check_hover,taking_input;
    int mouse_x,mouse_y,ground_mouse_x,ground_mouse_y,click_x,click_z;
} SceneFrame225;
SceneRuntime225 *scene_runtime225_create(World *world,World3D *scene,LocConfig *locations,
    SeqConfig *sequences,IdkConfig *identities,ObjConfig *objects,SpotAnimConfig *spots);
void scene_runtime225_destroy(SceneRuntime225 *runtime);
/* Caller binds the original 512x334 surface and palette/cache. Input positions
 * are relative to that surface. top_level=-1 derives client.getTopLevel. */
bool scene_runtime225_frame(SceneRuntime225 *runtime,const SceneFrame225 *frame,
    SceneEmit225 observer,void *user);
SceneBuild225 *scene_runtime225_build(SceneRuntime225 *runtime);
const SceneRaster225 *scene_runtime225_result(SceneRuntime225 *runtime);
int scene_runtime225_top_level(const World *world,int level,int eye_x,int eye_z,
    int player_x,int player_z,int pitch);
/* Optional frame-phase timing hook (Wii U hardware profiling); NULL on PC. */
enum { PERF225_PRE, PERF225_SYNC, PERF225_ACTORS, PERF225_PREPARE, PERF225_RASTER, PERF225_OVERLAY };
extern void (*scene_perf_mark225)(int phase);
#define SCENE_PERF225(phase) do{if(scene_perf_mark225)scene_perf_mark225(phase);}while(0)
#endif
