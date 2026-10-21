#ifndef RS2_NPC_ENTITY_H
#define RS2_NPC_ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "entity/pathing_entity.h"
#include "config/npc.h"
#include "gfx/model.h"

typedef struct {
    PathingEntity base;
    NpcType type;
    bool has_type;
} NpcEntity;

void   npc_entity_init(NpcEntity *npc);
Model* npc_entity_get_model(NpcEntity *npc, Arena *arena);
Model* npc_entity_get_animated_model(NpcEntity *npc,const struct SeqConfig *sequences,Arena *arena);
void   npc_entity_free(NpcEntity *npc);

#endif // RS2_NPC_ENTITY_H
