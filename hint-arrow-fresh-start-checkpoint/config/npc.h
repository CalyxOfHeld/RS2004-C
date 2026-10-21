#ifndef NPC_TYPE_H
#define NPC_TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include "core/jagfile.h"
#include "core/packet.h"
#include "gfx/model.h"

typedef struct NpcType {
    int index;
    char name[64];
    char desc[128];
    int8_t size;
    int* models;
    int model_count;
    int* heads;
    int head_count;
    int readyanim;
    int walkanim;
    int walkanim_b;
    int walkanim_r;
    int walkanim_l;
    bool anim_has_alpha;
    int* recol_s;
    int* recol_d;
    int recol_count;
    char op[5][32];
    bool op_valid[5];
    int resizex;
    int resizey;
    int resizez;
    bool minimap;
    int vislevel; // combat level
    int resizeh;
    int resizev;
} NpcType;

typedef struct NpcConfig {
    int count;
    int* offsets;
    uint8_t* dat;
    int32_t dat_len;
} NpcConfig;

void npc_unpack(NpcConfig* config, Jagfile* jag);
void npc_reset(NpcType* npc);
void npc_decode(NpcType* npc, Packet* pkt);
bool npc_get(NpcConfig* config, int id, NpcType* out_npc);
Model* npc_get_model(NpcType* npc, Arena* arena);
Model* npc_get_head_model(NpcType* npc, Arena* arena);
void npc_free_entry(NpcType* npc);
void npc_free(NpcConfig* config);

#endif // NPC_TYPE_H
