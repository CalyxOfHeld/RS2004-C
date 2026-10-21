#ifndef RS2_PLAYER_ENTITY_H
#define RS2_PLAYER_ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "entity/pathing_entity.h"
#include "config/idk.h"
#include "config/obj.h"
#include "config/seq.h"
#include "core/packet.h"
#include "core/arena.h"

typedef struct {
    PathingEntity base;
    char name[64];
    bool visible;
    int gender;
    int headicons;
    int appearances[12];
    int colors[5];
    int combat_level;
    uint64_t appearance_hashcode;
} PlayerEntity;

void   player_entity_init(PlayerEntity *p);
void   player_entity_read_appearance(PlayerEntity *p, Packet *pkt);
Model* player_entity_get_model(PlayerEntity *p, IdkConfig *idk_cfg, ObjConfig *obj_cfg, SeqConfig *seq_cfg, Arena *arena);
Model* player_entity_get_head_model(PlayerEntity *p, IdkConfig *idk_cfg, ObjConfig *obj_cfg, Arena *arena);

#endif // RS2_PLAYER_ENTITY_H
