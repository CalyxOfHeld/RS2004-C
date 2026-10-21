#include "net/entity_sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

void entity_sync_init(EntitySyncContext *ctx, NpcConfig *npc_cfg) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(EntitySyncContext));
    player_entity_init(&ctx->local_player);
    ctx->local_player_index = 2047; // default local player index
    ctx->npc_cfg = npc_cfg;
}

static void rebase_entity(PathingEntity *entity, int dx, int dz) {
    // Original client.java REBUILD_NORMAL: retain every queued step and the
    // interpolated position in world space as the scene origin changes.
    for (int step=0; step<10; step++) {
        entity->path_tile_x[step]-=dx;
        entity->path_tile_z[step]-=dz;
    }
    entity->x-=dx*128;
    entity->z-=dz*128;
}

void entity_sync_update_facing(EntitySyncContext *ctx,PathingEntity *actor,int base_x,int base_z,int local_pid) {
    PathingEntity *target=NULL;
    if(actor->target_id>=0 && actor->target_id<MAX_NPCS && ctx->npcs[actor->target_id])target=&ctx->npcs[actor->target_id]->base;
    if(actor->target_id>=32768) {
        int slot=actor->target_id-32768;if(slot==local_pid)slot=ctx->local_player_index;
        if(slot==ctx->local_player_index)target=&ctx->local_player.base;
        else if(slot>=0 && slot<MAX_PLAYERS && ctx->players[slot])target=&ctx->players[slot]->base;
    }
    if(target) {
        int dx=actor->x-target->x,dz=actor->z-target->z;
        if(dx || dz)actor->dst_yaw=(int)((uint32_t)(int)(atan2((double)dx,(double)dz)*325.949)&2047u);
    }
    if((actor->target_tile_x || actor->target_tile_z) && (actor->path_length==0 || actor->seq_trigger>0)) {
        int dx=actor->x-(actor->target_tile_x-base_x-base_x)*64,dz=actor->z-(actor->target_tile_z-base_z-base_z)*64;
        if(dx || dz)actor->dst_yaw=(int)((uint32_t)(int)(atan2((double)dx,(double)dz)*325.949)&2047u);
        actor->target_tile_x=actor->target_tile_z=0;
    }
    pathing_entity_update_facing(actor);
}

void entity_sync_rebase(EntitySyncContext *ctx, int delta_x, int delta_z) {
    if (!ctx || (!delta_x && !delta_z)) return;
    rebase_entity(&ctx->local_player.base,delta_x,delta_z);
    for (int i=0;i<MAX_PLAYERS;i++)
        if (ctx->players[i] && ctx->players[i]!=&ctx->local_player)
            rebase_entity(&ctx->players[i]->base,delta_x,delta_z);
    for (int i=0;i<MAX_NPCS;i++)
        if (ctx->npcs[i]) rebase_entity(&ctx->npcs[i]->base,delta_x,delta_z);
}

