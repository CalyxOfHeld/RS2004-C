/* client.pushPlayers/pushNpcs/pushProjectiles/pushSpotanims and drawScene.
 * Models resolve inside traversal; frame storage outlives all their draws. */
#include "dash3d/scene_runtime225.h"
#include "dash3d/scene_prepare225.h"
#include "dash3d/camera.h"
#include "gfx/pix3d.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void (*scene_perf_mark225)(int phase);

typedef struct {
    struct SceneRuntime225 *runtime;int kind,y,cycle;bool low_memory;
    void *entity;SceneModel225 model;
} RuntimeEntity225;
struct SceneRuntime225 {
    World *world;World3D *scene;LocConfig *locations;SeqConfig *sequences;
    IdkConfig *identities;ObjConfig *objects;SpotAnimConfig *spots;
    SceneBuild225 *build;uint64_t structure_revision,build_revision;
    Visibility225 visibility;Occluder225 pools[4][500];size_t pool_counts[4];
    Arena arena;RuntimeEntity225 entities[5000];size_t entity_count;
    bool occupied[104][104];int32_t hits[1000];SceneRaster225 result;int32_t object_order[4*104*104];
};
int scene_runtime225_top_level(const World *w,int level,int ex,int ez,int px,int pz,int pitch){
    if(!w || level<0 || level>=4)return -1;
    int x=ex>>7,z=ez>>7,tx=px>>7,tz=pz>>7,top=3;
    if(x<0 || x>=104 || z<0 || z>=104 || tx<0 || tx>=104 || tz<0 || tz>=104)return level;
    if(pitch<310){
        if(w->tile_flags[level][x][z]&4)top=level;
        int dx=abs(tx-x),dz=abs(tz-z),fraction=32768;
        if(dx>dz){
            int step=dz*65536/dx;
            while(x!=tx){
                x+=x<tx?1:-1;if(w->tile_flags[level][x][z]&4)top=level;
                fraction+=step;if(fraction>=65536){fraction-=65536;z+=z<tz?1:z>tz?-1:0;if(w->tile_flags[level][x][z]&4)top=level;}
            }
        }else if(dz){
            int step=dx*65536/dz;
            while(z!=tz){
                z+=z<tz?1:-1;if(w->tile_flags[level][x][z]&4)top=level;
                fraction+=step;if(fraction>=65536){fraction-=65536;x+=x<tx?1:x>tx?-1:0;if(w->tile_flags[level][x][z]&4)top=level;}
            }
        }
    }
    if(w->tile_flags[level][tx][tz]&4)top=level;
    return top;
}
static bool synchronize(SceneRuntime225 *r){
    if(r->build && r->structure_revision==r->scene->structure_revision)return world3d_scene_refresh225(r->build);
    world3d_destroy_scene225(r->build);r->build=world3d_construct225(r->world,r->scene,r->locations);
    if(!r->build)return false;
    if(r->build_revision!=r->scene->build_revision){
        for(int level=0;level<4;level++){
            size_t count;Occluder225 *pool=world3d_scene_pool225(r->build,level,&count);
            if(count>500)return false;
            r->pool_counts[level]=count;memcpy(r->pools[level],pool,count*sizeof(*pool));
        }
        r->build_revision=r->scene->build_revision;
    }
    r->structure_revision=r->scene->structure_revision;return true;
}
SceneRuntime225 *scene_runtime225_create(World *world,World3D *scene,LocConfig *locations,
    SeqConfig *sequences,IdkConfig *identities,ObjConfig *objects,SpotAnimConfig *spots){
    if(!world || !scene || !locations || !sequences || !identities || !objects || !spots)return NULL;
    SceneRuntime225 *r=calloc(1,sizeof(*r));if(!r)return NULL;
    r->world=world;r->scene=scene;r->locations=locations;r->sequences=sequences;
    r->identities=identities;r->objects=objects;r->spots=spots;r->build_revision=UINT64_MAX;
    r->arena=arena_create(128*1024*1024);int distances[9];
    for(int i=0;i<9;i++){int angle=i*32+143;distances[i]=(angle*3+600)*pix3d_sin_table[angle]>>16;}
    if(!r->arena.capacity || !world3d_visibility_init225(&r->visibility,distances,800,512,334,500) || !synchronize(r)){
        scene_runtime225_destroy(r);return NULL;
    }
    return r;
}
void scene_runtime225_destroy(SceneRuntime225 *r){if(r){world3d_destroy_scene225(r->build);arena_free(&r->arena);free(r);}}
SceneBuild225 *scene_runtime225_build(SceneRuntime225 *r){return r?r->build:NULL;}
const SceneRaster225 *scene_runtime225_result(SceneRuntime225 *r){return r?&r->result:NULL;}
static const SceneModel225 *resolve_entity(void *context){
    RuntimeEntity225 *e=context;SceneRuntime225 *r=e->runtime;Model *model=NULL;bool pickable=false;
    if(e->kind==0){
        PlayerEntity *p=e->entity;model=player_entity_get_model(p,r->identities,r->objects,r->sequences,&r->arena);
        if(model && !e->low_memory){model=actor_with_spotanim(model,&p->base,r->spots,&r->arena);model=player_with_location(model,p,e->cycle,e->y,&r->arena);}
        pickable=true;
    }else if(e->kind==1){
        NpcEntity *n=e->entity;model=npc_entity_get_animated_model(n,r->sequences,&r->arena);
        if(model)model=actor_with_spotanim(model,&n->base,r->spots,&r->arena);
        pickable=n->type.size==1;
    }else{
        SceneEffect *p=e->entity;
        model=spotanim_model(r->spots,p->id,p->frame,p->projectile?p->pitch:0,0,!p->projectile,&r->arena);
    }
    e->model=world3d_scene_model225(model,pickable);return model?&e->model:NULL;
}
static bool add_actor(SceneRuntime225 *r,const SceneFrame225 *f,int kind,void *actor,int index,int player_count){
    PlayerEntity *p=kind==0?actor:NULL;NpcEntity *n=kind==1?actor:NULL;
    if((p && !p->visible) || (n && !n->has_type))return true;
    PathingEntity *a=p?&p->base:&n->base;int x=a->x>>7,z=a->z>>7;
    if(x<0 || x>=104 || z<0 || z>=104)return true;
    bool overlay=p && p->loc_model && f->cycle>=p->loc_start_cycle && f->cycle<p->loc_stop_cycle;
    if(!overlay && (p || a->size==1) && (a->x&127)==64 && (a->z&127)==64){
        if(r->occupied[x][z])return true;
        r->occupied[x][z]=true;
    }
    if(r->entity_count>=5000)return false;
    int y=camera_get_height(f->level,a->x,a->z,r->scene->heightmap,r->world->tile_flags);
    if(p)p->base.y=y;
    RuntimeEntity225 *e=&r->entities[r->entity_count];
    *e=(RuntimeEntity225){r,kind,y,f->cycle,p && index!=2047 &&
        ((pix3d_low_detail && player_count>50) || player_count>200) && a->secondary_seq_id==a->seq_stand_id,actor,{0}};
    int bitset=(index<<14)+(kind==1?0x20000000:0);bool added;
    if(overlay){e->low_memory=false;added=world3d_scene_temporary_tiles225(r->build,f->level,a->x,y,a->z,a->yaw,bitset,NULL,resolve_entity,e,
        p->loc_min_tile_x,p->loc_max_tile_x,p->loc_min_tile_z,p->loc_max_tile_z);}
    else added=world3d_scene_temporary225(r->build,f->level,a->x,y,a->z,p?60:(a->size-1)*64+60,a->yaw,a->seq_stretches,
        bitset,NULL,resolve_entity,e);
    /* The original callers ignore addTemporary's footprint/capacity refusal. */
    if(added)r->entity_count++;
    return true;
}
static bool add_effects(SceneRuntime225 *r,const SceneFrame225 *f){
    SceneEffects *effects=f->effects;if(!effects)return true;
    for(int kind=1;kind>=0;kind--)for(size_t i=0;i<effects->count;i++){
        SceneEffect *p=&effects->items[i];
        if(p->projectile!=(kind!=0) || p->complete || p->level!=f->level || f->cycle<p->start_cycle)continue;
        if(!isfinite(p->x) || !isfinite(p->y) || !isfinite(p->z) || fabs(p->x)>1000000 || fabs(p->y)>1000000 || fabs(p->z)>1000000)continue;
        if(r->entity_count>=5000)return false;
        RuntimeEntity225 *e=&r->entities[r->entity_count];
        *e=(RuntimeEntity225){r,2,0,f->cycle,false,p,{0}};
        if(world3d_scene_temporary225(r->build,f->level,(int)p->x,(int)p->y,(int)p->z,60,p->projectile?p->yaw:0,false,-1,NULL,resolve_entity,e))r->entity_count++;
    }
    return true;
}
static bool add_object(SceneRuntime225 *r,int level,int x,int z){
    GroundTile *tile=&r->scene->tiles[level][x][z];if(!tile->items)return true;GroundItem *items[3];int count=ground_items_models(tile,items);
    if(!count)return true;
    int y=camera_get_height(level,x*128+64,z*128+64,r->scene->heightmap,r->world->tile_flags);
    return world3d_scene_objects225(r->build,level,x,y,z,items[0]->model,count>1?items[1]->model:NULL,count>2?items[2]->model:NULL);
}
static int compare_index(const void *a,const void *b){int x=*(const int32_t *)a,y=*(const int32_t *)b;return (x>y)-(x<y);}
/* Same level/x/z order as a scan of every tile, visiting only the tiles
 * ground_items_stocked lists (a full scan is a cache miss per tile on Wii U). */
