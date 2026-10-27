#include "dash3d/camera.h"
#include "dash3d/scene_runtime225.h"
#include "core/jrandom.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int ca_int(FILE *f){uint8_t b[4];assert(fread(b,1,4,f)==4);return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static uint64_t ca_long(FILE *f){uint64_t high=(uint32_t)ca_int(f);return high<<32|(uint32_t)ca_int(f);}
static void ca_terrain(int hm[4][105][105],uint8_t flags[4][104][104]){for(int l=0;l<4;l++)for(int x=0;x<105;x++)for(int z=0;z<105;z++){hm[l][x][z]=-l*240-x*13-z*7+x*z%37;if(x<104 && z<104)flags[l][x][z]=(uint8_t)((l==1 && (x+z)%3==0?2:0)|((x+z)%4==0?4:0));}}
static void ca_orbit(Camera *c){c->orbit_x=c->orbit_z=6400;c->orbit_yaw=43;c->orbit_pitch=200;c->pitch_clamp=50000;c->anticheat_yaw=9;c->yaw_velocity=c->pitch_velocity=0;}
static void ca_fresh(Camera *c){camera_init(c,6400,6400);ca_orbit(c);c->eye_x=6400;c->eye_y=-900;c->eye_z=6000;c->render_pitch=200;c->render_yaw=2000;}
static void ca_state(FILE *f,const Camera *c,int record){assert((fgetc(f)!=0)==c->cutscene);int v[]={c->src_x,c->src_z,c->src_height,c->move_speed,c->move_acceleration,c->dst_x,c->dst_z,c->dst_height,c->rotate_speed,c->rotate_acceleration,c->eye_x,c->eye_y,c->eye_z,c->render_pitch,c->render_yaw};for(int i=0;i<15;i++){int e=ca_int(f);if(e!=v[i])fprintf(stderr,"Camera record%d field%d expected%d got%d\n",record,i,e,v[i]);assert(e==v[i]);}for(int i=0;i<5;i++){assert((fgetc(f)!=0)==c->modifier_enabled[i]);assert(ca_int(f)==c->modifier_jitter[i]);assert(ca_int(f)==c->modifier_scale[i]);assert(ca_int(f)==c->modifier_speed[i]);assert(ca_int(f)==c->modifier_cycle[i]);}}
typedef struct {
 void (*packet)(Camera*,int,const uint8_t*,size_t,int);
 void (*tick)(Camera*,int);
 void (*draw)(Camera*,const Camera*,int,uint64_t,int);
} CameraReferenceHooks;
static void camera_reference_run(const char *path,Camera *c,int hm[4][105][105],uint8_t flags[4][104][104],const CameraReferenceHooks *hooks){
 FILE *f=fopen(path,"rb");assert(f && ca_int(f)==0x43413235);int cases=ca_int(f);assert(cases==3307);World *w=calloc(1,sizeof(*w));assert(w);ca_terrain(hm,flags);memcpy(w->tile_flags,flags,sizeof(w->tile_flags));int level=0,packets=0,ticks=0,draws=0;
 for(int n=0;n<cases;n++){int op=ca_int(f),length=ca_int(f);uint8_t payload[6];assert(length>=0 && length<=6 && fread(payload,1,length,f)==(size_t)length);uint64_t seed=ca_long(f);
  if(op==-3){ca_fresh(c);level=payload[0];if(length>1)c->eye_x=c->eye_z=-2000;}
  else if(op==-1){if(hooks && hooks->tick)hooks->tick(c,level);else {camera_cutscene_tick225(c,level,hm,flags);camera_modifier_tick225(c);}ticks++;}
  else if(op==-2){camera_prepare_draw(c,6400,6400,level,hm,flags);Camera before=*c,view;math_random_seed((int64_t)seed);camera_shake225(c,&view);int top=c->cutscene?camera_cutscene_top225(c,level,hm,flags):scene_runtime225_top_level(w,level,c->eye_x,c->eye_z,6400,6400,c->render_pitch);int v[]={view.eye_x,view.eye_y,view.eye_z,view.render_pitch,view.render_yaw,top};for(int i=0;i<6;i++){int e=ca_int(f);if(e!=v[i])fprintf(stderr,"Camera draw%d field%d expected%d got%d\n",n,i,e,v[i]);assert(e==v[i]);}assert(ca_long(f)==math_random_state());assert(!memcmp(&before,c,sizeof(*c)));if(hooks && hooks->draw)hooks->draw(c,&view,level,seed,draws);draws++;}
  else {if(hooks && hooks->packet)hooks->packet(c,op,payload,(size_t)length,level);else assert(camera_packet225(c,op,payload,(size_t)length,level,hm,flags));packets++;}
  ca_state(f,c,n);
 }
 assert(fgetc(f)==EOF && packets==825 && ticks==1552 && draws==900);fclose(f);free(w);puts("PASS camera original:3307 states,825 packets,1552 cutscene ticks,900 actual original draw camera/RNG views;all5 modifier arrays,base restore,height/angle/top-level/pitch clamp exact");
}
static void camera_fault_controls(void){Camera c;ca_fresh(&c);Camera before=c;static int hm[4][105][105];static uint8_t flags[4][104][104];uint8_t data[7]={0};int faults=0;for(int opindex=0;opindex<4;opindex++){int op=(int[]){3,13,74,239}[opindex],valid=op==239?0:op==13?4:6;for(int n=0;n<=7;n++)if(n!=valid){assert(!camera_packet225(&c,op,data,(size_t)n,0,hm,flags) && !memcmp(&c,&before,sizeof(c)));faults++;}}data[0]=5;assert(!camera_packet225(&c,13,data,4,0,hm,flags) && !memcmp(&c,&before,sizeof(c)));assert(!camera_packet225(&c,3,data,6,4,hm,flags));assert(!camera_packet225(&c,3,NULL,6,0,hm,flags));for(int i=0;i<5;i++)c.modifier_cycle[i]=INT32_MAX;camera_modifier_tick225(&c);for(int i=0;i<5;i++)assert(c.modifier_cycle[i]==INT32_MIN);assert(faults==28);puts("PASS camera safety:28 length faults,invalid modifier/plane/null payload,cycle wrap;rejection retains state");}