bool entity_sync_player_info(EntitySyncContext *ctx, Packet *pkt, size_t packet_size) {
    if (!ctx || !pkt || packet_size>pkt->length) return false;
    pkt->length=packet_size;

    int extended_update_ids[MAX_PLAYERS];
    int extended_update_count = 0;

    access_bits(pkt);

    // 1. Local Player Movement & Update
    int local_update_req = gbit(pkt, 1);
    if (local_update_req != 0) {
        int update_type = gbit(pkt, 2);
        if (update_type == 0) {
            extended_update_ids[extended_update_count++] = ctx->local_player_index;
        } else if (update_type == 1) {
            int dir = gbit(pkt, 3);
            pathing_entity_move_along_route(&ctx->local_player.base, false, dir);
            int ext = gbit(pkt, 1);
            if (ext == 1) {
                extended_update_ids[extended_update_count++] = ctx->local_player_index;
            }
        } else if (update_type == 2) {
            int dir1 = gbit(pkt, 3);
            pathing_entity_move_along_route(&ctx->local_player.base, true, dir1);
            int dir2 = gbit(pkt, 3);
            pathing_entity_move_along_route(&ctx->local_player.base, true, dir2);
            int ext = gbit(pkt, 1);
            if (ext == 1) {
                extended_update_ids[extended_update_count++] = ctx->local_player_index;
            }
        } else if (update_type == 3) {
            ctx->current_plane = gbit(pkt, 2);
            int tile_x = gbit(pkt, 7);
            int tile_z = gbit(pkt, 7);
            int clear_walk = gbit(pkt, 1);
            pathing_entity_teleport(&ctx->local_player.base, false, clear_walk == 1, tile_x, tile_z);
            int ext = gbit(pkt, 1);
            if (ext == 1) {
                extended_update_ids[extended_update_count++] = ctx->local_player_index;
            }
        }
    }

    // 2. Existing Players Update
    int old_count = gbit(pkt, 8);
    int new_player_indices[MAX_PLAYERS];
    int new_player_count = 0;
    bool seen[MAX_PLAYERS]={false};
    if(pkt->error || old_count>ctx->player_count) return false;

    for (int i = 0; i < old_count && i < ctx->player_count; i++) {
        int slot = ctx->player_indices[i];
        PlayerEntity *p = ctx->players[slot];
        int update = gbit(pkt, 1);
        if (update == 0) {
            new_player_indices[new_player_count++] = slot;
        } else {
            int type = gbit(pkt, 2);
            if (type == 0) {
                new_player_indices[new_player_count++] = slot;
                extended_update_ids[extended_update_count++] = slot;
            } else if (type == 1) {
                new_player_indices[new_player_count++] = slot;
                int dir = gbit(pkt, 3);
                if (p) pathing_entity_move_along_route(&p->base, false, dir);
                int ext = gbit(pkt, 1);
                if (ext == 1) {
                    extended_update_ids[extended_update_count++] = slot;
                }
            } else if (type == 2) {
                new_player_indices[new_player_count++] = slot;
                int dir1 = gbit(pkt, 3);
                int dir2 = gbit(pkt, 3);
                if (p) {
                    pathing_entity_move_along_route(&p->base, true, dir1);
                    pathing_entity_move_along_route(&p->base, true, dir2);
                }
                int ext = gbit(pkt, 1);
                if (ext == 1) {
                    extended_update_ids[extended_update_count++] = slot;
                }
            } else if (type == 3) {
                if (ctx->players[slot]) ctx->players[slot]->visible=false;
            }
        }
    }

    for(int i=0;i<new_player_count;i++) seen[new_player_indices[i]]=true;
    // Read complete indices; the server may omit a sentinel when no masks follow.
    while (!pkt->error && pkt->bit_pos + 11 <= packet_size * 8) {
        int slot = gbit(pkt, 11);
        if (slot == 2047) break;
        if(pkt->bit_pos+12>packet_size*8 || new_player_count>=MAX_PLAYERS || seen[slot]) return false;
        seen[slot]=true;
        if (!ctx->players[slot]) {
            ctx->players[slot] = (PlayerEntity*)calloc(1, sizeof(PlayerEntity));
            if(!ctx->players[slot]) return false;
            player_entity_init(ctx->players[slot]);
        }
        if(ctx->players[slot]->name[0]) ctx->players[slot]->visible=true;
        new_player_indices[new_player_count++] = slot;

        int dx = gbit(pkt, 5);
        if (dx > 15) dx -= 32;
        int dz = gbit(pkt, 5);
        if (dz > 15) dz -= 32;
        int clear_walk = gbit(pkt, 1);

        pathing_entity_teleport(&ctx->players[slot]->base, false, clear_walk == 1,
                               ctx->local_player.base.path_tile_x[0] + dx,
                               ctx->local_player.base.path_tile_z[0] + dz);

        int ext = gbit(pkt, 1);
        if (ext == 1) {
            extended_update_ids[extended_update_count++] = slot;
        }
    }

    access_bytes(pkt);

    // 4. Extended Updates
    for (int i = 0; i < extended_update_count; i++) {
        int slot = extended_update_ids[i];
        PlayerEntity *p = (slot == ctx->local_player_index) ? &ctx->local_player : ctx->players[slot];
        if (!p) continue;

        int mask = g1(pkt);
        if (mask & 0x80) {
            mask += (g1(pkt) << 8);
        }

        if (mask & 0x1) {
            int len = g1(pkt);
            if(pkt->pos>packet_size || (size_t)len>packet_size-pkt->pos) return false;
            Packet appearance=packet_wrap(pkt->data+pkt->pos,len);
            player_entity_read_appearance(p,&appearance);
            if(appearance.error || appearance.pos!=(size_t)len) return false;
            pkt->pos+=(size_t)len;
        }
        if (mask & 0x2) {
            int seq = g2(pkt);
            if (seq == 65535) seq = -1;
            int delay = g1(pkt);
            if(!pathing_entity_set_sequence(&p->base,ctx->seq_cfg,seq,delay)) return false;
            if(ctx->trace)printf("[ANIM] player=%d received=%d delay=%d active=%d frame=%d cycle=%d active_delay=%d\n",slot,seq,delay,p->base.primary_seq_id,p->base.primary_seq_frame,p->base.primary_seq_cycle,p->base.primary_seq_delay);
        }
        if (mask & 0x4) {
            p->base.target_id = g2(pkt);
            if (p->base.target_id == 65535) p->base.target_id = -1;
        }
        if (mask & 0x8) {
            gjstr(pkt, p->base.chat, sizeof(p->base.chat));
            p->base.chat_timer = 150;
        }
        if (mask & 0x10) {
            p->base.damage = g1(pkt);
            p->base.damage_type = g1(pkt);
            p->base.health = g1(pkt);
            p->base.total_health = g1(pkt);
            p->base.combat_cycle=ctx->cycle+400;
            if(ctx->trace)printf("[HIT] player=%d damage=%d type=%d health=%d/%d until=%d\n",extended_update_ids[i],p->base.damage,p->base.damage_type,p->base.health,p->base.total_health,p->base.combat_cycle);
        }
        if (mask & 0x20) {
            p->base.target_tile_x = g2(pkt);
            p->base.target_tile_z = g2(pkt);
        }
        if (mask & 0x40) {
            g2(pkt);
            g1(pkt);
            int text_len = g1(pkt);
            if(pkt->pos>packet_size || (size_t)text_len>packet_size-pkt->pos) return false;
            pkt->pos += text_len;
        }
        if(mask & 0x100) {
            p->base.spotanim_id=g2(pkt);uint32_t info=(uint32_t)g4(pkt);
            p->base.spotanim_height=info>>16;p->base.spotanim_delay=ctx->cycle+(info&65535);
            p->base.spotanim_frame=p->base.spotanim_delay>ctx->cycle?-1:0;p->base.spotanim_cycle=0;
            if(p->base.spotanim_id==65535) p->base.spotanim_id=-1;
        }
        if(mask & 0x200) {
            p->base.exact_start_x=g1(pkt);p->base.exact_start_z=g1(pkt);
            p->base.exact_end_x=g1(pkt);p->base.exact_end_z=g1(pkt);
            p->base.exact_end_cycle=ctx->cycle+g2(pkt);p->base.exact_start_cycle=ctx->cycle+g2(pkt);p->base.exact_facing=g1(pkt);
            p->base.path_length=0;p->base.path_tile_x[0]=p->base.exact_end_x;p->base.path_tile_z[0]=p->base.exact_end_z;
            if(ctx->trace)printf("[EXACT] player=%d from=(%d,%d) to=(%d,%d) start=%d stop=%d direction=%d\n",slot,p->base.exact_start_x,p->base.exact_start_z,p->base.exact_end_x,p->base.exact_end_z,p->base.exact_end_cycle,p->base.exact_start_cycle,p->base.exact_facing);
        }
        if(pkt->error) return false;
    }

    // Update active player list
    if(pkt->error || pkt->pos!=packet_size) return false;
    for(int i=0;i<ctx->player_count;i++) if(!seen[ctx->player_indices[i]] && ctx->players[ctx->player_indices[i]]) ctx->players[ctx->player_indices[i]]->visible=false;
    memcpy(ctx->player_indices,new_player_indices,new_player_count*sizeof(int));
    ctx->player_count = new_player_count;
    return true;
}

