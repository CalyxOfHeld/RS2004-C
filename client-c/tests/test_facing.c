#include "net/entity_sync.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static int rd(FILE *f){uint8_t b[4];assert(fread(b,1,4,f)==4);return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
int main(void) {
 EntitySyncContext *ctx=calloc(1,sizeof(*ctx));assert(ctx);entity_sync_init(ctx,NULL);ctx->local_player.base.x=6336;ctx->local_player.base.z=6400;ctx->players[17]=calloc(1,sizeof(PlayerEntity));ctx->npcs[42]=calloc(1,sizeof(NpcEntity));assert(ctx->players[17]&&ctx->npcs[42]);
 FILE *f=fopen("tests/fixtures/facing-reference.bin","rb");assert(f && rd(f)==0x46414331);int count=rd(f);
 for(int n=0;n<count;n++) {
  PathingEntity a;pathing_entity_init(&a);a.x=rd(f);a.z=rd(f);a.yaw=rd(f);a.dst_yaw=rd(f);a.target_id=rd(f);a.path_length=rd(f);a.seq_trigger=rd(f);a.target_tile_x=rd(f);a.target_tile_z=rd(f);a.seq_turn_id=rd(f);a.seq_stand_id=a.secondary_seq_id=808;a.seq_walk_id=819;
  ctx->npcs[42]->base.x=rd(f);ctx->npcs[42]->base.z=rd(f);ctx->players[17]->base.x=rd(f);ctx->players[17]->base.z=rd(f);
  entity_sync_update_facing(ctx,&a,3152,3200,11);int fields[]={a.yaw,a.dst_yaw,a.secondary_seq_id,a.target_tile_x,a.target_tile_z};
  for(int i=0;i<5;i++){int expected=rd(f);if(fields[i]!=expected){fprintf(stderr,"facing case=%d field=%d actual=%d expected=%d\n",n,i,fields[i],expected);abort();}}
 }
 assert(fgetc(f)==EOF);fclose(f);printf("PASS: %d original entity/coordinate facing states, NPC/player targets, local-PID alias, target consumption, turn and movement gates\n",count);entity_sync_cleanup(ctx);free(ctx);return 0;
}
