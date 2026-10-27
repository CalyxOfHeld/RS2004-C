/* Real-cache construction inputs for the existing isolated scene comparator. */
#include "dash3d/world.h"
#include "dash3d/scene_raster225.h"
#include "dash3d/scene_build225.h"
#include "config/seq.h"
#include "gfx/anim.h"
#include "gfx/pix3d.h"
#include "gfx/pix8.h"
#include "core/jrandom.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "entity/npc_entity.h"
#include "entity/player_entity.h"
#include "entity/actor_model.h"
#include "dash3d/effects.h"
#include "dash3d/location_anim.h"
#include "dash3d/scene_runtime225.h"
#include "dash3d/camera.h"

typedef struct {
    Arena arena;World *world;World3D *scene;
    LocConfig locations;SeqConfig sequences;FloConfig floors;
    double brightness;
} NativeFixture225;

static Jagfile *archive(const char *dir,const char *name,Arena *arena){
    char path[2048];snprintf(path,sizeof(path),"%s/%s",dir,name);
    return jagfile_load_file(path,arena);
}
static uint8_t *raw_map(const char *dir,const char *name,size_t *length){
    char path[2048];snprintf(path,sizeof(path),"%s/%s",dir,name);
    FILE *f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
    long size=ftell(f);rewind(f);if(size<=0){fclose(f);return NULL;}
    uint8_t *data=malloc((size_t)size);
    if(!data || fread(data,1,(size_t)size,f)!=(size_t)size){free(data);fclose(f);return NULL;}
    fclose(f);*length=(size_t)size;return data;
}
void painter_native_close(void *fixture){
    NativeFixture225 *f=fixture;if(!f)return;
    if(f->scene){world3d_dispose(f->scene);free(f->scene);}
    free(f->world);flo_free(&f->floors);loc_free(&f->locations);
    seq_free(&f->sequences);arena_free(&f->arena);free(f);
}
void *painter_native_open(const char *cache,const char *maps,double requested_brightness){
    NativeFixture225 *f=calloc(1,sizeof(*f));if(!f)return NULL;
    JRandom random;jrandom_set_seed(&random,225);
    double brightness=requested_brightness+(jrandom_next_double(&random)*0.03-0.015);
    f->brightness=brightness;pix3d_init_colour_table(brightness);
    f->arena=arena_create(128*1024*1024);
    Jagfile *models=archive(cache,"models",&f->arena),*config=archive(cache,"config",&f->arena);
    Jagfile *textures=archive(cache,"textures",&f->arena);
    if(!models || !config || !textures){painter_native_close(f);return NULL;}
    model_init_repo(models,&f->arena);
    if(!anim_unpack(models,&f->arena)){painter_native_close(f);return NULL;}
    seq_unpack(&f->sequences,config);loc_unpack(&f->locations,config);
    math_random_seed(225);flo_unpack(&f->floors,config);
    for(int id=0;id<50;id++){
        char name[8];snprintf(name,sizeof(name),"%d",id);
        Pix8 *image=pix8_load(textures,name,0,NULL);if(!image){painter_native_close(f);return NULL;}
        pix3d_set_texture_palette_gamma(id,image->palette,image->palette_count,brightness);
        pix8_free(image);
    }
    f->world=calloc(1,sizeof(*f->world));f->scene=calloc(1,sizeof(*f->scene));
    if(!f->world || !f->scene){painter_native_close(f);return NULL;}
    world_init(f->world,104,104);f->world->build225=true;
    world3d_init(f->scene,104,104,4);f->scene->sequences=&f->sequences;
    size_t ground_length,loc_length;
    uint8_t *ground=raw_map(maps,"m50_50.raw",&ground_length),*locs=raw_map(maps,"l50_50.raw",&loc_length);
    if(!ground || !locs){free(ground);free(locs);painter_native_close(f);return NULL;}
    world_load_ground(f->world,ground,ground_length,20,20,3180,3180);
    math_random_seed(225);world_load_locations(f->world,locs,loc_length,&f->locations,f->scene,NULL,20,20);
    free(ground);free(locs);world_build(f->world,f->scene,&f->floors,NULL);
    return f;
}
const int32_t *painter_native_heights(void *fixture){
    NativeFixture225 *f=fixture;return f?&f->scene->heightmap[0][0][0]:NULL;
}
int painter_native_tile(void *fixture,int level,int x,int z,SceneGeometry225 *geometry){
    NativeFixture225 *f=fixture;if(!f || !geometry || level<0 || level>=4 || x<0 || x>=104 || z<0 || z>=104)return -1;
    GroundTile *tile=&f->scene->tiles[level][x][z];
    geometry->underlay=tile->has_underlay?&tile->underlay:NULL;
    geometry->overlay=tile->has_overlay?&tile->overlay:NULL;
    return tile->draw_level;
}
World *painter_native_world(void *fixture){NativeFixture225 *f=fixture;return f?f->world:NULL;}
World3D *painter_native_scene(void *fixture){NativeFixture225 *f=fixture;return f?f->scene:NULL;}
double painter_native_brightness(void *fixture){NativeFixture225 *f=fixture;return f?f->brightness:0.0;}
SceneBuild225 *painter_native_construct(void *fixture){
    NativeFixture225 *f=fixture;return f?world3d_construct225(f->world,f->scene,&f->locations):NULL;
}
int painter_native_compare_asset(const SceneModel225 *actual,const SceneModel225 *expected){
    if(!actual || !expected)return 1;
    const Model *a=actual->model,*b=expected->model;
    if(a->num_points!=b->num_points || a->num_faces!=b->num_faces || a->num_t!=b->num_t)return 1;
    for(int i=0;i<a->num_points;i++)if(a->point_x[i]!=b->point_x[i] || a->point_y[i]!=b->point_y[i] || a->point_z[i]!=b->point_z[i])return 2;
    for(int i=0;i<a->num_faces;i++){
        if(a->face_vertex_a[i]!=b->face_vertex_a[i] || a->face_vertex_b[i]!=b->face_vertex_b[i] || a->face_vertex_c[i]!=b->face_vertex_c[i])return 2;
        int ta=a->face_render_type?a->face_render_type[i]:0,tb=b->face_render_type?b->face_render_type[i]:0;
        if(ta==255)ta=-1;
        if(tb==255)tb=-1;
        if(ta!=tb)return 4;
        if(ta==-1)continue;
        if(a->face_colour_a[i]!=b->face_colour_a[i])return 4;
        if(!(ta&1) && (a->face_colour_b[i]!=b->face_colour_b[i] || a->face_colour_c[i]!=b->face_colour_c[i]))return 4;
        if((ta&3)>=2 && a->face_colour[i]!=b->face_colour[i])return 8;
        int pa=a->face_priority?a->face_priority[i]:a->priority,pb=b->face_priority?b->face_priority[i]:b->priority;
        int aa=a->face_alpha?a->face_alpha[i]:0,ab=b->face_alpha?b->face_alpha[i]:0;
        if(pa!=pb || aa!=ab)return 16;
    }
    for(int i=0;i<a->num_t;i++)if(a->texture_a[i]!=b->texture_a[i] || a->texture_b[i]!=b->texture_b[i] || a->texture_c[i]!=b->texture_c[i])return 8;
    if(actual->bounds.radius!=expected->bounds.radius || actual->bounds.max_y!=expected->bounds.max_y ||
       actual->bounds.min_depth!=expected->bounds.min_depth || actual->bounds.max_depth!=expected->bounds.max_depth)return 32;
    return actual->pickable==expected->pickable?0:64;
}