bool entity_sync_npc_info(EntitySyncContext *ctx, Packet *pkt, size_t packet_size) {
    if (!ctx || !pkt || packet_size>pkt->length) return false;
    pkt->length=packet_size;

    int extended_update_ids[MAX_NPCS];
    int extended_update_count = 0;

    access_bits(pkt);

    // 1. Existing NPCs
    int old_count = gbit(pkt, 8);
    int new_npc_indices[MAX_NPCS];
    int new_npc_count = 0;
    bool seen[MAX_NPCS]={false};
    if(pkt->error || old_count>ctx->npc_count) return false;

    for (int i = 0; i < old_count && i < ctx->npc_count; i++) {
        int slot = ctx->npc_indices[i];
        NpcEntity *npc = ctx->npcs[slot];
        int update = gbit(pkt, 1);
        if (update == 0) {
            new_npc_indices[new_npc_count++] = slot;
        } else {
            int type = gbit(pkt, 2);
            if (type == 0) {
                new_npc_indices[new_npc_count++] = slot;
                extended_update_ids[extended_update_count++] = slot;
            } else if (type == 1) {
                new_npc_indices[new_npc_count++] = slot;
                int dir = gbit(pkt, 3);
                if (npc) pathing_entity_move_along_route(&npc->base, false, dir);
                int ext = gbit(pkt, 1);
                if (ext == 1) {
                    extended_update_ids[extended_update_count++] = slot;
                }
            } else if (type == 2) {
                new_npc_indices[new_npc_count++] = slot;
                int dir1 = gbit(pkt, 3);
                int dir2 = gbit(pkt, 3);
                if (npc) {
                    pathing_entity_move_along_route(&npc->base, true, dir1);
                    pathing_entity_move_along_route(&npc->base, true, dir2);
                }
                int ext = gbit(pkt, 1);
                if (ext == 1) {
                    extended_update_ids[extended_update_count++] = slot;
                }
            } else if (type == 3) {
                if (ctx->npcs[slot]) {
                    npc_entity_free(ctx->npcs[slot]);
                    free(ctx->npcs[slot]);
                    ctx->npcs[slot] = NULL;
                }
            }
        }
    }

    for(int i=0;i<new_npc_count;i++) seen[new_npc_indices[i]]=true;
    // 2. New NPCs Vis
    while (!pkt->error && pkt->bit_pos + 13 <= packet_size * 8) {
        int slot = gbit(pkt, 13);
        if (slot == 8191) break;
        if(pkt->bit_pos+22>packet_size*8 || new_npc_count>=MAX_NPCS || seen[slot]) return false;
        seen[slot]=true;
        if (!ctx->npcs[slot]) {
            ctx->npcs[slot] = (NpcEntity*)calloc(1, sizeof(NpcEntity));
            if(!ctx->npcs[slot]) return false;
            npc_entity_init(ctx->npcs[slot]);
        }
        new_npc_indices[new_npc_count++] = slot;

        int type_id = gbit(pkt, 11);
        if (ctx->npc_cfg) {
            if (ctx->npcs[slot]->has_type) {
                npc_free_entry(&ctx->npcs[slot]->type);
                ctx->npcs[slot]->has_type=false;
            }
            if (npc_get(ctx->npc_cfg, type_id, &ctx->npcs[slot]->type)) {
                ctx->npcs[slot]->has_type = true;
                ctx->npcs[slot]->base.size = ctx->npcs[slot]->type.size;
                ctx->npcs[slot]->base.seq_walk_id = ctx->npcs[slot]->type.walkanim;
                ctx->npcs[slot]->base.seq_turn_around_id = ctx->npcs[slot]->type.walkanim_b;
                ctx->npcs[slot]->base.seq_turn_left_id = ctx->npcs[slot]->type.walkanim_r;
                ctx->npcs[slot]->base.seq_turn_right_id = ctx->npcs[slot]->type.walkanim_l;
                ctx->npcs[slot]->base.seq_stand_id = ctx->npcs[slot]->type.readyanim;
            }
        }

        int dx = gbit(pkt, 5);
        if (dx > 15) dx -= 32;
        int dz = gbit(pkt, 5);
        if (dz > 15) dz -= 32;

        pathing_entity_teleport(&ctx->npcs[slot]->base, false, false,
                               ctx->local_player.base.path_tile_x[0] + dx,
                               ctx->local_player.base.path_tile_z[0] + dz);

        int ext = gbit(pkt, 1);
        if (ext == 1) {
            extended_update_ids[extended_update_count++] = slot;
        }
    }

    access_bytes(pkt);

    // 3. NPC Extended Updates
    for (int i = 0; i < extended_update_count; i++) {
        int slot = extended_update_ids[i];
        NpcEntity *npc = ctx->npcs[slot];
        if (!npc) continue;

        int mask = g1(pkt);
        if (mask & 0x2) {
            int seq = g2(pkt);
            if (seq == 65535) seq = -1;
            int delay = g1(pkt);
            if(!pathing_entity_set_sequence(&npc->base,ctx->seq_cfg,seq,delay)) return false;
        }
        if (mask & 0x4) {
            npc->base.target_id = g2(pkt);
            if (npc->base.target_id == 65535) npc->base.target_id = -1;
        }
        if (mask & 0x8) {
            gjstr(pkt, npc->base.chat, sizeof(npc->base.chat));
            npc->base.chat_timer = 100;
        }
        if (mask & 0x10) {
            npc->base.damage = g1(pkt);
            npc->base.damage_type = g1(pkt);
            npc->base.health = g1(pkt);
            npc->base.total_health = g1(pkt);
            npc->base.combat_cycle=ctx->cycle+400;
            if(ctx->trace)printf("[HIT] npc damage=%d type=%d health=%d/%d until=%d\n",npc->base.damage,npc->base.damage_type,npc->base.health,npc->base.total_health,npc->base.combat_cycle);
        }
        if (mask & 0x20) {
            int new_type = g2(pkt);
            if (ctx->npc_cfg) {
                if (npc->has_type) {
                    npc_free_entry(&npc->type);
                    npc->has_type=false;
                }
                if (npc_get(ctx->npc_cfg, new_type, &npc->type)) {
                    npc->has_type = true;
                    npc->base.size = npc->type.size;
                }
            }
        }
        if (mask & 0x40) {
            npc->base.spotanim_id=g2(pkt);uint32_t info=(uint32_t)g4(pkt);
            npc->base.spotanim_height=info>>16;npc->base.spotanim_delay=ctx->cycle+(info&65535);
            npc->base.spotanim_frame=npc->base.spotanim_delay>ctx->cycle?-1:0;npc->base.spotanim_cycle=0;
            if(npc->base.spotanim_id==65535) npc->base.spotanim_id=-1;
        }
        if(mask & 0x80) {npc->base.target_tile_x=g2(pkt);npc->base.target_tile_z=g2(pkt);}
        if(pkt->error) return false;
    }

    if(pkt->error || pkt->pos!=packet_size) return false;
    for(int i=0;i<ctx->npc_count;i++) {
        int slot=ctx->npc_indices[i];
        if(!seen[slot] && ctx->npcs[slot]) {npc_entity_free(ctx->npcs[slot]);free(ctx->npcs[slot]);ctx->npcs[slot]=NULL;}
    }
    memcpy(ctx->npc_indices,new_npc_indices,new_npc_count*sizeof(int));ctx->npc_count=new_npc_count;
    return true;
}

void entity_sync_cleanup(EntitySyncContext *ctx) {
    if (!ctx) return;
    player_entity_dispose(&ctx->local_player);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (ctx->players[i]) {
            player_entity_dispose(ctx->players[i]);
            free(ctx->players[i]);
            ctx->players[i] = NULL;
        }
    }
    for (int i = 0; i < MAX_NPCS; i++) {
        if (ctx->npcs[i]) {
            npc_entity_free(ctx->npcs[i]);
            free(ctx->npcs[i]);
            ctx->npcs[i] = NULL;
        }
    }
}
