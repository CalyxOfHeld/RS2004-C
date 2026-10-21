#ifndef RS2_EFFECTS_H
#define RS2_EFFECTS_H
#include "config/spotanim.h"
#include "net/entity_sync.h"
#include "dash3d/world.h"
typedef struct {
    bool mobile,complete,projectile;
    int id,level,src_x,src_z,src_y,offset_y,start_cycle,last_cycle,peak_pitch,arc,target;
    int yaw,pitch,frame,frame_cycle;
    double x,y,z,vx,vz,velocity,vy,ay;
} SceneEffect;
typedef struct SceneEffects {
    SceneEffect *items;size_t count,capacity;
    SpotAnimConfig *config;bool trace;int cycle;
} SceneEffects;
void projectile_velocity(SceneEffect *p,int y,int z,int x,int cycle);
void effect_advance(SceneEffect *p,const SpotAnimConfig *cfg,int delta);
bool effects_add_packet(SceneEffects *fx,World *world,World3D *scene,int level,int bx,int bz,int op,const uint8_t *data);
void effects_tick(SceneEffects *fx,World *world,World3D *scene,EntitySyncContext *entities,int cycle);
void effects_clear(SceneEffects *fx);
void effects_dispose(SceneEffects *fx);
void effects_draw(SceneEffects *fx,Arena *frame,int level,int ex,int ey,int ez,int pitch,int yaw);
void actor_spotanim_tick(PathingEntity *actor,const SpotAnimConfig *cfg,int cycle);
Model *actor_spotanim_model(PathingEntity *actor,SpotAnimConfig *cfg,Arena *frame);
Model *actor_with_spotanim(Model *body,PathingEntity *actor,SpotAnimConfig *cfg,Arena *frame);
#endif