static bool add_objects(SceneRuntime225 *r){
    GroundTile *const *stocked;size_t count;GroundTile *first=&r->scene->tiles[0][0][0];
    if(ground_items_stocked(&stocked,&count) && count<=4*104*104){
        int32_t *order=r->object_order;size_t n=0;
        for(size_t i=0;i<count;i++){
            if(stocked[i]<first || stocked[i]>=first+4*104*104 || !stocked[i]->items)continue;
            order[n++]=(int32_t)(stocked[i]-first);
        }
        qsort(order,n,sizeof(*order),compare_index);
        for(size_t i=0;i<n;i++){
            if(i && order[i]==order[i-1])continue;
            if(!add_object(r,order[i]/(104*104),order[i]/104%104,order[i]%104))return false;
        }
        return true;
    }
    for(int level=0;level<4;level++)for(int x=0;x<104;x++)for(int z=0;z<104;z++)if(!add_object(r,level,x,z))return false;
    return true;
}
bool scene_runtime225_frame(SceneRuntime225 *r,const SceneFrame225 *f,SceneEmit225 observer,void *user){
    if(!r || !f || !f->local || f->level<0 || f->level>=4)return false;
    world3d_scene_clear_temporary225(r->build);arena_reset(&r->arena);r->entity_count=0;memset(r->occupied,0,sizeof(r->occupied));
    if(!synchronize(r) || !add_objects(r))return false;
    SCENE_PERF225(PERF225_SYNC);
    EntitySyncContext *sync=f->entities;int players=sync?sync->player_count:0;
    if(!add_actor(r,f,0,f->local,2047,players))return false;
    if(sync)for(int i=0;i<players;i++){int id=sync->player_indices[i];if(sync->players[id] && !add_actor(r,f,0,sync->players[id],id,players))return false;}
    if(f->offline_npc && !add_actor(r,f,1,f->offline_npc,0,players))return false;
    if(sync)for(int i=0;i<sync->npc_count;i++){int id=sync->npc_indices[i];if(sync->npcs[id] && !add_actor(r,f,1,sync->npcs[id],id,players))return false;}
    if(!add_effects(r,f))return false;
    SCENE_PERF225(PERF225_ACTORS);
    SceneTraversal225 *s=world3d_scene225(r->build);int top=f->top_level;
    if(top<0)top=scene_runtime225_top_level(r->world,f->level,f->eye_x,f->eye_z,f->local->base.x,f->local->base.z,f->pitch);
    if(top<0 || top>=4)return false;
    s->occlusion.pool=r->pools[top];s->occlusion.count=r->pool_counts[top];
    if(!world3d_prepare_scene225(s,&r->visibility,f->eye_x,f->eye_y,f->eye_z,f->pitch,f->yaw,top))return false;
    SCENE_PERF225(PERF225_PREPARE);
    memset(&r->result,0,sizeof(r->result));SceneRaster225 *out=&r->result;
    out->geometry=world3d_scene_geometry225(r->build);out->geometry_count=s->ground_count;
    out->resolve=world3d_scene_resolve225;out->resolve_user=r->build;
    out->view.pitch=f->pitch;out->view.yaw=f->yaw;out->view.low_memory=pix3d_low_detail;
    out->view.taking_input=f->taking_input;out->view.mouse_x=f->ground_mouse_x;out->view.mouse_y=f->ground_mouse_y;
    out->view.click_tile_x=f->click_x;out->view.click_tile_z=f->click_z;
    out->picking=(ScenePicking225){f->check_hover,f->mouse_x,f->mouse_y,r->hits,0,1000};
    s->emit=observer;s->emit_user=user;bool ok=world3d_render_scene225(s,-1,out);
    SCENE_PERF225(PERF225_RASTER);
    world3d_scene_clear_temporary225(r->build);return ok;
}
