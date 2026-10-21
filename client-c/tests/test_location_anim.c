#include "dash3d/location_anim.h"
#include "net/zone_sync.h"
#include "gfx/anim.h"
#include "gfx/pix3d.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int rd(FILE *f) {uint8_t b[4];assert(fread(b,1,4,f)==4);return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static void mesh(FILE *f,Model *m,int id,int shape,int rotation) {
    int vertices=rd(f),faces=rd(f),light=rd(f);assert(m && vertices==m->num_points && faces==m->num_faces);
    for(int i=0;i<vertices;i++) {
        int x=rd(f),y=rd(f),z=rd(f);
        if(x!=m->point_x[i] || y!=m->point_y[i] || z!=m->point_z[i]) {
            fprintf(stderr,"loc id=%d shape=%d rot=%d vertex=%d expected=%d,%d,%d got=%d,%d,%d\n",id,shape,rotation,i,x,y,z,m->point_x[i],m->point_y[i],m->point_z[i]);abort();
        }
    }
    for(int i=0;i<faces;i++) {
        int alpha=rd(f);if(alpha!=(m->face_alpha?m->face_alpha[i]:0)) {
            fprintf(stderr,"loc alpha id=%d shape=%d rot=%d face=%d expected=%d got=%d\n",id,shape,rotation,i,alpha,m->face_alpha?m->face_alpha[i]:0);abort();
        }
        if(light) {
            int a=rd(f),b=rd(f),c=rd(f);
            // Flat faces only use A in the original renderer.
            if(m->face_render_type && (m->face_render_type[i]&1)) {b=m->face_colour_b[i];c=m->face_colour_c[i];}
            if(a!=m->face_colour_a[i] || b!=m->face_colour_b[i] || c!=m->face_colour_c[i]) {
                fprintf(stderr,"loc lighting id=%d shape=%d rot=%d face=%d expected=%d,%d,%d got=%d,%d,%d\n",id,shape,rotation,i,a,b,c,m->face_colour_a[i],m->face_colour_b[i],m->face_colour_c[i]);abort();
            }
        }
    }
}
int main(void) {
    setbuf(stdout,NULL);Arena perm=arena_create(64*1024*1024);pix3d_init();
    Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&perm),*models=jagfile_load_file("../engine/data/pack/client/models",&perm);assert(config && models);
    model_init_repo(models,&perm);assert(anim_unpack(models,&perm));SeqConfig seq;seq_unpack(&seq,config);LocConfig locs;loc_unpack(&locs,config);
    FILE *f=fopen("tests/fixtures/location-reference.bin","rb");assert(f && rd(f)==0x4c4f4331);int meshes=0;
    for(int id;(id=rd(f))!=-1;meshes++) {
        int shape=rd(f),rotation=rd(f),frame=rd(f),sw=rd(f),se=rd(f),ne=rd(f),nw=rd(f);
        LocType loc;assert(loc_get(&locs,id,&loc));Model *m=loc_get_model_frame(&loc,shape,rotation,frame,NULL);loc_skew_model(m,&loc,sw,se,ne,nw);
        if(id==521 && rotation==0)printf("[ALPHA] loc521 frame=%d alpha=%d flag=%d\n",frame,m->face_alpha?m->face_alpha[0]:0,loc.anim_has_alpha);
        mesh(f,m,id,shape,rotation);model_free_unshared(m);loc_free_entry(&loc);
    }
    printf("PASS: %d original animated scenery meshes (mirroring, frame transforms, rotations, colour, scale, slope, lighting and alpha)\n",meshes);
    loc_clear_model_cache(true);
    int alpha_id=rd(f),first_frame=rd(f),alpha_steps=rd(f);LocType alpha;assert(loc_get(&locs,alpha_id,&alpha));
    Model *held=loc_get_model_frame(&alpha,10,0,first_frame,NULL);assert(held);
    for(int step=0;step<alpha_steps;step++) {
        int clear=rd(f),frame=rd(f),rotation=rd(f);if(clear)loc_clear_model_cache(clear==2);
        Model *next=loc_get_model_frame(&alpha,10,rotation,frame,NULL);mesh(f,next,alpha_id,10,rotation);mesh(f,held,alpha_id,10,0);model_free_unshared(next);
    }
    loc_clear_model_cache(true);model_free_unshared(held);loc_free_entry(&alpha);
    printf("PASS: %d original shared-alpha cache transitions, retained models and static/full cache clearing\n",alpha_steps);
    int sequences=rd(f);assert(sequences==seq.count);
    for(int i=0;i<sequences;i++) {
        int id=rd(f),steps=rd(f);LocationAnimation a={.frame=-1};bool active=true;
        for(int step=0;step<steps;step++) {
            int delta=rd(f),frame=rd(f),cycle=rd(f),want=rd(f);bool changed;
            if(active)active=location_anim_advance(&a,&seq.instances[id],delta,&changed);
            if(a.frame!=frame || a.cycle!=cycle || active!=want) {fprintf(stderr,"sequence=%d step=%d delta=%d actual=%d,%d,%d expected=%d,%d,%d\n",id,step,delta,a.frame,a.cycle,active,frame,cycle,want);abort();}
        }
    }
    printf("PASS: all %d original sequence lifecycles through original zone42/pushLocs, including loops, expiry, zero and batched ticks\n",sequences);
    loc_clear_model_cache(true);
    World *world=calloc(1,sizeof(*world));World3D *scene=calloc(1,sizeof(*scene));assert(world && scene);ZoneSync zone={0};
    world_init(world,104,104);world3d_init(scene,104,104,4);int cases=rd(f),sample_id=-1,sample_shape=-1,sample_seq=-1;
    for(int n=0;n<cases;n++) {
        world3d_reset(scene);scene->sequences=NULL;
        int id=rd(f),shape=rd(f),rotation=rd(f),sequence=rd(f),parts=rd(f);
        assert(world_change_location(world,scene,&locs,NULL,0,50,50,id,shape,rotation));scene->sequences=&seq;
        scene->heightmap[0][50][50]=-123;scene->heightmap[0][51][50]=-76;scene->heightmap[0][51][51]=-40;scene->heightmap[0][50][51]=-91;
        uint8_t packet[]={48,48,42,0x22,(uint8_t)(shape*4+3),(uint8_t)(sequence>>8),(uint8_t)sequence};
        assert(zone_sync_apply(&zone,world,scene,&locs,NULL,0,162,packet,sizeof(packet))==1);
        assert(scene->location_animation_count==1);location_anim_tick(scene,&locs,1);
        GroundTile *tile=&scene->tiles[0][50][50];assert(tile->loc_count==parts);
        for(int part=0;part<parts;part++)mesh(f,tile->locs[part].model,id,shape,rotation);
        if(world_location_layer(shape)==1)assert(tile->locs[0].yaw==rotation*512);
        sample_id=id;sample_shape=shape;sample_seq=sequence;
    }
    assert(fgetc(f)==EOF);fclose(f);printf("PASS: %d complete original zone42 scene model replacements across all 23 map shapes and four rotations\n",cases);
    // Rejection is atomic across the enclosed packet, including its base.
    size_t count=scene->location_animation_count;int base=zone.base_x;
    uint8_t malformed[]={1,2,42,0x22,(uint8_t)(sample_shape*4),0,0,42,0x22,255,0,0};
    assert(zone_sync_apply(&zone,world,scene,&locs,NULL,0,162,malformed,sizeof(malformed))==-1 && scene->location_animation_count==count && zone.base_x==base);
    malformed[9]=sample_shape*4;malformed[10]=255;malformed[11]=255;
    assert(zone_sync_apply(&zone,world,scene,&locs,NULL,0,162,malformed,sizeof(malformed))==-1 && scene->location_animation_count==count);
    // Deletion removes animation on its next frame update; rebuild clears it immediately.
    assert(world_change_location(world,scene,&locs,NULL,0,50,50,-1,sample_shape,0));location_anim_tick(scene,&locs,10000);assert(scene->location_animation_count==0);
    assert(location_anim_add(scene,sample_id,0,50,50,world_location_layer(sample_shape),sample_seq,false));world3d_reset(scene);assert(scene->location_animation_count==0 && scene->sequences==&seq);
    // Built-in animation starts from cache without a zone42 packet.
    int automatic=0;
    for(int id=0;id<locs.count;id++) {LocType loc;assert(loc_get(&locs,id,&loc));if(loc.anim>=0 && loc.model_count>0) {
        int shape=loc.shapes[0];assert(world_change_location(world,scene,&locs,NULL,0,50,50,id,shape,0));assert(scene->location_animation_count==1);
        LocationAnimation *a=scene->location_animations;const SeqType *s=&seq.instances[loc.anim];
        assert(a->sequence==loc.anim && ((s->replayoff==-1 && a->frame==-1) || (s->replayoff!=-1 && a->frame>=0 && a->frame<s->frame_count)));
        location_anim_tick(scene,&locs,1);world3d_reset(scene);automatic++;
    }loc_free_entry(&loc);}
    printf("PASS: %d cache-driven automatic scenery animations; malformed enclosed rejection before mutation; removal and rebuild cleanup\n",automatic);
    zone_sync_dispose(&zone);world3d_dispose(scene);free(scene);free(world);loc_free(&locs);seq_free(&seq);arena_free(&perm);return 0;
}
