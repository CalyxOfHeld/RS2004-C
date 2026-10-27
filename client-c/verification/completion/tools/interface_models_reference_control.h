#include "ui/component_packets.h"
#include "config/obj.h"
#include "gfx/pix3d.h"
#include "core/jagfile.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int im_int(FILE *f){uint8_t b[4];assert(fread(b,1,4,f)==4);return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static void im_array(FILE *f,const int32_t *data,int length,int record,int field){int n=im_int(f);if(n<0){assert(!data);return;}assert(n==length && (data || !n));for(int i=0;i<n;i++){int expected=im_int(f);if(expected!=data[i])fprintf(stderr,"Interface record%d array%d index%d expected%d got%d\n",record,field,i,expected,data[i]);assert(expected==data[i]);}}
static void im_mesh(FILE *f,ComponentConfig *cfg,const Model *m,int record){bool present=fgetc(f)!=0;assert(present==(m!=NULL));if(!present)return;assert(im_int(f)==m->num_points && im_int(f)==m->num_faces && im_int(f)==m->num_t && im_int(f)==m->priority);const int32_t *arrays[]={m->point_x,m->point_y,m->point_z,m->face_vertex_a,m->face_vertex_b,m->face_vertex_c,component_model_colourless225(cfg,m)?NULL:m->face_colour,m->face_colour_a,m->face_colour_b,m->face_colour_c,m->face_render_type,m->face_priority,m->face_alpha,m->texture_a,m->texture_b,m->texture_c};for(int i=0;i<16;i++)im_array(f,arrays[i],i<3?m->num_points:i>=13?m->num_t:m->num_faces,record,i);}
typedef struct {void (*packet)(int,const uint8_t*,size_t);void (*picture)(Component*,const uint32_t*,int);} InterfaceModelsHooks;
static void interface_models_reference_run(const char *path,ComponentConfig *cfg,VarpContext *varps,Arena *arena,const InterfaceModelsHooks *hooks){
 FILE *f=fopen(path,"rb");assert(f && im_int(f)==0x494d3235);int cases=im_int(f);assert(cases==378);int ids[3];for(int i=0;i<3;i++){ids[i]=im_int(f);Component *c=component_get(cfg,ids[i]);assert(c && c->type==6);c->model=NULL;c->xan=256;c->yan=0;c->zoom=1000;c->anim_id=c->active_anim_id=-1;}
 pix3d_init();for(int i=0;i<65536;i++)pix3d_colour_table[i]=im_int(f);model_use_preview225(true);Arena frame=arena_create(16*1024*1024);assert(frame.buffer);int pictures=0;uint32_t *pixels=calloc(512*334+2,sizeof(*pixels)),*expected=malloc(512*334*sizeof(*expected));assert(pixels && expected);
 for(int n=0;n<cases;n++){int op=im_int(f),length=im_int(f);uint8_t payload[6];assert(length>=0 && length<=6 && fread(payload,1,length,f)==(size_t)length);if(hooks && hooks->packet)hooks->packet(op,payload,(size_t)length);else assert(component_apply_packet(cfg,varps,arena,op,payload,(size_t)length)==1);
  for(int i=0;i<3;i++){Component *c=component_get(cfg,ids[i]);assert(im_int(f)==c->xan && im_int(f)==c->yan && im_int(f)==c->zoom);im_mesh(f,cfg,c->model,n);}for(int i=0;i<3;i++)for(int j=0;j<3;j++)assert((fgetc(f)!=0)==(component_get(cfg,ids[i])->model==component_get(cfg,ids[j])->model));int owned,cached;component_models_stats225(cfg,&owned,&cached);assert(cached<=50 && owned<=53);
  if(fgetc(f)!=0){int slot=im_int(f);assert(slot>=0 && slot<3);for(int i=0;i<512*334;i++)expected[i]=(uint32_t)im_int(f);Component *c=component_get(cfg,ids[slot]);Model *base=c->model;arena_reset(&frame);if(!component_model_colourless225(cfg,base)){c->model=model_copy(base,&frame);assert(c->model);model_calculate_normals(c->model,64,768,-50,-10,-50);}for(int i=0;i<512*334;i++)pixels[i+1]=0;pixels[0]=pixels[512*334+1]=0xdeadbeef;pix2d_bind(pixels+1,512,334);pix3d_init_2d();component_draw(cfg,c,0,0,0);c->model=base;for(int i=0;i<512*334;i++){if(pixels[i+1]!=expected[i])fprintf(stderr,"Interface preview record%d pixel%d expected%x got%x\n",n,i,expected[i],pixels[i+1]);assert(pixels[i+1]==expected[i]);}assert(pixels[0]==0xdeadbeef && pixels[512*334+1]==0xdeadbeef);if(hooks && hooks->picture)hooks->picture(c,expected,pictures);pictures++;}
 }
 assert(fgetc(f)==EOF && pictures==37);fclose(f);free(pixels);free(expected);arena_free(&frame);puts("PASS interface models original:378 packet states,3 actual cached widgets,complete mesh arrays/null colours/shading/alias/LRU50/count50;37 complete512x334 previews,6327296 pixels and guards");
}
