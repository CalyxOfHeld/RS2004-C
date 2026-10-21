#include "ui/mouse_picking.h"
#include <stdio.h>
#include <string.h>

void mouse_picker_init(MousePicker *picker) {
    if (!picker) return;
    picker->check_hover = false;
    picker->mouse_x = -1;
    picker->mouse_y = -1;
    picker->picked_count = 0;
}

void mouse_picker_reset(MousePicker *picker, int mouse_x, int mouse_y) {
    if (!picker) return;
    picker->check_hover = true;
    picker->mouse_x = mouse_x;
    picker->mouse_y = mouse_y;
    picker->picked_count = 0;
}

void mouse_picker_add_picked(MousePicker *picker, uint32_t bitset) {
    if (!picker || picker->picked_count >= MAX_PICKED) return;
    // Avoid duplicate bitsets
    for (int i = 0; i < picker->picked_count; i++) {
        if (picker->picked_bitsets[i] == bitset) return;
    }
    picker->picked_bitsets[picker->picked_count++] = bitset;
}

uint32_t mouse_picker_make_bitset(int type, int id, int tile_x, int tile_z) {
    return ((uint32_t)(type & 0x3) << 29) |
           ((uint32_t)(id & 0x7FFF) << 14) |
           ((uint32_t)(tile_z & 0x7F) << 7) |
           (uint32_t)(tile_x & 0x7F);
}

static const char* get_combat_level_color(int player_combat, int npc_combat) {
    int diff = npc_combat - player_combat;
    if (diff < -9) return "@gre@";
    if (diff < -6) return "@gr1@";
    if (diff < -3) return "@gr2@";
    if (diff < 0)  return "@gr3@";
    if (diff > 9)  return "@red@";
    if (diff > 6)  return "@or3@";
    if (diff > 3)  return "@or2@";
    if (diff > 0)  return "@or1@";
    return "@yel@";
}

