from pathlib import Path
p=Path('include/gfx/model.h');s=p.read_text().replace('typedef struct {','typedef struct {\n    Arena *allocation_arena;',1);p.write_text(s)
p=Path('src/gfx/model.c');s=p.read_text().replace('    m->num_points = meta->num_points;','    m->allocation_arena=arena;\n    m->num_points = meta->num_points;').replace('    dst->num_points = src->num_points;','    dst->allocation_arena=arena;\n    dst->num_points = src->num_points;').replace('    memset(m, 0, sizeof(Model));\n\n    m->point_x', '    memset(m, 0, sizeof(Model));\n    m->allocation_arena=arena;\n\n    m->point_x')
for suffix in ['a','b','c']:
    s=s.replace(f'if(!m->face_colour_{suffix}) m->face_colour_{suffix} = (int32_t *)malloc(m->num_faces * sizeof(int32_t));', f'if(!m->face_colour_{suffix}) m->face_colour_{suffix} = m->allocation_arena ? arena_alloc(m->allocation_arena,m->num_faces*sizeof(int32_t)) : malloc(m->num_faces*sizeof(int32_t));')
p.write_text(s)
p=Path('src/ui/mouse_picking.c');s=p.read_text();s=s.replace('void mouse_picker_build_menu(MousePicker *picker,','static void build_menu(MousePicker *picker,',1).replace('                             int local_player_combat) {','                             int local_player_combat, EntitySyncContext *sync) {',1);s=s.replace('''            if (npcs && id >= 0 && id < npc_count && npcs[id].has_type) {''','''            if(sync && id>=0 && id<MAX_NPCS && sync->npcs[id] && sync->npcs[id]->has_type) {
                npc_type=sync->npcs[id]->type;found=true;
            } else if (npcs && id >= 0 && id < npc_count && npcs[id].has_type) {''').replace('} else if (npc_cfg && npc_get(npc_cfg, id, &npc_type)) {','} else if (!sync && npc_cfg && npc_get(npc_cfg, id, &npc_type)) {');s=s.replace('''            if (players && id >= 0 && id < player_count && players[id].visible) {''','''            PlayerEntity *target=sync && id>=0 && id<MAX_PLAYERS ? sync->players[id] :
                players && id>=0 && id<player_count ? &players[id] : NULL;
            if (target && target->visible) {''').replace('players[id].name','target->name').replace('players[id].combat_level','target->combat_level');idx=s.index('const char* mouse_picker_get_primary_action');s=s[:idx]+'''void mouse_picker_build_menu(MousePicker *picker,ContextMenu *menu,NpcConfig *npc_cfg,LocConfig *loc_cfg,ObjConfig *obj_cfg,
    NpcEntity *npcs,int npc_count,PlayerEntity *players,int player_count,int local_player_combat) {
    build_menu(picker,menu,npc_cfg,loc_cfg,obj_cfg,npcs,npc_count,players,player_count,local_player_combat,NULL);
}
void mouse_picker_build_live_menu(MousePicker *picker,ContextMenu *menu,NpcConfig *npc_cfg,LocConfig *loc_cfg,ObjConfig *obj_cfg,EntitySyncContext *sync) {
    build_menu(picker,menu,npc_cfg,loc_cfg,obj_cfg,NULL,0,NULL,0,sync->local_player.combat_level,sync);
}

'''+s[idx:];p.write_text(s)
p=Path('include/ui/mouse_picking.h');s=p.read_text().replace('#include "entity/player_entity.h"','#include "entity/player_entity.h"\n#include "net/entity_sync.h"').replace('const char* mouse_picker_get_primary_action','void mouse_picker_build_live_menu(MousePicker *picker,ContextMenu *menu,NpcConfig *npc_cfg,LocConfig *loc_cfg,ObjConfig *obj_cfg,EntitySyncContext *sync);\n\nconst char* mouse_picker_get_primary_action');p.write_text(s)
p=Path('platform/sdl2_main.c');s=p.read_text();idx=s.index('#include "verification_input.h"');s=s[:idx]+'''static int current_entity_frame(PathingEntity *entity,SeqConfig *sequences) {
    int id=entity->secondary_seq_id,frame=entity->secondary_seq_frame;
    if(entity->primary_seq_id>=0 && entity->primary_seq_delay==0) {id=entity->primary_seq_id;frame=entity->primary_seq_frame;}
    if(id<0 || id>=sequences->count) return -1;
    SeqType *seq=&sequences->instances[id];
    return frame>=0 && frame<seq->frame_count ? seq->frames[frame] : -1;
}

'''+s[idx:]
# Render only server-present entities in live mode; each temporary model's arena space is reclaimed after submission.
idx=s.index('        // Render Player (authentic');s=s[:idx]+'''        if(live_server) {
            for(int index=0;index<sync_ctx.npc_count;index++) {
                int slot=sync_ctx.npc_indices[index];NpcEntity *npc=sync_ctx.npcs[slot];
                if(!npc || !npc->has_type) continue;
                size_t mark=arena_frame.offset;Model *model=npc_entity_get_model(npc,&arena_frame);
                if(model) {
                    model_create_label_references(model,&arena_frame);
                    int frame=current_entity_frame(&npc->base,&seq_cfg);
                    if(frame>=0) model_apply_transform(model,frame);
                    model_calculate_normals(model,64,850,-30,-50,-30);
                    int y=camera_get_height(sync_ctx.current_plane,npc->base.x,npc->base.z,world3d->heightmap,world->tile_flags);
                    if(model_draw_pickable(model,npc->base.yaw,camera_pitch,camera_yaw,npc->base.x-eye_x,y-eye_y,npc->base.z-eye_z,mouse_x,mouse_y))
                        mouse_picker_add_picked(&picker,mouse_picker_make_bitset(1,slot,npc->base.x/128,npc->base.z/128));
                }
                arena_frame.offset=mark;
            }
            for(int index=0;index<sync_ctx.player_count;index++) {
                int slot=sync_ctx.player_indices[index];PlayerEntity *other=sync_ctx.players[slot];
                if(!other || !other->visible) continue;
                size_t mark=arena_frame.offset;Model *model=player_entity_get_model(other,&idk_cfg,&obj_cfg,&seq_cfg,&arena_frame);
                if(model) {
                    model_create_label_references(model,&arena_frame);int frame=current_entity_frame(&other->base,&seq_cfg);
                    if(frame>=0) model_apply_transform(model,frame);
                    model_calculate_normals(model,64,850,-30,-50,-30);
                    int y=camera_get_height(sync_ctx.current_plane,other->base.x,other->base.z,world3d->heightmap,world->tile_flags);
                    if(model_draw_pickable(model,other->base.yaw,camera_pitch,camera_yaw,other->base.x-eye_x,y-eye_y,other->base.z-eye_z,mouse_x,mouse_y))
                        mouse_picker_add_picked(&picker,mouse_picker_make_bitset(0,slot,other->base.x/128,other->base.z/128));
                }
                arena_frame.offset=mark;
            }
        }

'''+s[idx:];s=s.replace('''        if (player_rest_model) {
            Model *p_model''','''        if (player.visible && player_rest_model) {
            Model *p_model''');s=s.replace('''            pathing_entity_update_movement(&hans.base);''','''            if(live_server) {
                for(int i=0;i<sync_ctx.npc_count;i++) {
                    NpcEntity *npc=sync_ctx.npcs[sync_ctx.npc_indices[i]];if(!npc) continue;
                    pathing_entity_update_movement(&npc->base);pathing_entity_update_facing(&npc->base);pathing_entity_update_sequences(&npc->base,&seq_cfg);
                }
                for(int i=0;i<sync_ctx.player_count;i++) {
                    PlayerEntity *other=sync_ctx.players[sync_ctx.player_indices[i]];if(!other) continue;
                    pathing_entity_update_movement(&other->base);pathing_entity_update_facing(&other->base);pathing_entity_update_sequences(&other->base,&seq_cfg);
                }
            }
            pathing_entity_update_movement(&hans.base);''');s=s.replace('''            mouse_picker_build_menu(&picker,&menu,&npc_cfg,&loc_cfg,&obj_cfg,offline?&hans:NULL,offline?1:0,&player,1,player.combat_level);''','''            if(live_server) mouse_picker_build_live_menu(&picker,&menu,&npc_cfg,&loc_cfg,&obj_cfg,&sync_ctx);
            else if(offline) mouse_picker_build_menu(&picker,&menu,&npc_cfg,&loc_cfg,&obj_cfg,&hans,1,&player,1,player.combat_level);
            else context_menu_add_action(&menu,"Walk here",(MenuAction){.type=MENU_CANCEL});''');s=s.replace('''        printf("[PASS] Connected to LIVE SERVER! (is_mod=%d)\\n", session.is_mod);''','''        player.visible=false;
        printf("[PASS] Connected to LIVE SERVER! (is_mod=%d)\\n", session.is_mod);''')
