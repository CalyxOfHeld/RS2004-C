#ifndef RS2_MOUSE_PICKING_H
#define RS2_MOUSE_PICKING_H

#include <stdint.h>
#include <stdbool.h>
#include "config/npc.h"
#include "config/loc.h"
#include "config/obj.h"
#include "ui/context_menu.h"
#include "entity/npc_entity.h"
#include "entity/player_entity.h"
#include "net/entity_sync.h"

#define MAX_PICKED 1000

typedef struct MousePicker {
    bool check_hover;
    int mouse_x;
    int mouse_y;
    int picked_count;
    uint32_t picked_bitsets[MAX_PICKED];
    const MenuSelection *selection;
    int world_x225;
    int world_z225;
    /* Original handleViewportOptions checks scene.getInfo on currentLevel. */
    bool (*location_present225)(void *user,uint32_t bitset);
    void *location_user225;
} MousePicker;

void mouse_picker_init(MousePicker *picker);
void mouse_picker_reset(MousePicker *picker, int mouse_x, int mouse_y);
void mouse_picker_add_picked(MousePicker *picker, uint32_t bitset);

uint32_t mouse_picker_make_bitset(int type, int id, int tile_x, int tile_z);

void mouse_picker_build_menu(MousePicker *picker,
                             ContextMenu *menu,
                             NpcConfig *npc_cfg,
                             LocConfig *loc_cfg,
                             ObjConfig *obj_cfg,
                             NpcEntity *npcs, int npc_count,
                             PlayerEntity *players, int player_count,
                             int local_player_combat);

void mouse_picker_build_live_menu(MousePicker *picker,ContextMenu *menu,NpcConfig *npc_cfg,LocConfig *loc_cfg,ObjConfig *obj_cfg,EntitySyncContext *sync);

void mouse_picker_build_live_menu225(MousePicker *picker,ContextMenu *menu,NpcConfig *npc_cfg,LocConfig *loc_cfg,ObjConfig *obj_cfg,EntitySyncContext *sync,bool suppress_trade);
/* client.handleInput adds private-chat options before viewport options. */
void mouse_picker_append_live_menu225(MousePicker *picker,ContextMenu *menu,NpcConfig *npc_cfg,LocConfig *loc_cfg,ObjConfig *obj_cfg,EntitySyncContext *sync,bool suppress_trade);

const char* mouse_picker_get_primary_action(ContextMenu *menu);

#endif // RS2_MOUSE_PICKING_H