static void build_menu(MousePicker *picker,
                             ContextMenu *menu,
                             NpcConfig *npc_cfg,
                             LocConfig *loc_cfg,
                             ObjConfig *obj_cfg,
                             NpcEntity *npcs, int npc_count,
                             PlayerEntity *players, int player_count,
                             int local_player_combat, EntitySyncContext *sync) {
    if (!menu) return;
    context_menu_clear(menu);

    // Cancel option is always at the bottom (index 0)
    context_menu_add_action(menu,"Cancel",(MenuAction){.type=MENU_CANCEL});

    // "Walk here" is the base world action (index 1)
    context_menu_add_action(menu,"Walk here",(MenuAction){.type=MENU_WALK});

    // If no picker or no entities picked, we're done
    if (!picker || picker->picked_count == 0) {
        return;
    }

    // Process picked bitsets in reverse (topmost drawn has highest priority)
    for (int i = picker->picked_count - 1; i >= 0; i--) {
        uint32_t bitset = picker->picked_bitsets[i];
        int type = (bitset >> 29) & 0x3;
        int id   = (bitset >> 14) & 0x7FFF;

        if (type == 1) { // NPC
            NpcType npc_type;
            bool found = false;

            if(sync && id>=0 && id<MAX_NPCS && sync->npcs[id] && sync->npcs[id]->has_type) {
                npc_type=sync->npcs[id]->type;found=true;
            } else if (npcs && id >= 0 && id < npc_count && npcs[id].has_type) {
                npc_type = npcs[id].type;
                found = true;
            } else if (!sync && npc_cfg && npc_get(npc_cfg, id, &npc_type)) {
                found = true;
            }

            if (found) {
                char full_name[128];
                if (npc_type.vislevel > 0) {
                    snprintf(full_name, sizeof(full_name), "%s %s(level-%d)",
                             npc_type.name, get_combat_level_color(local_player_combat, npc_type.vislevel), npc_type.vislevel);
                } else {
                    snprintf(full_name, sizeof(full_name), "%s", npc_type.name);
                }

                // Examine option
                char examine_str[160];
                snprintf(examine_str, sizeof(examine_str), "Examine @yel@%s", full_name);
                context_menu_add_action(menu,examine_str,(MenuAction){.type=MENU_NPC_EXAMINE,.target=id});

                // NPC ops in reverse order (4 down to 0)
                for(int pass=0;pass<2;pass++) for (int o = 4; o >= 0; o--) {
                    bool attack=strcmp(npc_type.op[o],"Attack")==0;
                    if(attack!=(pass==1)) continue;
                    if (npc_type.op_valid[o] && npc_type.op[o][0] != '\0') {
                        char op_str[160];
                        snprintf(op_str, sizeof(op_str), "%s @yel@%s", npc_type.op[o], full_name);
                        context_menu_add_action(menu,op_str,(MenuAction){.type=MENU_NPC_OP,.operation=o+1,.target=id,.low_priority=strcmp(npc_type.op[o],"Attack")==0 && npc_type.vislevel>local_player_combat});
                    }
                }
            }
        } else if (type == 2) { // Scenery Loc
            if (loc_cfg) {
                LocType lt;
                if (loc_get(loc_cfg, id, &lt)) {
                    if (lt.name[0] != '\0') {
                        char examine_str[160];
                        snprintf(examine_str, sizeof(examine_str), "Examine @cya@%s", lt.name);
                        context_menu_add_action(menu,examine_str,(MenuAction){.type=MENU_LOC_EXAMINE,.target=id});

                        for (int o = 4; o >= 0; o--) {
                            if (lt.op_valid[o] && lt.op[o][0] != '\0') {
                                char op_str[160];
                                snprintf(op_str, sizeof(op_str), "%s @cya@%s", lt.op[o], lt.name);
                                context_menu_add_action(menu,op_str,(MenuAction){.type=MENU_LOC_OP,.operation=o+1,.target=id,.x=(int)(bitset&127),.z=(int)((bitset>>7)&127)});
                            }
                        }
                    }
                    loc_free_entry(&lt);
                }
            }
        } else if (type == 0) { // Player
            PlayerEntity *target=sync && id>=0 && id<MAX_PLAYERS ? sync->players[id] :
                players && id>=0 && id<player_count ? &players[id] : NULL;
            if (target && target->visible) {
                char p_name[128];
                snprintf(p_name, sizeof(p_name), "%s %s(level-%d)",
                         target->name,
                         get_combat_level_color(local_player_combat, target->combat_level),
                         target->combat_level);

                char ex_str[160];
                snprintf(ex_str, sizeof(ex_str), "Follow @whi@%s", p_name);
                context_menu_add_action(menu,ex_str,(MenuAction){.type=MENU_PLAYER_OP,.operation=3,.target=id});

                snprintf(ex_str, sizeof(ex_str), "Trade with @whi@%s", p_name);
                context_menu_add_action(menu,ex_str,(MenuAction){.type=MENU_PLAYER_OP,.operation=4,.target=id});
            }
        } else if (type == 3) { // Ground Item
            if (obj_cfg) {
                ObjType ot;
                if (obj_get(obj_cfg, id, &ot)) {
                    if (ot.name[0] != '\0') {
                        char ex_str[160];
                        snprintf(ex_str, sizeof(ex_str), "Examine @lre@%s", ot.name);
                        context_menu_add_action(menu,ex_str,(MenuAction){.type=MENU_OBJ_EXAMINE,.target=id});

                        for(int op=4;op>=0;op--) {
                            const char *label=ot.op_valid[op]?ot.op[op]:op==2?"Take":NULL;
                            if(!label || !label[0]) continue;
                            snprintf(ex_str,sizeof(ex_str),"%s @lre@%s",label,ot.name);
                            context_menu_add_action(menu,ex_str,(MenuAction){.type=MENU_OBJ_OP,.operation=op+1,.target=id,.x=(int)(bitset&127),.z=(int)((bitset>>7)&127)});
                        }
                    }
                    obj_free_entry(&ot);
                }
            }
        }
    }
    context_menu_prioritize(menu);
}

void mouse_picker_build_menu(MousePicker *picker,ContextMenu *menu,NpcConfig *npc_cfg,LocConfig *loc_cfg,ObjConfig *obj_cfg,
    NpcEntity *npcs,int npc_count,PlayerEntity *players,int player_count,int local_player_combat) {
    build_menu(picker,menu,npc_cfg,loc_cfg,obj_cfg,npcs,npc_count,players,player_count,local_player_combat,NULL);
}
void mouse_picker_build_live_menu(MousePicker *picker,ContextMenu *menu,NpcConfig *npc_cfg,LocConfig *loc_cfg,ObjConfig *obj_cfg,EntitySyncContext *sync) {
    build_menu(picker,menu,npc_cfg,loc_cfg,obj_cfg,NULL,0,NULL,0,sync->local_player.combat_level,sync);
}

const char* mouse_picker_get_primary_action(ContextMenu *menu) {
    if (!menu || menu->size <= 0) return "@whi@Walk here";
    // The top-left primary action is the last added option before Cancel
    // menu->options[menu->size - 1] is the highest priority action
    return menu->options[menu->size - 1];
}
