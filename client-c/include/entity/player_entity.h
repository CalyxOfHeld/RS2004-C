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
    Model *loc_model;
    int loc_start_cycle,loc_stop_cycle,loc_offset_x,loc_offset_y,loc_offset_z;
    int loc_min_tile_x,loc_max_tile_x,loc_min_tile_z,loc_max_tile_z;
} PlayerEntity;

void   player_entity_init(PlayerEntity *p);
void   player_entity_dispose(PlayerEntity *p);
Model *player_with_location(Model *body,PlayerEntity *player,int cycle,int ground_y,Arena *arena);
void   player_entity_read_appearance(PlayerEntity *p, Packet *pkt);
Model* player_entity_get_model(PlayerEntity *p, IdkConfig *idk_cfg, ObjConfig *obj_cfg, SeqConfig *seq_cfg, Arena *arena);
Model* player_entity_get_head_model(PlayerEntity *p, IdkConfig *idk_cfg, ObjConfig *obj_cfg, Arena *arena);

#endif // RS2_PLAYER_ENTITY_H
