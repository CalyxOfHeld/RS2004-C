#include "entity/pathing_entity.h"
#include "config/seq.h"
#include "gfx/anim.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static int rd(FILE *f){uint8_t b[4];assert(fread(b,1,4,f)==4);return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static void compare(FILE *f,PathingEntity *e,int id,int variant,int tick) {
 int fields[]={e->primary_seq_id,e->primary_seq_frame,e->primary_seq_cycle,e->primary_seq_loop,e->primary_seq_delay,e->secondary_seq_id,e->secondary_seq_frame,e->secondary_seq_cycle,e->seq_stretches};
 for(int i=0;i<9;i++){int expected=rd(f);if(fields[i]!=expected){fprintf(stderr,"clock id=%d variant=%d tick=%d field=%d actual=%d expected=%d\n",id,variant,tick,i,fields[i],expected);abort();}}
}
static void bounds_controls(void) {
 const int values[]={-1,127,128,1535,1536,11775,11776,13183,13184};
 for(int local=0;local<2;local++)for(int axis=0;axis<2;axis++)for(int n=0;n<9;n++) {
  PathingEntity e;pathing_entity_init(&e);e.x=e.z=6400;e.size=2;
  if(axis)e.z=values[n];else e.x=values[n];
  e.path_tile_x[0]=40;e.path_tile_z[0]=41;e.path_length=3;
  e.primary_seq_id=123;e.spotanim_id=456;e.exact_end_cycle=10;e.exact_start_cycle=20;
  e.secondary_seq_frame=1;e.secondary_seq_cycle=6;
  int x=e.x,z=e.z,minimum=local?1536:128,maximum=local?11776:13184;
  bool reset=values[n]<minimum || values[n]>=maximum;
  pathing_entity_validate225(&e,local!=0);
  assert(e.x==(reset?40*128+128:x) && e.z==(reset?41*128+128:z));
  assert(e.path_length==(reset?0:3));
  assert(e.primary_seq_id==(reset?-1:123) && e.spotanim_id==(reset?-1:456));
  assert(e.exact_end_cycle==(reset?0:10) && e.exact_start_cycle==(reset?0:20));
  assert(e.secondary_seq_frame==1 && e.secondary_seq_cycle==6);
 }
 puts("PASS: original general/local bounds, both axes and retained locomotion clock (36 controls)");
}
int main(void) {
 bounds_controls();
 setbuf(stdout,NULL);Arena arena=arena_create(64*1024*1024);Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&arena),*models=jagfile_load_file("../engine/data/pack/client/models",&arena);assert(config&&models);assert(anim_unpack(models,&arena));SeqConfig seq;seq_unpack(&seq,config);
 FILE *f=fopen("tests/fixtures/actor-clock-reference.bin","rb");assert(f && rd(f)==0x41434c31);int cases=rd(f),ticks=0;
 for(int i=0;i<cases;i++) {
  int id=rd(f),variant=rd(f);PathingEntity e;pathing_entity_init(&e);
  e.primary_seq_id=rd(f);e.primary_seq_frame=rd(f);e.primary_seq_cycle=rd(f);e.primary_seq_loop=rd(f);e.primary_seq_delay=rd(f);e.secondary_seq_id=rd(f);e.secondary_seq_frame=rd(f);e.secondary_seq_cycle=rd(f);e.seq_stretches=rd(f);
  int count=rd(f);for(int tick=0;tick<count;tick++){pathing_entity_update_sequences(&e,&seq);compare(f,&e,id,variant,tick);ticks++;}
 }
 assert(fgetc(f)==EOF);fclose(f);printf("PASS: %d original primary/secondary clocks, %d complete states across all %d sequences, frame wrapping, delay, replay boundary, placeholders and stretch flags\n",cases,ticks,seq.count);seq_free(&seq);arena_free(&arena);return 0;
}