/* Scenario inputs only. Every model comes from the existing native builders. */
typedef struct {
    NativeFixture225 *native;SceneBuild225 *build;
    Arena frame,retained;NpcConfig npcs;IdkConfig identities;ObjConfig objects;SpotAnimConfig spots;
    NpcEntity npc;PlayerEntity player;SceneModel225 held,npc_model,player_model;
    int calls[3];
    bool picking_layout,reverse;int32_t direct_bitset;
} DynamicFixture225;
static SceneModel225 actor_asset(Model *model){
    SceneModel225 out={0};out.model=model;if(!model)return out;
    int radius2=0,min_y=0;
    for(int i=0;i<model->num_points;i++){
        int x=model->point_x[i],y=model->point_y[i],z=model->point_z[i];
        int square=x*x+z*z;if(square>radius2)radius2=square;
        if(-y>out.bounds.max_y)out.bounds.max_y=-y;
        if(y>min_y)min_y=y;
    }
    int radius=out.bounds.radius=(int)(sqrt((double)radius2)+0.99),height=out.bounds.max_y;
    out.bounds.min_depth=(int)(sqrt((double)(radius*radius+height*height))+0.99);
    out.bounds.max_depth=out.bounds.min_depth+(int)(sqrt((double)(radius*radius+min_y*min_y))+0.99);return out;
}
void painter_dynamic_close(void *context){
    DynamicFixture225 *d=context;if(!d)return;
    world3d_scene_clear_temporary225(d->build);npc_entity_free(&d->npc);player_entity_dispose(&d->player);
    actor_models_clear();spotanim_free(&d->spots);npc_free(&d->npcs);obj_free(&d->objects);idk_free(&d->identities);
    arena_free(&d->frame);arena_free(&d->retained);free(d);
}
void *painter_dynamic_open(void *native,SceneBuild225 *build,const char *cache,const int32_t selection[5]){
    NativeFixture225 *f=native;if(!f || !build)return NULL;
    DynamicFixture225 *d=calloc(1,sizeof(*d));if(!d)return NULL;d->native=f;d->build=build;
    d->frame=arena_create(16*1024*1024);d->retained=arena_create(4*1024*1024);
    npc_entity_init(&d->npc);player_entity_init(&d->player);
    Jagfile *config=archive(cache,"config",&f->arena);if(!config){painter_dynamic_close(d);return NULL;}
    npc_unpack(&d->npcs,config);obj_unpack(&d->objects,config,true);idk_unpack(&d->identities,config,&f->arena);
    if(!spotanim_unpack(&d->spots,config,&f->sequences) || !npc_get(&d->npcs,0,&d->npc.type)){painter_dynamic_close(d);return NULL;}
    d->npc.has_type=true;d->npc.base.seq_stand_id=d->npc.base.secondary_seq_id=d->npc.type.readyanim;
    d->held=actor_asset(npc_entity_get_animated_model(&d->npc,&f->sequences,&d->retained));
    d->held.pickable=d->npc.type.size==1;d->direct_bitset=0x20000002;
    size_t kept=0;
    for(size_t i=0;i<f->scene->location_animation_count;i++){
        LocationAnimation a=f->scene->location_animations[i];
        if(a.level==selection[0] && a.x==selection[1] && a.z==selection[2] && a.layer==selection[3] && a.id==selection[4])f->scene->location_animations[kept++]=a;
    }
    f->scene->location_animation_count=kept;
    if(kept!=1 || !d->held.model){painter_dynamic_close(d);return NULL;}return d;
}
static const SceneModel225 *dynamic_npc(void *context){
    DynamicFixture225 *d=context;d->calls[0]++;
    Model *body=npc_entity_get_animated_model(&d->npc,&d->native->sequences,&d->frame);
    d->npc_model=actor_asset(actor_with_spotanim(body,&d->npc.base,&d->spots,&d->frame));
    d->npc_model.pickable=d->npc.type.size==1;return &d->npc_model;
}
static const SceneModel225 *dynamic_player(void *context){
    DynamicFixture225 *d=context;d->calls[1]++;
    Model *body=player_entity_get_model(&d->player,&d->identities,&d->objects,&d->native->sequences,&d->frame);
    d->player_model=actor_asset(actor_with_spotanim(body,&d->player.base,&d->spots,&d->frame));
    d->player_model.pickable=true;return &d->player_model;
}
void painter_dynamic_pick_layout(void *context,int32_t direct_bitset,bool reverse){
    DynamicFixture225 *d=context;d->picking_layout=true;d->direct_bitset=direct_bitset;d->held.pickable=false;d->reverse=reverse;
}
static const SceneModel225 *dynamic_unused(void *context){DynamicFixture225 *d=context;d->calls[2]++;return &d->held;}
static void actor_selection(PathingEntity *a,const int32_t *s){
    a->seq_stand_id=s[0];a->primary_seq_id=s[1];a->primary_seq_frame=s[2];a->primary_seq_delay=s[3];
    a->secondary_seq_id=s[4];a->secondary_seq_frame=s[5];
}
bool painter_dynamic_frame(void *context,const int32_t input[16],uint8_t *appearance,int32_t length){
    DynamicFixture225 *d=context;if(!d || !appearance || length<=0)return false;
    world3d_scene_clear_temporary225(d->build);arena_reset(&d->frame);memset(d->calls,0,sizeof(d->calls));
    if(input[1])actor_models_clear();
    location_anim_tick(d->native->scene,&d->native->locations,input[0]);
    if(!world3d_scene_refresh225(d->build))return false;
    Packet packet=packet_wrap(appearance,(size_t)length);player_entity_read_appearance(&d->player,&packet);if(packet.error)return false;
    actor_selection(&d->npc.base,input+4);actor_selection(&d->player.base,input+10);
    d->npc.base.spotanim_id=d->player.base.spotanim_id=input[3];
    d->npc.base.spotanim_frame=d->player.base.spotanim_frame=0;d->npc.base.spotanim_height=d->player.base.spotanim_height=13;
    for(int slot=0;slot<3;slot++){
        int actor=d->reverse?2-slot:slot;
        int tx=d->picking_layout?42:42+actor,tz=!d->picking_layout && actor==1?36:35;
        int x=tx*128+64,z=tz*128+64,y=d->native->scene->heightmap[0][tx][tz];
        int bitset=actor==2?d->direct_bitset:0x20000000+actor;
        if(!world3d_scene_temporary225(d->build,0,x,y,z,60,input[2],true,bitset,actor==2?&d->held:NULL,
            actor==0?dynamic_npc:actor==1?dynamic_player:dynamic_unused,d))return false;
    }
    return true;
}
bool painter_dynamic_preview(void *context){
    DynamicFixture225 *d=context;return d && model_render_simple225(d->held.model,128,256,0,0,0,20,500);
}
bool painter_runtime_preview(void *context){
    DynamicFixture225 *d=context;if(!d)return false;model_use_preview225(true);
    model_render_obj(d->held.model,128,256,0,0,0,20,500);model_use_preview225(false);return true;
}
void painter_dynamic_stats(void *context,int32_t stats[5]){
    DynamicFixture225 *d=context;for(int i=0;i<3;i++)stats[i]=d->calls[i];stats[3]=d->npc.base.height;stats[4]=d->player.base.height;
}
void *painter_runtime_open(void *context){
    DynamicFixture225 *d=context;return scene_runtime225_create(d->native->world,d->native->scene,&d->native->locations,
        &d->native->sequences,&d->identities,&d->objects,&d->spots);
}
SceneBuild225 *painter_dynamic_build(void *context){DynamicFixture225 *d=context;return d->build;}
bool painter_runtime_rebuild(void *context,const char *maps){
    DynamicFixture225 *d=context;NativeFixture225 *f=d->native;
    world3d_scene_clear_temporary225(d->build);world3d_reset(f->scene);world_reset(f->world);
    f->world->random_hue_offset=f->world->random_lightness_offset=0;
    size_t ground_length,loc_length;
    uint8_t *ground=raw_map(maps,"m50_50.raw",&ground_length),*locs=raw_map(maps,"l50_50.raw",&loc_length);
    if(!ground || !locs){free(ground);free(locs);return false;}
    world_load_ground(f->world,ground,ground_length,20,20,3180,3180);math_random_seed(225);
    world_load_locations(f->world,locs,loc_length,&f->locations,f->scene,NULL,20,20);
    free(ground);free(locs);world_build(f->world,f->scene,&f->floors,NULL);
    size_t kept=0;for(size_t i=0;i<f->scene->location_animation_count;i++){
        LocationAnimation a=f->scene->location_animations[i];if(a.level==0 && a.x==32 && a.z==39 && a.id==196)f->scene->location_animations[kept++]=a;
    }
    f->scene->location_animation_count=kept;
    world3d_destroy_scene225(d->build);d->build=world3d_construct225(f->world,f->scene,&f->locations);return kept==1 && d->build;
}
bool painter_runtime_frame(void *runtime,void *context,const int32_t camera[15],const int32_t input[10],
    int32_t yaw,SceneEmit225 observer,void *user){
    DynamicFixture225 *d=context;World3D *scene=d->native->scene;int cycle=100+input[9],spot=input[8];
    if(input[9]==3 && !world_change_location(d->native->world,scene,&d->native->locations,NULL,0,41,30,-1,10,3))return false;
    if(input[9]==4 && !world_change_location(d->native->world,scene,&d->native->locations,NULL,0,41,30,879,10,3))return false;
    d->player.base.x=42*128+64;d->player.base.z=35*128+64;d->player.base.yaw=yaw;
    d->player.loc_model=input[9]==1?d->held.model:NULL;
    if(d->player.loc_model){
        d->player.loc_start_cycle=100;d->player.loc_stop_cycle=104;
        d->player.loc_offset_x=d->player.base.x+64;d->player.loc_offset_z=d->player.base.z+64;
        d->player.loc_offset_y=scene->heightmap[0][42][35];
        d->player.loc_min_tile_x=42;d->player.loc_max_tile_x=43;d->player.loc_min_tile_z=34;d->player.loc_max_tile_z=35;
    }
    d->npc.base.x=44*128+64;d->npc.base.z=35*128+64;d->npc.base.yaw=yaw;
    GroundTile *tile=&scene->tiles[0][43][36];ground_items_clear(tile);
    if(!ground_items_event(tile,&d->objects,223,995,25000,0) || !ground_items_event(tile,&d->objects,223,1277,1,0) ||
       !ground_items_event(tile,&d->objects,223,379,1,0))return false;
    SceneEffect effect[2]={{0}};
    effect[0]=(SceneEffect){.projectile=true,.id=spot,.level=0,.src_x=41*128+64,.src_z=36*128+64,
        .src_y=scene->heightmap[0][41][36]-80,.start_cycle=cycle,.last_cycle=cycle+30,.peak_pitch=28};
    projectile_velocity(&effect[0],scene->heightmap[0][45][38]-100,38*128+64,45*128+64,cycle);effect_advance(&effect[0],&d->spots,1);
    effect[1]=(SceneEffect){.id=spot,.level=0,.x=45*128+64,.z=37*128+64,.y=scene->heightmap[0][45][37],.start_cycle=cycle};
    effect_advance(&effect[1],&d->spots,1);
    SceneEffects effects={.items=effect,.count=2,.config=&d->spots,.cycle=cycle};
    SceneFrame225 f={0};f.local=&d->player;f.offline_npc=&d->npc;f.effects=&effects;f.cycle=cycle;
    f.eye_x=camera[4];f.eye_y=camera[7];f.eye_z=camera[8];f.pitch=camera[6];f.yaw=camera[3];f.top_level=-1;
    f.check_hover=input[0]!=0;f.mouse_x=input[1];f.mouse_y=input[2];f.taking_input=input[3]!=0;
    f.ground_mouse_x=input[4];f.ground_mouse_y=input[5];f.click_x=input[6];f.click_z=input[7];
    return scene_runtime225_frame(runtime,&f,observer,user);
}
