#ifndef RS2_CONTEXT_MENU_H
#define RS2_CONTEXT_MENU_H

#include <stdint.h>
#include <stdbool.h>
#include "gfx/pixfont.h"

#define MAX_MENU_OPTIONS 64

typedef enum {
    MENU_LEGACY, MENU_CANCEL, MENU_WALK, MENU_IF_BUTTON, MENU_IF_CLOSE,
    MENU_IF_CONTINUE, MENU_HELD_OP, MENU_INV_BUTTON, MENU_ITEM_SELECT,
    MENU_SPELL_SELECT, MENU_ITEM_ON_ITEM, MENU_ITEM_EXAMINE, MENU_NPC_OP,
    MENU_NPC_EXAMINE, MENU_LOC_OP, MENU_LOC_EXAMINE, MENU_OBJ_OP,
    MENU_OBJ_EXAMINE, MENU_PLAYER_OP,
    MENU_ITEM_ON_NPC, MENU_ITEM_ON_LOC, MENU_ITEM_ON_OBJ, MENU_ITEM_ON_PLAYER,
    MENU_SPELL_ON_NPC, MENU_SPELL_ON_LOC, MENU_SPELL_ON_OBJ, MENU_SPELL_ON_PLAYER,
    MENU_SPELL_ON_ITEM
} MenuActionType;
typedef struct {
    MenuActionType type;
    int operation, target, component, slot, x, z;
    bool low_priority;
} MenuAction;
typedef struct MenuSelection {
    bool item_selected;
    int item, slot, component;
    char name[64];
    bool spell_selected;
    int spell_component, spell_flags;
    char spell_caption[128];
} MenuSelection;

typedef struct ContextMenu {
    bool visible;
    int x;
    int y;
    int width;
    int height;
    int size;
    char options[MAX_MENU_OPTIONS][64];
    int action_ids[MAX_MENU_OPTIONS]; /* compatibility with old fixture tests only */
    MenuAction actions[MAX_MENU_OPTIONS];
} ContextMenu;

void context_menu_init(ContextMenu *menu);
void context_menu_clear(ContextMenu *menu);
void context_menu_add_option(ContextMenu *menu, const char *option, int action_id);
void context_menu_add_action(ContextMenu *menu, const char *option, MenuAction action);
bool context_menu_pick_action(ContextMenu *menu,int x,int y,MenuAction *action);
void context_menu_prioritize(ContextMenu *menu);
void context_menu_open(ContextMenu *menu, int click_x, int click_y, PixFont *font);
void context_menu_draw(ContextMenu *menu, int mouse_x, int mouse_y, PixFont *font);
int  context_menu_handle_click(ContextMenu *menu, int mouse_x, int mouse_y);

#endif // RS2_CONTEXT_MENU_H
