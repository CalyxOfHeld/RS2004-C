#include "entity/npc_entity.h"
#include <string.h>

void npc_entity_init(NpcEntity *npc) {
    if (!npc) return;
    memset(npc, 0, sizeof(NpcEntity));
    pathing_entity_init(&npc->base);
    npc->has_type = false;
}

Model* npc_entity_get_model(NpcEntity *npc, Arena *arena) {
    if (!npc || !npc->has_type) return NULL;
    return npc_get_model(&npc->type, arena);
}

void npc_entity_free(NpcEntity *npc) {
    if (!npc) return;
    if (npc->has_type) {
        npc_free_entry(&npc->type);
        npc->has_type = false;
    }
}
