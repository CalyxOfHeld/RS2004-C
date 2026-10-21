#include "dash3d/effects.h"
#include "gfx/anim.h"
#include "gfx/pix3d.h"
#include "gfx/pix2d.h"
#include "ui/combat_feedback.h"
#include "net/zone_sync.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static uint32_t rd(FILE *f){uint8_t b[4];assert(fread(b,1,4,f)==4);return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static double dbl(FILE *f){uint64_t bits=(uint64_t)rd(f)<<32;bits|=rd(f);double d;memcpy(&d,&bits,8);return d;}
static void close_double(FILE *f,double actual){double want=dbl(f);if(fabs(actual-want)>1e-9*(1+fabs(want))){fprintf(stderr,"trajectory %.17g != %.17g\n",actual,want);abort();}}
int main(void) {
    setbuf(stdout,NULL);Arena perm=arena_create(64*1024*1024),frame=arena_create(8*1024*1024);
    Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&perm),*models=jagfile_load_file("../engine/data/pack/client/models",&perm);assert(config && models);
    model_init_repo(models,&perm);assert(anim_unpack(models,&perm));pix3d_init();
    SeqConfig seq;seq_unpack(&seq,config);SpotAnimConfig spots;assert(spotanim_unpack(&spots,config,&seq));
    FILE *f=fopen("tests/fixtures/effects-reference.bin","rb");assert(f);assert(rd(f)==0x45464631);assert((int)rd(f)==spots.count);
    for(int id=0;id<spots.count;id++) {
        SpotAnimType *s=&spots.instances[id];int fields[]={s->model,s->anim,s->resizeh,s->resizev,s->orientation,s->ambient,s->contrast,s->anim_has_alpha};
        for(int i=0;i<8;i++)assert((int32_t)rd(f)==fields[i]);
        for(int i=0;i<6;i++)assert((int)rd(f)==s->recol_s[i]);
        for(int i=0;i<6;i++)assert((int)rd(f)==s->recol_d[i]);
        for(int variant=0;variant<3;variant++) {
            arena_reset(&frame);int n=rd(f);Model *m=spotanim_model(&spots,id,n,variant==1?300:0,0,variant==2,&frame);assert(m);
            int points=rd(f),faces=rd(f);
            if(points!=m->num_points || faces!=m->num_faces){fprintf(stderr,"mesh counts id=%d variant=%d expected=%d/%d got=%d/%d\n",id,variant,points,faces,m->num_points,m->num_faces);abort();}
            for(int i=0;i<m->num_points;i++) {
                int x=(int32_t)rd(f),y=(int32_t)rd(f),z=(int32_t)rd(f);
                if(x!=m->point_x[i] || y!=m->point_y[i] || z!=m->point_z[i]) {fprintf(stderr,"mesh id=%d variant=%d vertex=%d\n",id,variant,i);abort();}
            }
            for(int i=0;i<m->num_faces;i++) {
                int a=(int32_t)rd(f),b=(int32_t)rd(f),c=(int32_t)rd(f),alpha=rd(f);
                // Only A is read for original flat-shaded faces. Native fills
                // unused B/C with A; compare all rendered values and alpha.
                if(m->face_render_type && (m->face_render_type[i]&1)){b=m->face_colour_b[i];c=m->face_colour_c[i];}
                if(a!=m->face_colour_a[i] || b!=m->face_colour_b[i] || c!=m->face_colour_c[i] || alpha!=(m->face_alpha?m->face_alpha[i]:0)) {
                    fprintf(stderr,"lit mesh id=%d variant=%d face=%d expected=%d,%d,%d,%d got=%d,%d,%d,%d\n",id,variant,i,a,b,c,alpha,m->face_colour_a[i],m->face_colour_b[i],m->face_colour_c[i],m->face_alpha?m->face_alpha[i]:0);abort();
                }
            }
        }
    }
    int cases=rd(f);assert(cases==8);
    for(int n=0;n<cases;n++) {
        SceneEffect p={.projectile=true};p.id=rd(f);p.start_cycle=rd(f);p.last_cycle=rd(f);p.src_x=rd(f);p.src_z=rd(f);p.src_y=(int32_t)rd(f);p.peak_pitch=rd(f);p.arc=rd(f);
        int ticks=rd(f);
        for(int tick=0;tick<ticks;tick++) {
            int x=rd(f),z=rd(f),y=(int32_t)rd(f);projectile_velocity(&p,y,z,x,p.start_cycle+tick);effect_advance(&p,&spots,1);
            close_double(f,p.x);close_double(f,p.y);close_double(f,p.z);close_double(f,p.vx);close_double(f,p.vz);close_double(f,p.velocity);close_double(f,p.vy);close_double(f,p.ay);
            assert((int)rd(f)==p.yaw && (int)rd(f)==p.pitch && (int)rd(f)==p.frame && (int)rd(f)==p.frame_cycle);
        }
    }
    int animated=rd(f);assert(animated>0);
    for(int n=0;n<animated;n++) {
        int id=rd(f),ticks=rd(f);SceneEffect fixed={.id=id};
        PathingEntity actor;pathing_entity_init(&actor);actor.spotanim_id=id;actor.spotanim_delay=5;actor.spotanim_frame=-1;
        for(int tick=0;tick<ticks;tick++) {
            if(!fixed.complete)effect_advance(&fixed,&spots,1);
            actor_spotanim_tick(&actor,&spots,tick);
            assert((int)rd(f)==fixed.frame && (int)rd(f)==fixed.frame_cycle && (int)rd(f)==fixed.complete);
            assert((int32_t)rd(f)==actor.spotanim_id && (int32_t)rd(f)==actor.spotanim_frame && (int)rd(f)==actor.spotanim_cycle);
        }
    }
    assert(fgetc(f)==EOF);fclose(f);
    printf("PASS: all %d animated definitions match original stationary and delayed actor frame cycles through completion\n",animated);
    printf("PASS: %d real spotanim definitions and %d original animated/lit meshes including pitch/orientation/alpha; eight homing trajectories\n",spots.count,spots.count*3);
    Jagfile *media_jag=jagfile_load_file("../engine/data/pack/client/media",&perm),*title=jagfile_load_file("../engine/data/pack/client/title",&perm);assert(media_jag && title);
    Media media;assert(media_load(&media,media_jag,&perm));PixFont font;pixfont_init_lookup();assert(pixfont_load(&font,title,"p11",&perm));
    size_t pixel_count=789*532;uint32_t *pixels=calloc(pixel_count,4),*expected=calloc(pixel_count,4);assert(pixels && expected);pix2d_bind(pixels,789,532);
    f=fopen("tests/fixtures/combat-reference.bin","rb");assert(f && rd(f)==0x48495431);int frames=rd(f);
    for(int n=0;n<frames;n++) {
        int bx=rd(f),by=rd(f),hx=rd(f),hy=rd(f);PathingEntity e={0};e.combat_cycle=rd(f);e.health=rd(f);e.total_health=rd(f);e.damage=rd(f);e.damage_type=rd(f);
        int count=rd(f);memset(pixels,0,pixel_count*4);memset(expected,0,pixel_count*4);
        for(int i=0;i<count;i++){unsigned at=rd(f);assert(at<pixel_count);expected[at]=rd(f);}
        combat_feedback_draw(&e,100,&media,&font,bx,by,hx,hy);
        assert(!memcmp(pixels,expected,pixel_count*4));
        assert(pix2d_clip_left==0 && pix2d_clip_right==789 && pix2d_clip_top==0 && pix2d_clip_bottom==532);
    }
    assert(fgetc(f)==EOF);fclose(f);printf("PASS: %d actual original combat overlay framebuffers and expiry boundaries\n",frames);
    SceneEffect invalid={.x=NAN,.id=91};SceneEffects invalid_fx={.items=&invalid,.count=1,.config=&spots};
    effects_draw(&invalid_fx,&frame,0,0,0,0,256,0);
    invalid.x=0;invalid.y=1e100;effects_draw(&invalid_fx,&frame,0,0,0,0,256,0);
    // Production entity mask receives the original hit timer and delayed effect.
    EntitySyncContext *ctx=calloc(1,sizeof(*ctx));assert(ctx);entity_sync_init(ctx,NULL);ctx->cycle=123;
    uint8_t info[]={0x80,0,0,0x1f,0xfc,0x11,0x10,7,1,2,10,0,0,0,12,0,3};
    // Build minimal local update in the bit format used by PLAYER_INFO.
    memset(info,0,sizeof(info));int bit=0;
    #define BITS(v,n) do {unsigned val=(v);for(int j=(n)-1;j>=0;j--,bit++)if((val>>j)&1)info[bit/8]|=(uint8_t)(1u<<(7-(bit&7)));} while(0)
    BITS(1,1);BITS(0,2);BITS(0,8);BITS(2047,11);
    int pos=(bit+7)/8;info[pos++]=0x90;info[pos++]=1;info[pos++]=7;info[pos++]=1;info[pos++]=2;info[pos++]=10;info[pos++]=0;info[pos++]=0;info[pos++]=0;info[pos++]=12;info[pos++]=0;info[pos++]=3;
    Packet packet=packet_wrap(info,pos);assert(entity_sync_player_info(ctx,&packet,pos));
    assert(ctx->local_player.base.combat_cycle==523 && ctx->local_player.base.damage==7);
    assert(ctx->local_player.base.spotanim_delay==126 && ctx->local_player.base.spotanim_frame==-1 && ctx->local_player.base.spotanim_height==12);
    actor_spotanim_tick(&ctx->local_player.base,&spots,125);assert(ctx->local_player.base.spotanim_frame==-1);
    actor_spotanim_tick(&ctx->local_player.base,&spots,126);assert(ctx->local_player.base.spotanim_frame==0);
    World *world=calloc(1,sizeof(*world));World3D *scene=calloc(1,sizeof(*scene));assert(world && scene);
    world_init(world,104,104);world3d_init(scene,104,104,4);LocConfig locs;loc_unpack(&locs,config);
    SceneEffects effects={.config=&spots,.cycle=100};ZoneSync zone={.effects=&effects};
    uint8_t enclosed[]={48,48,69,0x22,1,0,0,1,0,91,50,30,0,2,0,8,16,10,191,0x22,0,92,10,0,3};
    assert(zone_sync_apply(&zone,world,scene,&locs,NULL,0,162,enclosed,sizeof(enclosed)-1)==-1 && effects.count==0);
    assert(zone_sync_apply(&zone,world,scene,&locs,NULL,0,162,enclosed,sizeof(enclosed))==1 && effects.count==2);
    assert(effects.items[0].target==1 && effects.items[0].start_cycle==102 && effects.items[0].last_cycle==108);
    assert(effects.items[1].start_cycle==103 && !effects.items[1].projectile);
    effects_tick(&effects,world,scene,ctx,101);assert(!effects.items[0].mobile);
    effects_tick(&effects,world,scene,ctx,102);assert(effects.items[0].mobile);
    effects_tick(&effects,world,scene,ctx,109);assert(effects.count==1 && !effects.items[0].projectile);
    ctx->current_plane=1;effects_tick(&effects,world,scene,ctx,110);assert(effects.count==0);
    enclosed[8]=255;enclosed[9]=255;
    assert(zone_sync_apply(&zone,world,scene,&locs,NULL,0,162,enclosed,sizeof(enclosed))==-1 && effects.count==0);
    effects_dispose(&effects);zone_sync_dispose(&zone);loc_free(&locs);world3d_dispose(scene);free(scene);free(world);
    entity_sync_cleanup(ctx);free(ctx);
    free(pixels);free(expected);spotanim_free(&spots);seq_free(&seq);arena_free(&frame);arena_free(&perm);
    puts("PASS: production hit/actor-effect timers; enclosed projectile/static packets, atomic malformed rejection, delays/expiry/plane clear");return 0;
}
