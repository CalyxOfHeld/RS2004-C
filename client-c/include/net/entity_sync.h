#ifndef RS2_ENTITY_SYNC_H
#define RS2_ENTITY_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include "entity/player_entity.h"
#include "entity/npc_entity.h"
#include "config/npc.h"
#include "core/packet.h"

#define MAX_PLAYERS 2048
#define MAX_NPCS 8192

typedef struct {
    PlayerEntity local_player;
    int local_player_index;
    int current_plane;
    int cycle;
    int in_multizone;
    bool trace;

    PlayerEntity *players[MAX_PLAYERS];
    int player_indices[MAX_PLAYERS];
    int player_count;

    NpcEntity *npcs[MAX_NPCS];
    int npc_indices[MAX_NPCS];
    int npc_count;

    NpcConfig *npc_cfg;
    SeqConfig *seq_cfg;
    /* Packet text has been bounded before this callback. packed=false is the
     * original forced0x8 path; packed=true is filtered public0x40. */
    void (*player_chat)(void *user,PlayerEntity *player,bool packed,int colour,int rights,const uint8_t *text,size_t length);
    void *player_chat_user;
} EntitySyncContext;

void entity_sync_init(EntitySyncContext *ctx, NpcConfig *npc_cfg);
void entity_sync_rebase(EntitySyncContext *ctx, int delta_x, int delta_z);
void entity_sync_update_facing(EntitySyncContext *ctx,PathingEntity *actor,int base_x,int base_z,int local_pid);
bool entity_sync_player_info(EntitySyncContext *ctx, Packet *pkt, size_t packet_size);
bool entity_sync_npc_info(EntitySyncContext *ctx, Packet *pkt, size_t packet_size);
void entity_sync_tick_chats(EntitySyncContext *ctx);
void entity_sync_cleanup(EntitySyncContext *ctx);

int entity_sync_control225(EntitySyncContext *ctx,int opcode,const uint8_t *payload,size_t length);
#endif // RS2_ENTITY_SYNC_H
