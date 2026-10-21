#include "entity/actor_model.h"
#include <string.h>
typedef struct {uint64_t key,age;Model *model;} ActorModelEntry;
static ActorModelEntry player_cache[200],npc_cache[30];
static uint64_t model_age;
static Arena scratch;
Model *actor_model_cached(bool player,uint64_t key) {
    ActorModelEntry *cache=player ? player_cache:npc_cache;int count=player ? 200:30;
    for(int i=0;i<count;i++)if(cache[i].model && cache[i].key==key){cache[i].age=++model_age;return cache[i].model;}
    return NULL;
}
Model *actor_model_store(bool player,uint64_t key,Model *rest) {
    if(!rest)return NULL;
    Model *copy=model_copy(rest,NULL);if(!copy)return NULL;
    ActorModelEntry *cache=player ? player_cache:npc_cache;int count=player ? 200:30,oldest=0;
    for(int i=0;i<count;i++){if(!cache[i].model){oldest=i;break;}if(cache[i].age<cache[oldest].age)oldest=i;}
    model_free_unshared(cache[oldest].model);cache[oldest]=(ActorModelEntry){key,++model_age,copy};return copy;
}
Arena *actor_model_scratch(void) {
    if(!scratch.buffer)scratch=arena_create(8*1024*1024);
    arena_reset(&scratch);return &scratch;
}
void actor_models_clear(void) {
    for(int i=0;i<200;i++){model_free_unshared(player_cache[i].model);player_cache[i]=(ActorModelEntry){0};}
    for(int i=0;i<30;i++){model_free_unshared(npc_cache[i].model);npc_cache[i]=(ActorModelEntry){0};}
    arena_free(&scratch);model_age=0;
}
static int frame(const SeqConfig *sequences,int id,int index) {
    if(!sequences || id<0 || id>=sequences->count)return -1;
    const SeqType *seq=&sequences->instances[id];return index>=0 && index<seq->frame_count ? seq->frames[index]:-1;
}
void actor_model_frames(const PathingEntity *actor,const SeqConfig *sequences,int *primary,int *secondary,const int **mask) {
    *primary=*secondary=-1;*mask=NULL;
    if(!sequences)return;
    if(actor->primary_seq_id>=0 && actor->primary_seq_id<sequences->count && actor->primary_seq_delay==0) {
        *primary=frame(sequences,actor->primary_seq_id,actor->primary_seq_frame);
        if(actor->secondary_seq_id>=0 && actor->secondary_seq_id!=actor->seq_stand_id)*secondary=frame(sequences,actor->secondary_seq_id,actor->secondary_seq_frame);
        *mask=sequences->instances[actor->primary_seq_id].walkmerge;
    } else *primary=frame(sequences,actor->secondary_seq_id,actor->secondary_seq_frame);
}
