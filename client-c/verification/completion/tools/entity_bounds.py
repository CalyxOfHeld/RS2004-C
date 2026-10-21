from pathlib import Path
p=Path('include/net/entity_sync.h');s=p.read_text().replace('void entity_sync_player_info','bool entity_sync_player_info').replace('void entity_sync_npc_info','bool entity_sync_npc_info');p.write_text(s)
p=Path('include/entity/pathing_entity.h');s=p.read_text().replace('    int cycle;','''    int spotanim_id,spotanim_height,spotanim_delay;
    int exact_start_x,exact_start_z,exact_end_x,exact_end_z,exact_start_cycle,exact_end_cycle,exact_facing;
    int cycle;''');p.write_text(s)
p=Path('src/entity/pathing_entity.c');s=p.read_text().replace('    e->primary_seq_id = -1;','    e->spotanim_id = -1;\n    e->primary_seq_id = -1;',1);p.write_text(s)
p=Path('src/net/entity_sync.c');s=p.read_text().replace('void entity_sync_player_info','bool entity_sync_player_info').replace('void entity_sync_npc_info','bool entity_sync_npc_info');s=s.replace('    if (!ctx || !pkt) return;', '    if (!ctx || !pkt || packet_size>pkt->length) return false;\n    pkt->length=packet_size;',2)
s=s.replace('    int new_player_count = 0;', '    int new_player_count = 0;\n    bool seen[MAX_PLAYERS]={false};\n    if(pkt->error || old_count>ctx->player_count) return false;').replace('    int new_npc_count = 0;', '    int new_npc_count = 0;\n    bool seen[MAX_NPCS]={false};\n    if(pkt->error || old_count>ctx->npc_count) return false;')
# Preserve player appearance cache when a player temporarily leaves view.
s=s.replace('''                if (ctx->players[slot]) {
                    free(ctx->players[slot]);
                    ctx->players[slot] = NULL;
                }''','''                if (ctx->players[slot]) ctx->players[slot]->visible=false;''')
s=s.replace('''    // 3. New Players Vis
    while (pkt->bit_pos + 11 < packet_size * 8) {''','''    for(int i=0;i<new_player_count;i++) seen[new_player_indices[i]]=true;
    // Read complete indices; the server may omit a sentinel when no masks follow.
    while (!pkt->error && pkt->bit_pos + 11 <= packet_size * 8) {''')
s=s.replace('''        if (slot == 2047) break;

        if (!ctx->players[slot])''','''        if (slot == 2047) break;
        if(pkt->bit_pos+12>packet_size*8 || new_player_count>=MAX_PLAYERS || seen[slot]) return false;
        seen[slot]=true;
        if (!ctx->players[slot])''')
s=s.replace('''            player_entity_init(ctx->players[slot]);
        }
        new_player_indices''','''            if(!ctx->players[slot]) return false;
            player_entity_init(ctx->players[slot]);
        }
        if(ctx->players[slot]->name[0]) ctx->players[slot]->visible=true;
        new_player_indices''')
s=s.replace('''            size_t end_pos = pkt->pos + len;
            player_entity_read_appearance(p, pkt);
            pkt->pos = end_pos;''','''            if(pkt->pos>packet_size || (size_t)len>packet_size-pkt->pos) return false;
            Packet appearance=packet_wrap(pkt->data+pkt->pos,len);
            player_entity_read_appearance(p,&appearance);
            if(appearance.error || appearance.pos!=(size_t)len) return false;
            pkt->pos+=(size_t)len;''')
s=s.replace('''            int text_len = g1(pkt);
            pkt->pos += text_len;''','''            int text_len = g1(pkt);
            if(pkt->pos>packet_size || (size_t)text_len>packet_size-pkt->pos) return false;
            pkt->pos += text_len;''')
needle='''        if (mask & 0x40) {
            g2(pkt);''';# Add additional player masks just before player mask loop closes.
