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
int main(void) {
 setbuf(stdout,NULL);Arena arena=arena_create(64*1024*1024);Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&arena),*models=jagfile_load_file("../engine/data/pack/client/models",&arena);assert(config&&models);assert(anim_unpack(models,&arena));SeqConfig seq;seq_unpack(&seq,config);
 FILE *f=fopen("tests/fixtures/actor-clock-reference.bin","rb");assert(f && rd(f)==0x41434c31);int cases=rd(f),ticks=0;
 for(int i=0;i<cases;i++) {
  int id=rd(f),variant=rd(f);PathingEntity e;pathing_entity_init(&e);
  e.primary_seq_id=rd(f);e.primary_seq_frame=rd(f);e.primary_seq_cycle=rd(f);e.primary_seq_loop=rd(f);e.primary_seq_delay=rd(f);e.secondary_seq_id=rd(f);e.secondary_seq_frame=rd(f);e.secondary_seq_cycle=rd(f);e.seq_stretches=rd(f);
  int count=rd(f);for(int tick=0;tick<count;tick++){pathing_entity_update_sequences(&e,&seq);compare(f,&e,id,variant,tick);ticks++;}
 }
 assert(fgetc(f)==EOF);fclose(f);printf("PASS: %d original primary/secondary clocks, %d complete states across all %d sequences, frame wrapping, delay, replay boundary, placeholders and stretch flags\n",cases,ticks,seq.count);seq_free(&seq);arena_free(&arena);return 0;
}
