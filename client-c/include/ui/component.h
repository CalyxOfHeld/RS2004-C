#ifndef RS2_COMPONENT_H
#define RS2_COMPONENT_H

#include <stdint.h>
#include <stdbool.h>
#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/pix8.h"
#include "gfx/pix32.h"
#include "gfx/pixfont.h"
#include "gfx/model.h"

#define COMPONENT_TYPE_CONTAINER   0
#define COMPONENT_TYPE_MODEL_UI    1
#define COMPONENT_TYPE_INV_GRID    2
#define COMPONENT_TYPE_BOX         3
#define COMPONENT_TYPE_TEXT        4
#define COMPONENT_TYPE_SPRITE      5
#define COMPONENT_TYPE_MODEL_3D    6
#define COMPONENT_TYPE_TEXT_LIST   7

typedef struct Component {
    int id;
    int layer;
    int type;
    int button_type;
    int client_code;
    int width;
    int height;
    int x;
    int y;
    int over_layer;

    // Conditions / scripts
    int script_comparator_count;
    int *script_comparator;
    int *script_operand;

    int script_count;
    int *script_lengths;
    int **scripts;

    // Type 0: Container
    int scroll;
    int scroll_position;
    bool hide;
    int child_count;
    int *child_id;
    int *child_x;
    int *child_y;

    // Type 1: Model UI
    int unused_short1;
    bool unused_bool1;

    // Type 2 & 7: Inventory / Items
    int *inv_slot_obj_id;
    int *inv_slot_obj_count;
    bool draggable;
    bool interactable;
    bool usable;
    int margin_x;
    int margin_y;
    int inv_slot_offset_x[20];
    int inv_slot_offset_y[20];
    Pix32 *inv_slot_sprite[20];
    char *iops[5];

    // Type 3: Box
    bool fill;

    // Type 4: Text
    bool center;
    int font_id;
    PixFont *font;
    bool shadowed;
    char *text;
    char *active_text;

    // Colours
    uint32_t colour;
    uint32_t active_colour;
    uint32_t over_colour;

    // Type 5: Sprite
    Pix32 *graphic;
    Pix32 *active_graphic;

    // Type 6: 3D Model
    int model_id;
    int active_model_id;
    int anim_id;
    int active_anim_id;
    int zoom;
    int xan;
    int yan;
    Model *model;

    // Actions & Buttons
    char *action_verb;
    char *action;
    int action_target;
    char *option;
} Component;

struct ObjConfig;
struct ContextMenu;
struct VarpContext;
struct MenuSelection;

typedef struct ComponentDragState {
    bool active;
    bool threshold;
    int  component_id;
    int  slot;
    int  grab_x;
    int  grab_y;
    int  cycles;
} ComponentDragState;

typedef struct ComponentScrollDrag {
    bool active;
    Component *target;
    int sx;
    int sy;
    int h;
    int s_max;
    int grab_offset;
    int arrow_dir;
    int arrow_hold_ticks;
} ComponentScrollDrag;

typedef struct ComponentConfig {
    int count;
    Component *instances;
    struct ObjConfig *obj_config;
    PixFont *font_p11;
    Pix8 *scrollbar0;
    Pix8 *scrollbar1;
    struct VarpContext *varp_ctx;
    struct MenuSelection *selection;
    int mouse_x;
    int mouse_y;
    ComponentDragState drag;
    ComponentScrollDrag scroll_drag;
} ComponentConfig;

bool component_unpack(ComponentConfig *cfg, Jagfile *media_jag, PixFont *fonts[4], Jagfile *interface_jag, Arena *arena);
Component *component_get(ComponentConfig *cfg, int id);
void draw_scrollbar(int x, int y, int scroll_pos, int scroll_max, int height, Pix8 *sb0, Pix8 *sb1);
void component_draw(ComponentConfig *cfg, Component *com, int x, int y, int scroll_y);
bool component_handle_scroll(Component *com, int mx, int my, int sx, int sy, int drag_cycles);
bool component_handle_scroll_recursive(ComponentConfig *cfg, Component *com, int mx, int my, int x, int y, int scroll_y);
int  component_inv_find_slot(Component *child, int vx, int vy, int mx, int my);
Component *component_find_inv_slot(ComponentConfig *cfg, Component *com, int mx, int my, int x, int y, int scroll_y, int *out_slot);
void component_handle_mousewheel(ComponentConfig *cfg, Component *com, int mx, int my, int x, int y, int scroll_y, int wheel_y);
bool component_build_menu(ComponentConfig *cfg, Component *com, int mx, int my, int x, int y, int scroll_y, struct ContextMenu *menu, struct ObjConfig *obj_cfg);

#endif // RS2_COMPONENT_H