start=s.index('    // Update active player list');end=s.rfind('    }',0,start)
s=s[:end]+'''        if(mask & 0x100) {
            p->base.spotanim_id=g2(pkt);uint32_t info=(uint32_t)g4(pkt);
            p->base.spotanim_height=info>>16;p->base.spotanim_delay=info&65535;
            if(p->base.spotanim_id==65535) p->base.spotanim_id=-1;
        }
        if(mask & 0x200) {
            p->base.exact_start_x=g1(pkt);p->base.exact_start_z=g1(pkt);
            p->base.exact_end_x=g1(pkt);p->base.exact_end_z=g1(pkt);
            p->base.exact_end_cycle=g2(pkt);p->base.exact_start_cycle=g2(pkt);p->base.exact_facing=g1(pkt);
        }
        if(pkt->error) return false;
'''+s[end:]
s=s.replace('''    ctx->player_count = new_player_count;
}''','''    if(pkt->error || pkt->pos!=packet_size) return false;
    for(int i=0;i<ctx->player_count;i++) if(!seen[ctx->player_indices[i]] && ctx->players[ctx->player_indices[i]]) ctx->players[ctx->player_indices[i]]->visible=false;
    ctx->player_count = new_player_count;
    return true;
}''') # old indices memcpy happens before seen loop -> fix below
s=s.replace('''    memcpy(ctx->player_indices, new_player_indices, new_player_count * sizeof(int));
    if(pkt->error''','''    if(pkt->error''').replace('''    ctx->player_count = new_player_count;
    return true;''','''    memcpy(ctx->player_indices,new_player_indices,new_player_count*sizeof(int));
    ctx->player_count = new_player_count;
    return true;''')
s=s.replace('''    // 2. New NPCs Vis
    while (pkt->bit_pos + 21 < packet_size * 8) {''','''    for(int i=0;i<new_npc_count;i++) seen[new_npc_indices[i]]=true;
    // 2. New NPCs Vis
    while (!pkt->error && pkt->bit_pos + 13 <= packet_size * 8) {''')
s=s.replace('''        if (slot == 8191) break;

        if (!ctx->npcs[slot])''','''        if (slot == 8191) break;
        if(pkt->bit_pos+22>packet_size*8 || new_npc_count>=MAX_NPCS || seen[slot]) return false;
        seen[slot]=true;
        if (!ctx->npcs[slot])''')
s=s.replace('''            npc_entity_init(ctx->npcs[slot]);''','''            if(!ctx->npcs[slot]) return false;
            npc_entity_init(ctx->npcs[slot]);''')
s=s.replace('''        if (mask & 0x40) {
            g2(pkt); // spotanim id
            g4(pkt); // spotanim height/delay
        }''','''        if (mask & 0x40) {
            npc->base.spotanim_id=g2(pkt);uint32_t info=(uint32_t)g4(pkt);
            npc->base.spotanim_height=info>>16;npc->base.spotanim_delay=info&65535;
            if(npc->base.spotanim_id==65535) npc->base.spotanim_id=-1;
        }
        if(mask & 0x80) {npc->base.target_tile_x=g2(pkt);npc->base.target_tile_z=g2(pkt);}
        if(pkt->error) return false;''')
s=s.replace('''    memcpy(ctx->npc_indices, new_npc_indices, new_npc_count * sizeof(int));
    ctx->npc_count = new_npc_count;
}''','''    if(pkt->error || pkt->pos!=packet_size) return false;
    for(int i=0;i<ctx->npc_count;i++) {
        int slot=ctx->npc_indices[i];
        if(!seen[slot] && ctx->npcs[slot]) {npc_entity_free(ctx->npcs[slot]);free(ctx->npcs[slot]);ctx->npcs[slot]=NULL;}
    }
    memcpy(ctx->npc_indices,new_npc_indices,new_npc_count*sizeof(int));ctx->npc_count=new_npc_count;
    return true;
}''')
# Reset type-validity before loading a replacement definition.
s=s.replace('''                npc_free_entry(&ctx->npcs[slot]->type);
            }''','''                npc_free_entry(&ctx->npcs[slot]->type);
                ctx->npcs[slot]->has_type=false;
            }''').replace('''                    npc_free_entry(&npc->type);
                }''','''                    npc_free_entry(&npc->type);
                    npc->has_type=false;
                }''');p.write_text(s)
p=Path('platform/sdl2_main.c');s=p.read_text().replace('opcode == 134) { // NPC_INFO','opcode == 1) { // NPC_INFO');s=s.replace('''                        entity_sync_player_info(&sync_ctx, &pinfo, pkt_len);''','''                        if(!entity_sync_player_info(&sync_ctx,&pinfo,pkt_len)) {
                            fprintf(stderr,"[Protocol] Invalid PLAYER_INFO\\n");protocol_close_session(&session);live_server=false;break;
                        }''').replace('''                        entity_sync_npc_info(&sync_ctx, &ninfo, pkt_len);''','''                        if(!entity_sync_npc_info(&sync_ctx,&ninfo,pkt_len)) {
                            fprintf(stderr,"[Protocol] Invalid NPC_INFO\\n");protocol_close_session(&session);live_server=false;break;
                        }''');p.write_text(s)
