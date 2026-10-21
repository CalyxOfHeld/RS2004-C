#include "dash3d/location_merge.h"
#include "gfx/anim.h"
#include "gfx/pix3d.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int rd(FILE *f){uint8_t b[4];assert(fread(b,1,4,f)==4);return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static Model *read_model(FILE *f,Arena *arena) {
    Model *m=arena_alloc_zero(arena,sizeof(*m));m->allocation_arena=arena;m->num_points=rd(f);m->num_faces=rd(f);m->num_t=rd(f);
    #define ARRAY(field,n) m->field=arena_alloc_zero(arena,(n)*sizeof(int32_t))
    ARRAY(point_x,m->num_points);ARRAY(point_y,m->num_points);ARRAY(point_z,m->num_points);
    ARRAY(face_vertex_a,m->num_faces);ARRAY(face_vertex_b,m->num_faces);ARRAY(face_vertex_c,m->num_faces);
    ARRAY(face_colour,m->num_faces);ARRAY(face_colour_a,m->num_faces);ARRAY(face_colour_b,m->num_faces);ARRAY(face_colour_c,m->num_faces);
    ARRAY(face_render_type,m->num_faces);ARRAY(face_priority,m->num_faces);ARRAY(face_alpha,m->num_faces);
    ARRAY(texture_a,m->num_t);ARRAY(texture_b,m->num_t);ARRAY(texture_c,m->num_t);
    #undef ARRAY
    for(int i=0;i<m->num_points;i++){m->point_x[i]=rd(f);m->point_y[i]=rd(f);m->point_z[i]=rd(f);}
    for(int i=0;i<m->num_faces;i++) {
        m->face_vertex_a[i]=rd(f);m->face_vertex_b[i]=rd(f);m->face_vertex_c[i]=rd(f);m->face_colour[i]=rd(f);
        m->face_colour_a[i]=rd(f);m->face_colour_b[i]=rd(f);m->face_colour_c[i]=rd(f);m->face_render_type[i]=rd(f);m->face_priority[i]=rd(f);m->face_alpha[i]=rd(f);
    }
    for(int i=0;i<m->num_t;i++){m->texture_a[i]=rd(f);m->texture_b[i]=rd(f);m->texture_c[i]=rd(f);}return m;
}
static void compare(Model *m,Model *want,int shape,int rotation,int tick) {
    if(m->num_points!=want->num_points || m->num_faces!=want->num_faces || m->num_t!=want->num_t){fprintf(stderr,"counts shape=%d rot=%d tick=%d actual=%d/%d/%d want=%d/%d/%d\n",shape,rotation,tick,m->num_points,m->num_faces,m->num_t,want->num_points,want->num_faces,want->num_t);abort();}
    for(int v=0;v<m->num_points;v++) if(m->point_x[v]!=want->point_x[v] || m->point_y[v]!=want->point_y[v] || m->point_z[v]!=want->point_z[v]) {fprintf(stderr,"vertex shape=%d rot=%d tick=%d v=%d actual=%d,%d,%d want=%d,%d,%d\n",shape,rotation,tick,v,m->point_x[v],m->point_y[v],m->point_z[v],want->point_x[v],want->point_y[v],want->point_z[v]);abort();}
    for(int i=0;i<m->num_faces;i++) {
        assert(m->face_vertex_a[i]==want->face_vertex_a[i] && m->face_vertex_b[i]==want->face_vertex_b[i] && m->face_vertex_c[i]==want->face_vertex_c[i]);
        assert((m->face_render_type?m->face_render_type[i]:0)==want->face_render_type[i]);assert((m->face_alpha?m->face_alpha[i]:0)==want->face_alpha[i]);
        if(want->face_render_type[i]&2)assert(m->face_colour[i]==want->face_colour[i]);
        assert((m->face_priority?m->face_priority[i]:m->priority)==want->face_priority[i]);
        bool flat=want->face_render_type[i]&1;
        if(m->face_colour_a[i]!=want->face_colour_a[i] || (!flat && (m->face_colour_b[i]!=want->face_colour_b[i] || m->face_colour_c[i]!=want->face_colour_c[i]))) {fprintf(stderr,"shade shape=%d rot=%d tick=%d face=%d\n",shape,rotation,tick,i);abort();}
    }
    for(int i=0;i<m->num_t;i++)assert(m->texture_a[i]==want->texture_a[i] && m->texture_b[i]==want->texture_b[i] && m->texture_c[i]==want->texture_c[i]);
}
static int present(World3D *scene,int layer) {
    for(size_t i=0;i<scene->map_location_count;i++){WorldMapLocation *loc=&scene->map_locations[i];if(loc->level==0 && loc->x==50 && loc->z==50 && world_location_layer(loc->shape)==layer)return loc->id;}return -1;
}
int main(void) {
    setbuf(stdout,NULL);Arena perm=arena_create(64*1024*1024),frame=arena_create(16*1024*1024);pix3d_init();
    Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&perm),*models=jagfile_load_file("../engine/data/pack/client/models",&perm);assert(config && models);model_init_repo(models,&perm);assert(anim_unpack(models,&perm));
    LocConfig locs;loc_unpack(&locs,config);World *world=calloc(1,sizeof(*world));World3D *scene=calloc(1,sizeof(*scene));EntitySyncContext *ctx=calloc(1,sizeof(*ctx));assert(world && scene && ctx);
    world_init(world,104,104);world3d_init(scene,104,104,4);entity_sync_init(ctx,NULL);ZoneSync zone={.entities=ctx,.local_pid=7};
    ctx->players[42]=calloc(1,sizeof(PlayerEntity));assert(ctx->players[42]);player_entity_init(ctx->players[42]);
    for(int l=0;l<4;l++)for(int x=0;x<105;x++)for(int z=0;z<105;z++)world->heightmap[l][x][z]=scene->heightmap[l][x][z]=-l*240-x*3+z*2;
    CollisionMap collision[4];FILE *f=fopen("tests/fixtures/merge-reference.bin","rb");assert(f && rd(f)==0x4d524731);Model *body=read_model(f,&perm);int cases=rd(f),frames=0;
    for(int n=0;n<cases;n++) {
        world3d_reset(scene);zone_sync_reset(&zone);loc_clear_model_cache(true);
        for(int l=0;l<4;l++)collision_map_init(&collision[l],104,104);
        int id=rd(f),shape=rd(f),rotation=rd(f),slot=rd(f),yaw=rd(f);
        assert(world_change_location(world,scene,&locs,collision,0,50,50,id,shape,rotation));ctx->cycle=1000;
        uint8_t packet[]={48,48,23,0x22,(uint8_t)(shape*4+rotation),(uint8_t)(id>>8),(uint8_t)id,0,5,0,15,0,(uint8_t)slot,3,254,255,4};
        assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,162,packet,sizeof(packet))==1 && zone.merge_count==2);
        PlayerEntity *p=slot==7?&ctx->local_player:ctx->players[42];
        int fields[]={p->loc_start_cycle,p->loc_stop_cycle,p->loc_offset_x,p->loc_offset_y,p->loc_offset_z,p->loc_min_tile_x,p->loc_max_tile_x,p->loc_min_tile_z,p->loc_max_tile_z};
        for(int i=0;i<9;i++)assert(fields[i]==rd(f));assert((p->loc_model!=NULL)==rd(f));
        int ticks=rd(f);
        for(int i=0;i<ticks;i++) {
            int tick=rd(f);p->base.x=rd(f);p->base.y=rd(f);p->base.z=rd(f);p->base.dst_yaw=yaw;
            assert(location_merge_tick(&zone,world,scene,&locs,collision,tick));assert(present(scene,world_location_layer(shape))==rd(f));assert(collision[0].flags[50][50]==rd(f));
            arena_reset(&frame);Model *expected=read_model(f,&frame);Model *model=player_with_location(body,p,tick,p->base.y,&frame);compare(model,expected,shape,rotation,tick);assert((p->loc_model!=NULL)==rd(f));frames++;
        }
        assert(zone.merge_count==0);
    }
    assert(fgetc(f)==EOF);fclose(f);
    printf("PASS: %d original zone23 local/remote packets, signed bounds, offsets, delayed scenery removal/restoration and collision states; %d complete original player/loc composites across all 23 shapes, rotations and eight facing values\n",cases,frames);
    // A malformed enclosed tail cannot replace a retained model or enqueue work.
    ctx->cycle=2000;uint8_t valid[]={48,48,23,0x22,40,0x0a,0x54,0,5,0,15,0,7,3,254,255,4};
    assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,162,valid,sizeof(valid))==1);Model *retained=ctx->local_player.loc_model;size_t count=zone.merge_count;
    uint8_t bad[22];memcpy(bad,valid,sizeof(valid));bad[17]=23;bad[18]=0;bad[19]=255;bad[20]=0;bad[21]=0;
    assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,162,bad,sizeof(bad))==-1 && ctx->local_player.loc_model==retained && zone.merge_count==count);
    valid[11]=0xff;valid[12]=0xff;assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,162,valid,sizeof(valid))==-1 && zone.merge_count==count);
    valid[11]=0;valid[12]=43;assert(zone_sync_apply(&zone,world,scene,&locs,collision,0,162,valid,sizeof(valid))==1 && zone.merge_count==count);
    zone_sync_reset(&zone);assert(zone.merge_count==0);player_with_location(body,&ctx->local_player,3000,0,&frame);assert(!ctx->local_player.loc_model);
    puts("PASS: malformed enclosed and invalid player-slot rejection before mutation; absent player ignored; region queue reset and owned-model expiry");
    entity_sync_cleanup(ctx);zone_sync_dispose(&zone);world3d_dispose(scene);free(scene);free(world);free(ctx);loc_free(&locs);arena_free(&frame);arena_free(&perm);return 0;
}
