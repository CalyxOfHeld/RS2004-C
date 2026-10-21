#include "net/entity_sync.h"
#include "gfx/anim.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int rd(FILE *f){uint8_t b[4];assert(fread(b,1,4,f)==4);return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static void state(FILE *f,PathingEntity *p,int test,int tick) {
    int fields[]={p->x,p->z,p->yaw,p->dst_yaw,p->seq_trigger,p->path_length,p->path_tile_x[0],p->path_tile_z[0],p->exact_end_cycle,p->exact_start_cycle,p->primary_seq_id,p->primary_seq_frame,p->primary_seq_cycle,p->primary_seq_delay,p->primary_seq_loop,p->secondary_seq_id,p->secondary_seq_frame,p->secondary_seq_cycle};
    for(int i=0;i<18;i++){int expected=rd(f);if(fields[i]!=expected){fprintf(stderr,"exact case=%d tick=%d field=%d actual=%d expected=%d\n",test,tick,i,fields[i],expected);abort();}}
}
int main(void) {
    setbuf(stdout,NULL);Arena arena=arena_create(64*1024*1024);Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&arena),*models=jagfile_load_file("../engine/data/pack/client/models",&arena);assert(config&&models);model_init_repo(models,&arena);assert(anim_unpack(models,&arena));SeqConfig seq;seq_unpack(&seq,config);
    EntitySyncContext *ctx=calloc(1,sizeof(*ctx));assert(ctx);entity_sync_init(ctx,NULL);ctx->seq_cfg=&seq;
    FILE *f=fopen("tests/fixtures/exact-reference.bin","rb");assert(f && rd(f)==0x45584331);int cases=rd(f),frames=0;
    for(int n=0;n<cases;n++) {
        PathingEntity *p=&ctx->local_player.base;player_entity_init(&ctx->local_player);
        int facing=rd(f),start=rd(f),duration=rd(f),sequence=rd(f),delay=rd(f);
        p->x=43*128+64;p->z=44*128+64;p->path_tile_x[0]=43;p->path_tile_z[0]=44;p->path_length=3;p->seq_trigger=5;p->primary_seq_id=sequence;p->primary_seq_delay=delay;
        uint8_t bytes[20]={0};int bit=0;
        #define BITS(v,n) do{unsigned value=(v);for(int j=(n)-1;j>=0;j--,bit++)if((value>>j)&1)bytes[bit/8]|=1u<<(7-(bit&7));}while(0)
        BITS(1,1);BITS(0,2);BITS(0,8);BITS(2047,11);
        #undef BITS
        size_t size=(bit+7)/8;bytes[size++]=128;bytes[size++]=2;bytes[size++]=46;bytes[size++]=44;bytes[size++]=46;bytes[size++]=47;
        bytes[size++]=start>>8;bytes[size++]=start;bytes[size++]=(start+duration)>>8;bytes[size++]=start+duration;bytes[size++]=facing;
        Packet packet=packet_wrap(bytes,size);ctx->cycle=1000;assert(entity_sync_player_info(ctx,&packet,size));state(f,p,n,-1);
        int ticks=rd(f);
        for(int tick=0;tick<ticks;tick++) {
            int cycle=rd(f);if(!pathing_entity_update_exact(p,&seq,cycle))pathing_entity_update_movement(p);pathing_entity_update_facing(p);pathing_entity_update_sequences(p,&seq);state(f,p,n,cycle);frames++;
        }
    }
    int pairs=rd(f);
    for(int n=0;n<pairs;n++) {
        int previous=rd(f),id=rd(f);PathingEntity p;pathing_entity_init(&p);p.primary_seq_id=previous;p.primary_seq_frame=2;p.primary_seq_cycle=3;p.primary_seq_loop=4;p.primary_seq_delay=5;
        assert(pathing_entity_set_sequence(&p,&seq,id,7));int fields[]={p.primary_seq_id,p.primary_seq_frame,p.primary_seq_cycle,p.primary_seq_loop,p.primary_seq_delay};
        for(int i=0;i<5;i++)assert(fields[i]==rd(f));
    }
    assert(fgetc(f)==EOF);fclose(f);
    // The NPC receive mask uses the same original restart/priority decision.
    ctx->npc_count=1;ctx->npc_indices[0]=17;ctx->npcs[17]=calloc(1,sizeof(NpcEntity));assert(ctx->npcs[17]);pathing_entity_init(&ctx->npcs[17]->base);
    ctx->npcs[17]->base.primary_seq_frame=8;ctx->npcs[17]->base.primary_seq_cycle=3;
    uint8_t npc_info[12]={0};int bit=0;
    #define BITS(v,n) do{unsigned value=(v);for(int j=(n)-1;j>=0;j--,bit++)if((value>>j)&1)npc_info[bit/8]|=1u<<(7-(bit&7));}while(0)
    BITS(1,8);BITS(1,1);BITS(0,2);BITS(8191,13);
    #undef BITS
    size_t bytes=(bit+7)/8;npc_info[bytes++]=2;npc_info[bytes++]=749>>8;npc_info[bytes++]=749&255;npc_info[bytes++]=7;
    Packet npc_packet=packet_wrap(npc_info,bytes);assert(entity_sync_npc_info(ctx,&npc_packet,bytes));
    assert(ctx->npcs[17]->base.primary_seq_id==749 && ctx->npcs[17]->base.primary_seq_frame==0 && ctx->npcs[17]->base.primary_seq_cycle==0 && ctx->npcs[17]->base.primary_seq_delay==7);
    PathingEntity invalid;pathing_entity_init(&invalid);invalid.exact_end_cycle=invalid.exact_start_cycle=1000;invalid.exact_end_x=50;invalid.exact_end_z=51;assert(pathing_entity_update_exact(&invalid,&seq,1000));assert(invalid.x==6464 && invalid.z==6592);
    assert(!pathing_entity_set_sequence(&invalid,&seq,seq.count,0));
    printf("PASS: %d actual original PLAYER_INFO forced-movement mask lifecycles, %d tick states (lead-in, interpolation, frame gates, directions and completion); %d original primary-sequence priority/restart decisions\n",cases,frames,pairs);
    puts("PASS: native zero-duration forced-movement guard and invalid animation ID rejection");
    entity_sync_cleanup(ctx);free(ctx);seq_free(&seq);arena_free(&arena);return 0;
}
