#ifndef RS2_ACTOR_MODEL_H
#define RS2_ACTOR_MODEL_H
#include "gfx/model.h"
#include "entity/pathing_entity.h"
#include "config/seq.h"

// Original independent NPC(30) and player(200) rest-model caches.
Model *actor_model_cached(bool player,uint64_t key);
Model *actor_model_store(bool player,uint64_t key,Model *rest);
Arena *actor_model_scratch(void);
void actor_models_clear(void);
void actor_model_frames(const PathingEntity *actor,const SeqConfig *sequences,int *primary,int *secondary,const int **mask);
#endif
