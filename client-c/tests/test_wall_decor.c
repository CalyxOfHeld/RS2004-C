#include "dash3d/world.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int rd(FILE *f){unsigned char b[4];assert(fread(b,1,4,f)==4);return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static int smart(unsigned char *b,int n){if(n<128){b[0]=n;return 1;}b[0]=(n+32768)>>8;b[1]=n;return 2;}
static void add(World *w,World3D *s,LocConfig *cfg,int id,int shape,int rot) {
 unsigned char b[12];int n=smart(b,id+1);n+=smart(b+n,40*64+41);b[n++]=shape*4+rot;b[n++]=0;b[n++]=0;
 world_load_locations(w,b,n,cfg,s,NULL,0,0);
}
int main(void) {
 Arena perm=arena_create(32*1024*1024);Jagfile *cfg=jagfile_load_file("../engine/data/pack/client/config",&perm),*models=jagfile_load_file("../engine/data/pack/client/models",&perm);assert(cfg&&models);model_init_repo(models,&perm);LocConfig locs;loc_unpack(&locs,cfg);
 World *w=calloc(1,sizeof(*w));World3D *s=calloc(1,sizeof(*s));assert(w&&s);world_init(w,104,104);world3d_init(s,104,104,4);
 FILE *f=fopen("tests/fixtures/wall-decor-reference.bin","rb");assert(f&&rd(f)==0x57444331);int cases=rd(f),draws=0;
 for(int c=0;c<cases;c++) {
  world_reset(w);world3d_reset(s);int shape=rd(f),rot=rd(f),order=rd(f),wall=rd(f);
  w->heightmap[0][40][40]=rd(f);w->heightmap[0][41][40]=rd(f);w->heightmap[0][41][41]=rd(f);w->heightmap[0][40][41]=rd(f);
  if(order==1)add(w,s,&locs,wall,0,rot);add(w,s,&locs,1938,shape,rot);if(order==2)add(w,s,&locs,wall,0,rot);
  WorldLocation *loc=NULL;GroundTile *t=&s->tiles[0][40][40];for(int j=0;j<t->loc_count;j++)if(t->locs[j].shape==shape)loc=&t->locs[j];assert(loc);
  assert(rd(f)==loc->x);assert(rd(f)==loc->y);assert(rd(f)==loc->z);assert(rd(f)==loc->model->num_points);
  for(int j=0;j<loc->model->num_points;j++){assert(rd(f)==loc->model->point_x[j]);assert(rd(f)==loc->model->point_y[j]);assert(rd(f)==loc->model->point_z[j]);}
  int eyes=rd(f);
  for(int eye=0;eye<eyes;eye++) {
   int ex=rd(f),ez=rd(f),count=rd(f);WorldLocationDraw d[2];int got=world3d_location_draws(loc,ex,ez,d);
   if(count!=got){fprintf(stderr,"decor case=%d shape=%d rotation=%d eye=%d,%d expected draws=%d actual=%d\n",c,shape,rot,ex,ez,count,got);abort();}
   for(int j=0;j<count;j++){assert(rd(f)==d[j].yaw);assert(rd(f)==d[j].x);assert(rd(f)==d[j].y);assert(rd(f)==d[j].z);draws++;}
  }
 }
 assert(fgetc(f)==EOF);fclose(f);world3d_dispose(s);free(s);free(w);loc_clear_model_cache(true);loc_free(&locs);arena_free(&perm);
 printf("PASS: %d original wall-decoration placements, canonical/skewed meshes, 3000 camera cases and %d original draw poses across shapes4..8, all rotations and wall insertion orders\n",cases,draws);return 0;
}