s=s.replace('''                if(offline) minimap_draw_dot''','''                if(live_server) for(int i=0;i<sync_ctx.npc_count;i++) {
                    NpcEntity *npc=sync_ctx.npcs[sync_ctx.npc_indices[i]];
                    if(npc) minimap_draw_dot(player.base.x,player.base.z,npc->base.x,npc->base.z,camera_yaw,561,5,media.mapdots[1]);
                }
                if(offline) minimap_draw_dot''')
s=s.replace('''            printf("[VERIFY] player tile=(%d,%d) sequence=%d frame=%d\\n", player.base.x / 128,
                player.base.z / 128, player.base.secondary_seq_id, player.base.secondary_seq_frame);''','''            printf("[VERIFY] player tile=(%d,%d) sequence=%d frame=%d\\n", player.base.x / 128,
                player.base.z / 128, player.base.secondary_seq_id, player.base.secondary_seq_frame);
            if(trace) printf("[STATE] live=%d plane=%d npcs=%d players=%d appearance_weapon=%d hash=%llu\\n",live_server,sync_ctx.current_plane,
                sync_ctx.npc_count,sync_ctx.player_count,player.appearances[3],(unsigned long long)player.appearance_hashcode);''')
# Use the synchronized plane consistently, not hard-coded ground level.
s=s.replace('world3d_render(world3d, eye_x, eye_y, eye_z, camera_pitch, camera_yaw, 0, 0);','world3d_render(world3d,eye_x,eye_y,eye_z,camera_pitch,camera_yaw,0,sync_ctx.current_plane);')
s=s.replace('camera_get_height(0, player.base.x, player.base.z,','camera_get_height(sync_ctx.current_plane, player.base.x, player.base.z,').replace('camera_update(&cam, player.base.x, player.base.z, 0,','camera_update(&cam, player.base.x, player.base.z, sync_ctx.current_plane,')
# Cleanup active network-owned definitions at shutdown.
s=s.replace('    protocol_close_session(&session);\n    client_stream_global_cleanup();','    protocol_close_session(&session);\n    entity_sync_cleanup(&sync_ctx);\n    client_stream_global_cleanup();');p.write_text(s)
