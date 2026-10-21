#include "ui/component.h"
#include "ui/context_menu.h"
#include "ui/varp.h"
#include "config/obj.h"
#include "core/packet.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CachedSprite {
    char name[32];
    int index;
    Pix32 *sprite;
    struct CachedSprite *next;
} CachedSprite;

static Pix32 *get_cached_sprite(Jagfile *media_jag, const char *name, int index, Arena *arena, CachedSprite **cache) {
    if (!media_jag || !name) return NULL;
    for (CachedSprite *c = *cache; c != NULL; c = c->next) {
        if (c->index == index && strcmp(c->name, name) == 0) {
            return c->sprite;
        }
    }
    Pix32 *sp = pix32_load(media_jag, name, index, arena);
    if (!sp) return NULL;
    CachedSprite *node = (CachedSprite *)arena_alloc(arena, sizeof(CachedSprite));
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    node->index = index;
    node->sprite = sp;
    node->next = *cache;
    *cache = node;
    return sp;
}

static Pix32 *load_sprite_spec(Jagfile *media_jag, const char *spec, Arena *arena, CachedSprite **cache) {
    if (!media_jag || !spec || spec[0] == '\0') return NULL;
    const char *comma = strrchr(spec, ',');
    if (!comma) return NULL;
    char name[32];
    size_t name_len = (size_t)(comma - spec);
    if (name_len >= sizeof(name)) name_len = sizeof(name) - 1;
    memcpy(name, spec, name_len);
    name[name_len] = '\0';
    int idx = atoi(comma + 1);
    return get_cached_sprite(media_jag, name, idx, arena, cache);
}


static char *read_string(Packet *pkt, Arena *arena) {
    char temp[1024];
    gjstr(pkt, temp, sizeof(temp));
    return arena_strdup(arena, temp);
}

bool component_unpack(ComponentConfig *cfg, Jagfile *media_jag, PixFont *fonts[4], Jagfile *interface_jag, Arena *arena) {
    if (!cfg || !interface_jag) return false;
    memset(cfg, 0, sizeof(ComponentConfig));
    CachedSprite *sprite_cache = NULL;

    size_t data_sz = 0;
    uint8_t *data = jagfile_read(interface_jag, "data", NULL, &data_sz, arena);
    if (!data) return false;

    Packet pkt = packet_wrap(data, data_sz);
    int layer = -1;
    int count = g2(&pkt);
    cfg->count = count;
    cfg->instances = (Component *)arena_alloc(arena, count * sizeof(Component));
    memset(cfg->instances, 0, count * sizeof(Component));

    while (pkt.pos < pkt.length) {
        int id = g2(&pkt);
        if (id == 65535) {
            layer = g2(&pkt);
            id = g2(&pkt);
        }
        if (id < 0 || id >= count) break;

        Component *c = &cfg->instances[id];
        c->id = id;
        c->layer = layer;
        c->type = g1(&pkt);
        c->button_type = g1(&pkt);
        c->client_code = g2(&pkt);
        c->width = g2(&pkt);
        c->height = g2(&pkt);

        int ol = g1(&pkt);
        if (ol == 0) {
            c->over_layer = -1;
        } else {
            c->over_layer = ((ol - 1) << 8) + g1(&pkt);
        }

        int sc_count = g1(&pkt);
        if (sc_count > 0) {
            c->script_comparator_count = sc_count;
            c->script_comparator = (int *)arena_alloc(arena, sc_count * sizeof(int));
            c->script_operand = (int *)arena_alloc(arena, sc_count * sizeof(int));
            for (int i = 0; i < sc_count; i++) {
                c->script_comparator[i] = g1(&pkt);
                c->script_operand[i] = g2(&pkt);
            }
        }

        int script_count = g1(&pkt);
        if (script_count > 0) {
            c->script_count = script_count;
            c->script_lengths = (int *)arena_alloc(arena, script_count * sizeof(int));
            c->scripts = (int **)arena_alloc(arena, script_count * sizeof(int *));
            for (int i = 0; i < script_count; i++) {
                int slen = g2(&pkt);
                c->script_lengths[i] = slen;
                c->scripts[i] = (int *)arena_alloc(arena, slen * sizeof(int));
                for (int j = 0; j < slen; j++) {
                    c->scripts[i][j] = g2(&pkt);
                }
            }
        }

        if (c->type == COMPONENT_TYPE_CONTAINER) {
            c->scroll = g2(&pkt);
            c->hide = (g1(&pkt) == 1);
            int child_cnt = g1(&pkt);
            c->child_count = child_cnt;
            c->child_id = (int *)arena_alloc(arena, child_cnt * sizeof(int));
            c->child_x = (int *)arena_alloc(arena, child_cnt * sizeof(int));
            c->child_y = (int *)arena_alloc(arena, child_cnt * sizeof(int));
            for (int i = 0; i < child_cnt; i++) {
                c->child_id[i] = g2(&pkt);
                c->child_x[i] = g2b(&pkt);
                c->child_y[i] = g2b(&pkt);
            }
        }

        if (c->type == COMPONENT_TYPE_MODEL_UI) {
            c->unused_short1 = g2(&pkt);
            c->unused_bool1 = (g1(&pkt) == 1);
        }

        if (c->type == COMPONENT_TYPE_INV_GRID) {
            int slot_cnt = c->width * c->height;
            c->inv_slot_obj_id = (int *)arena_alloc(arena, slot_cnt * sizeof(int));
            c->inv_slot_obj_count = (int *)arena_alloc(arena, slot_cnt * sizeof(int));
            c->draggable = (g1(&pkt) == 1);
            c->interactable = (g1(&pkt) == 1);
            c->usable = (g1(&pkt) == 1);
            c->margin_x = g1(&pkt);
            c->margin_y = g1(&pkt);

            for (int i = 0; i < 20; i++) {
                if (g1(&pkt) == 1) {
                    c->inv_slot_offset_x[i] = g2b(&pkt);
                    c->inv_slot_offset_y[i] = g2b(&pkt);
                    char *spec = read_string(&pkt, arena);
                    c->inv_slot_sprite[i] = load_sprite_spec(media_jag, spec, arena, &sprite_cache);
                }
            }

            for (int i = 0; i < 5; i++) {
                char *s = read_string(&pkt, arena);
                if (s && s[0] != '\0') {
                    c->iops[i] = s;
                }
            }
        }

        if (c->type == COMPONENT_TYPE_BOX) {
            c->fill = (g1(&pkt) == 1);
        }

        if (c->type == COMPONENT_TYPE_TEXT || c->type == COMPONENT_TYPE_MODEL_UI) {
            c->center = (g1(&pkt) == 1);
            int font_idx = g1(&pkt);
            c->font_id = font_idx;
            if (fonts && font_idx >= 0 && font_idx < 4) {
                c->font = fonts[font_idx];
            }
            c->shadowed = (g1(&pkt) == 1);
        }

        if (c->type == COMPONENT_TYPE_TEXT) {
            c->text = read_string(&pkt, arena);
            c->active_text = read_string(&pkt, arena);
        }

        if (c->type == COMPONENT_TYPE_MODEL_UI || c->type == COMPONENT_TYPE_BOX || c->type == COMPONENT_TYPE_TEXT) {
            c->colour = g4(&pkt);
        }

        if (c->type == COMPONENT_TYPE_BOX || c->type == COMPONENT_TYPE_TEXT) {
            c->active_colour = g4(&pkt);
            c->over_colour = g4(&pkt);
        }

        if (c->type == COMPONENT_TYPE_SPRITE) {
            char *spec1 = read_string(&pkt, arena);
            c->graphic = load_sprite_spec(media_jag, spec1, arena, &sprite_cache);
            char *spec2 = read_string(&pkt, arena);
            c->active_graphic = load_sprite_spec(media_jag, spec2, arena, &sprite_cache);
        }

        if (c->type == COMPONENT_TYPE_MODEL_3D) {
            int m1 = g1(&pkt);
            if (m1 != 0) {
                c->model_id = ((m1 - 1) << 8) + g1(&pkt);
            }
            int m2 = g1(&pkt);
            if (m2 != 0) {
                c->active_model_id = ((m2 - 1) << 8) + g1(&pkt);
            }
            int a1 = g1(&pkt);
            c->anim_id = (a1 == 0) ? -1 : (((a1 - 1) << 8) + g1(&pkt));
            int a2 = g1(&pkt);
            c->active_anim_id = (a2 == 0) ? -1 : (((a2 - 1) << 8) + g1(&pkt));
            c->zoom = g2(&pkt);
            c->xan = g2(&pkt);
            c->yan = g2(&pkt);
        }

        if (c->type == COMPONENT_TYPE_TEXT_LIST) {
            int slot_cnt = c->width * c->height;
            c->inv_slot_obj_id = (int *)arena_alloc(arena, slot_cnt * sizeof(int));
            c->inv_slot_obj_count = (int *)arena_alloc(arena, slot_cnt * sizeof(int));
            c->center = (g1(&pkt) == 1);
            int font_idx = g1(&pkt);
            c->font_id = font_idx;
            if (fonts && font_idx >= 0 && font_idx < 4) {
                c->font = fonts[font_idx];
            }
            c->shadowed = (g1(&pkt) == 1);
            c->colour = g4(&pkt);
            c->margin_x = g2b(&pkt);
            c->margin_y = g2b(&pkt);
            c->interactable = (g1(&pkt) == 1);
            for (int i = 0; i < 5; i++) {
                char *s = read_string(&pkt, arena);
                if (s && s[0] != '\0') {
                    c->iops[i] = s;
                }
            }
        }

        if (c->button_type == 2 || c->type == COMPONENT_TYPE_INV_GRID) {
            c->action_verb = read_string(&pkt, arena);
            c->action = read_string(&pkt, arena);
            c->action_target = g2(&pkt);
        }

        if (c->button_type == 1 || c->button_type == 4 || c->button_type == 5 || c->button_type == 6) {
            char *opt = read_string(&pkt, arena);
            if (opt && opt[0] != '\0') {
                c->option = opt;
            } else {
                if (c->button_type == 1) c->option = "Ok";
                else if (c->button_type == 4 || c->button_type == 5) c->option = "Select";
                else if (c->button_type == 6) c->option = "Continue";
            }
        }
    }

    return true;
}

Component *component_get(ComponentConfig *cfg, int id) {
    if (!cfg || id < 0 || id >= cfg->count) return NULL;
    return &cfg->instances[id];
}

void draw_scrollbar(int x, int y, int scroll_pos, int scroll_max, int height, Pix8 *sb0, Pix8 *sb1) {
    if (sb0) pix8_draw(sb0, x, y);
    if (sb1) pix8_draw(sb1, x, y + height - 16);
    pix2d_fill_rect(x, y + 16, 16, height - 32, 0x23201B); // SCROLLBAR_TRACK

    int thumb_h = (height - 32) * height / (scroll_max > 0 ? scroll_max : 1);
    if (thumb_h < 8) thumb_h = 8;
    int track_space = height - 32 - thumb_h;
    int scroll_range = scroll_max - height;
    int thumb_y = 0;
    if (scroll_range > 0) {
        thumb_y = track_space * scroll_pos / scroll_range;
    }

    pix2d_fill_rect(x, y + 16 + thumb_y, 16, thumb_h, 0x4D4233); // SCROLLBAR_GRIP_FOREGROUND
    pix2d_vline(x, y + 16 + thumb_y, thumb_h, 0x766654); // SCROLLBAR_GRIP_HIGHLIGHT
    pix2d_vline(x + 1, y + 16 + thumb_y, thumb_h, 0x766654);
    pix2d_hline(x, y + 16 + thumb_y, 16, 0x766654);
    pix2d_hline(x, y + 17 + thumb_y, 16, 0x766654);

    pix2d_vline(x + 15, y + 16 + thumb_y, thumb_h, 0x332D25); // SCROLLBAR_GRIP_LOWLIGHT
    pix2d_vline(x + 14, y + 17 + thumb_y, thumb_h - 1, 0x332D25);
    pix2d_hline(x, y + 15 + thumb_y + thumb_h, 16, 0x332D25);
    pix2d_hline(x + 1, y + 14 + thumb_y + thumb_h, 15, 0x332D25);
}

bool component_handle_scroll(Component *com, int mx, int my, int sx, int sy, int drag_cycles) {
    if (!com || com->scroll <= com->height) return false;
    int h = com->height;
    int s_max = com->scroll;

    // Up arrow [sx, sx+16) x [sy, sy+16)
    if (mx >= sx && mx < sx + 16 && my >= sy && my < sy + 16) {
        com->scroll_position -= (drag_cycles > 0 ? drag_cycles * 4 : 16);
    }
    // Down arrow [sx, sx+16) x [sy + h - 16, sy + h)
    else if (mx >= sx && mx < sx + 16 && my >= sy + h - 16 && my < sy + h) {
        com->scroll_position += (drag_cycles > 0 ? drag_cycles * 4 : 16);
    }
    // Track [sx, sx+16) x [sy + 16, sy + h - 16)
    else if (mx >= sx && mx < sx + 16 && my >= sy + 16 && my < sy + h - 16) {
        int thumb_h = (h - 32) * h / s_max;
        if (thumb_h < 8) thumb_h = 8;
        int thumb_rel = my - sy - 16 - thumb_h / 2;
        int track_avail = h - 32 - thumb_h;
        if (track_avail > 0) {
            com->scroll_position = (s_max - h) * thumb_rel / track_avail;
        }
    } else {
        return false;
    }

    if (com->scroll_position > com->scroll - com->height) {
        com->scroll_position = com->scroll - com->height;
    }
    if (com->scroll_position < 0) {
        com->scroll_position = 0;
    }
    return true;
}

bool component_handle_scroll_recursive(ComponentConfig *cfg, Component *com, int mx, int my, int x, int y, int scroll_y) {
    if (!cfg || !com || com->type != COMPONENT_TYPE_CONTAINER || !com->child_id || com->hide) return false;
    for (int i = 0; i < com->child_count; i++) {
        int cx = com->child_x[i] + x;
        int cy = com->child_y[i] + y - scroll_y;
        Component *child = component_get(cfg, com->child_id[i]);
        if (!child || child->hide) continue;
        int vx = cx + child->x;
        int vy = cy + child->y;
        if (child->type == COMPONENT_TYPE_CONTAINER) {
            if (child->scroll > child->height) {
                int sx = vx + child->width;
                int sy = vy;
                int h = child->height;
                int s_max = child->scroll;

                // Up arrow [sx, sx+16) x [sy, sy+16)
                if (mx >= sx && mx < sx + 16 && my >= sy && my < sy + 16) {
                    child->scroll_position -= 16;
                    if (child->scroll_position < 0) child->scroll_position = 0;
                    cfg->scroll_drag.active = true;
                    cfg->scroll_drag.target = child;
                    cfg->scroll_drag.sx = sx;
                    cfg->scroll_drag.sy = sy;
                    cfg->scroll_drag.h = h;
                    cfg->scroll_drag.s_max = s_max;
                    cfg->scroll_drag.grab_offset = 0;
                    cfg->scroll_drag.arrow_dir = -1;
                    cfg->scroll_drag.arrow_hold_ticks = 0;
                    return true;
                }
                // Down arrow [sx, sx+16) x [sy + h - 16, sy + h)
                if (mx >= sx && mx < sx + 16 && my >= sy + h - 16 && my < sy + h) {
                    child->scroll_position += 16;
                    int max_s = s_max - h;
                    if (child->scroll_position > max_s) child->scroll_position = max_s;
                    cfg->scroll_drag.active = true;
                    cfg->scroll_drag.target = child;
                    cfg->scroll_drag.sx = sx;
                    cfg->scroll_drag.sy = sy;
                    cfg->scroll_drag.h = h;
                    cfg->scroll_drag.s_max = s_max;
                    cfg->scroll_drag.grab_offset = 0;
                    cfg->scroll_drag.arrow_dir = 1;
                    cfg->scroll_drag.arrow_hold_ticks = 0;
                    return true;
                }
                // Track / thumb [sx, sx+16) x [sy + 16, sy + h - 16)
                if (mx >= sx && mx < sx + 16 && my >= sy + 16 && my < sy + h - 16) {
                    int thumb_h = (h - 32) * h / (s_max > 0 ? s_max : 1);
                    if (thumb_h < 8) thumb_h = 8;
                    int track_avail = h - 32 - thumb_h;
                    int scroll_range = s_max - h;
                    int thumb_top = sy + 16 + (scroll_range > 0 ? track_avail * child->scroll_position / scroll_range : 0);
                    int grab_offset = 0;
                    if (my >= thumb_top && my < thumb_top + thumb_h) {
                        grab_offset = my - thumb_top;
                    } else {
                        grab_offset = thumb_h / 2;
                        int thumb_rel = my - sy - 16 - grab_offset;
                        if (thumb_rel < 0) thumb_rel = 0;
                        if (thumb_rel > track_avail) thumb_rel = track_avail;
                        if (track_avail > 0 && scroll_range > 0) {
                            child->scroll_position = scroll_range * thumb_rel / track_avail;
                        }
                    }
                    if (child->scroll_position > s_max - h) child->scroll_position = s_max - h;
                    if (child->scroll_position < 0) child->scroll_position = 0;

                    cfg->scroll_drag.active = true;
                    cfg->scroll_drag.target = child;
                    cfg->scroll_drag.sx = sx;
                    cfg->scroll_drag.sy = sy;
                    cfg->scroll_drag.h = h;
                    cfg->scroll_drag.s_max = s_max;
                    cfg->scroll_drag.grab_offset = grab_offset;
                    cfg->scroll_drag.arrow_dir = 0;
                    cfg->scroll_drag.arrow_hold_ticks = 0;
                    return true;
                }
            }
            if (component_handle_scroll_recursive(cfg, child, mx, my, vx, vy, child->scroll_position)) {
                return true;
            }
        }
    }
    return false;
}

void component_handle_mousewheel(ComponentConfig *cfg, Component *com, int mx, int my, int x, int y, int scroll_y, int wheel_y) {
    if (!cfg || !com || com->type != COMPONENT_TYPE_CONTAINER || !com->child_id || com->hide) return;

    for (int i = 0; i < com->child_count; i++) {
        int cx = com->child_x[i] + x;
        int cy = com->child_y[i] + y - scroll_y;
        Component *child = component_get(cfg, com->child_id[i]);
        if (!child || child->hide) continue;
        int vx = cx + child->x;
        int vy = cy + child->y;
        if (child->type == COMPONENT_TYPE_CONTAINER) {
            if (child->scroll > child->height) {
                bool inside_child = (mx >= vx && my >= vy && mx < vx + child->width + 16 && my < vy + child->height);
                bool inside_tab = (mx >= 550 && mx < 765 && my >= 220 && my < 500 && x >= 550);
                if (inside_child || inside_tab) {
                    child->scroll_position -= wheel_y * 45;
                    if (child->scroll_position > child->scroll - child->height) {
                        child->scroll_position = child->scroll - child->height;
                    }
                    if (child->scroll_position < 0) {
                        child->scroll_position = 0;
                    }
                    return;
                }
            }
            component_handle_mousewheel(cfg, child, mx, my, vx, vy, child->scroll_position, wheel_y);
        }
    }
}

int component_inv_find_slot(Component *child, int vx, int vy, int mx, int my) {
    if (!child || child->type != COMPONENT_TYPE_INV_GRID) return -1;
    int slot = 0;
    int total_slots = child->width * child->height;
    for (int r = 0; r < child->height; r++) {
        for (int c = 0; c < child->width; c++) {
            int sx = vx + c * (child->margin_x + 32);
            int sy = vy + r * (child->margin_y + 32);
            if (slot < 20) {
                sx += child->inv_slot_offset_x[slot];
                sy += child->inv_slot_offset_y[slot];
            }
            if (mx >= sx && my >= sy && mx < sx + 32 && my < sy + 32) {
                return (slot < total_slots) ? slot : -1;
            }
            slot++;
        }
    }
    return -1;
}

Component *component_find_inv_slot(ComponentConfig *cfg, Component *com, int mx, int my, int x, int y, int scroll_y, int *out_slot) {
    if (!cfg || !com || com->hide) return NULL;
    if (com->type == COMPONENT_TYPE_CONTAINER) {
        if (!com->child_id) return NULL;
        if (mx < x || my < y || mx >= x + com->width || my >= y + com->height) return NULL;

        for (int i = 0; i < com->child_count; i++) {
            int cx = com->child_x[i] + x;
            int cy = com->child_y[i] + y - scroll_y;
            Component *child = component_get(cfg, com->child_id[i]);
            if (!child || child->hide) continue;
            int vx = cx + child->x;
            int vy = cy + child->y;

            if (child->type == COMPONENT_TYPE_CONTAINER) {
                Component *found = component_find_inv_slot(cfg, child, mx, my, vx, vy, child->scroll_position, out_slot);
                if (found) return found;
            } else if (child->type == COMPONENT_TYPE_INV_GRID) {
                int s = component_inv_find_slot(child, vx, vy, mx, my);
                if (s >= 0) {
                    if (out_slot) *out_slot = s;
                    return child;
                }
            }
        }
    } else if (com->type == COMPONENT_TYPE_INV_GRID) {
        int s = component_inv_find_slot(com, x, y, mx, my);
        if (s >= 0) {
            if (out_slot) *out_slot = s;
            return com;
        }
    }
    return NULL;
}

bool component_drag_release(ComponentConfig *cfg,Component *destination,int slot) {
    if(!cfg) return false;
    ComponentDragState drag=cfg->drag;
    cfg->drag.active=false;
    if(!drag.active || !drag.threshold || drag.cycles<5 || !destination ||
        destination->id!=drag.component_id || !destination->draggable ||
        !destination->inv_slot_obj_id || !destination->inv_slot_obj_count ||
        destination->width<=0 || destination->height<=0 ||
        drag.slot<0 || drag.slot>=destination->width*destination->height ||
        slot<0 || slot>=destination->width*destination->height || slot==drag.slot ||
        destination->inv_slot_obj_id[drag.slot]<=0) return false;
    // client.java objGrabThreshold / objDragCycles: swap, then INV_BUTTOND.
    int id=destination->inv_slot_obj_id[slot],count=destination->inv_slot_obj_count[slot];
    destination->inv_slot_obj_id[slot]=destination->inv_slot_obj_id[drag.slot];
    destination->inv_slot_obj_count[slot]=destination->inv_slot_obj_count[drag.slot];
    destination->inv_slot_obj_id[drag.slot]=id;
    destination->inv_slot_obj_count[drag.slot]=count;
    return true;
}

bool component_build_menu(ComponentConfig *cfg, Component *com, int mx, int my, int x, int y, int scroll_y, ContextMenu *menu, ObjConfig *obj_cfg) {
    if (!cfg || !com || com->type != COMPONENT_TYPE_CONTAINER || !com->child_id || com->hide) return false;
    if (mx < x || my < y || mx >= x + com->width || my >= y + com->height) return false;

    bool found = false;
    for (int i = 0; i < com->child_count; i++) {
        int cx = com->child_x[i] + x;
        int cy = com->child_y[i] + y - scroll_y;
        Component *child = component_get(cfg, com->child_id[i]);
        if (!child || child->hide) continue;
        int vx = cx + child->x;
        int vy = cy + child->y;

        if (child->type == COMPONENT_TYPE_CONTAINER) {
            if (component_build_menu(cfg, child, mx, my, vx, vy, child->scroll_position, menu, obj_cfg)) {
                found = true;
            }
        } else if (child->type == COMPONENT_TYPE_INV_GRID) {
            int slot = 0;
            int total_slots = child->width * child->height;
            for (int r = 0; r < child->height; r++) {
                for (int c = 0; c < child->width; c++) {
                    int sx = vx + c * (child->margin_x + 32);
                    int sy = vy + r * (child->margin_y + 32);
                    if (slot < 20) {
                        sx += child->inv_slot_offset_x[slot];
                        sy += child->inv_slot_offset_y[slot];
                    }
                    if (mx >= sx && my >= sy && mx < sx + 32 && my < sy + 32) {
                        found = true;
                        if (slot < total_slots && child->inv_slot_obj_id && child->inv_slot_obj_id[slot] > 0) {
                            int obj_id = child->inv_slot_obj_id[slot] - 1;
                            ObjType obj;
                            bool has_obj = (obj_cfg && obj_get(obj_cfg, obj_id, &obj));
                            const char *name = (has_obj && obj.name[0] != '\0') ? obj.name : "Item";

                            char opt_buf[128];
                            MenuAction action={.type=MENU_ITEM_EXAMINE,.target=obj_id,.component=child->id,.slot=slot};
                            snprintf(opt_buf,sizeof(opt_buf),"Examine @lre@%s",name);
                            context_menu_add_action(menu,opt_buf,action);
                            MenuSelection *selected=cfg->selection;
                            if(selected && selected->item_selected && child->interactable) {
                                if(selected->component!=child->id || selected->slot!=slot) {
                                    action.type=MENU_ITEM_ON_ITEM;
                                    snprintf(opt_buf,sizeof(opt_buf),"Use %.40s with @lre@%.40s",selected->name,name);
                                    context_menu_add_action(menu,opt_buf,action);
                                }
                            } else {
                                if(child->interactable) for(int o=4;o>=3;o--) {
                                    const char *op=has_obj && obj.iop[o][0] ? obj.iop[o] : o==4 ? "Drop" : NULL;
                                    if(op) {
                                        action.type=MENU_HELD_OP;action.operation=o+1;
                                        snprintf(opt_buf,sizeof(opt_buf),"%s @lre@%s",op,name);
                                        context_menu_add_action(menu,opt_buf,action);
                                    }
                                }
                                if(child->usable) {
                                    action.type=MENU_ITEM_SELECT;
                                    snprintf(opt_buf,sizeof(opt_buf),"Use @lre@%s",name);
                                    context_menu_add_action(menu,opt_buf,action);
                                }
                                if(child->interactable && has_obj) for(int o=2;o>=0;o--) {
                                    if(obj.iop[o][0]) {
                                        action.type=MENU_HELD_OP;action.operation=o+1;
                                        snprintf(opt_buf,sizeof(opt_buf),"%s @lre@%s",obj.iop[o],name);
                                        context_menu_add_action(menu,opt_buf,action);
                                    }
                                }
                                for(int o=4;o>=0;o--) if(child->iops[o]) {
                                    action.type=MENU_INV_BUTTON;action.operation=o+1;
                                    snprintf(opt_buf,sizeof(opt_buf),"%s @lre@%s",child->iops[o],name);
                                    context_menu_add_action(menu,opt_buf,action);
                                }
                            }
                            if(has_obj) obj_free_entry(&obj);
                        }
                    }
                    slot++;
                }
            }
        } else if (mx >= vx && my >= vy && mx < vx + child->width && my < vy + child->height) {
            found = true;
            if (child->button_type == 1 && child->option) {
                context_menu_add_action(menu,child->option,(MenuAction){.type=MENU_IF_BUTTON,.operation=1,.component=child->id});
            } else if (child->button_type == 2 && child->action_verb && child->action) {
                char opt[128];
                snprintf(opt, sizeof(opt), "%s @gre@%s", child->action_verb, child->action);
                context_menu_add_action(menu,opt,(MenuAction){.type=MENU_SPELL_SELECT,.operation=2,.component=child->id});
            } else if (child->button_type == 3) {
                context_menu_add_action(menu,"Close",(MenuAction){.type=MENU_IF_CLOSE,.operation=0,.component=child->id});
            } else if ((child->button_type == 4 || child->button_type == 5) && child->option) {
                context_menu_add_action(menu,child->option,(MenuAction){.type=MENU_IF_BUTTON,.operation=child->button_type,.component=child->id});
            } else if (child->button_type == 6) {
                context_menu_add_action(menu,child->option ? child->option : "Click here to continue",(MenuAction){.type=MENU_IF_CONTINUE,.operation=0,.component=child->id});
            }
        }
    }
    return found;
}

void component_draw(ComponentConfig *cfg, Component *com, int x, int y, int scroll_y) {
    if (!cfg || !com) return;

    if (com->type == COMPONENT_TYPE_CONTAINER) {
        if (!com->child_id || com->hide) return;

        int prev_left = pix2d_clip_left;
        int prev_top = pix2d_clip_top;
        int prev_right = pix2d_clip_right;
        int prev_bottom = pix2d_clip_bottom;

        int clip_l = (x > prev_left) ? x : prev_left;
        int clip_t = (y > prev_top) ? y : prev_top;
        int clip_r = (x + com->width < prev_right) ? (x + com->width) : prev_right;
        int clip_b = (y + com->height < prev_bottom) ? (y + com->height) : prev_bottom;

        if (clip_l < clip_r && clip_t < clip_b) {
            pix2d_set_clip(clip_l, clip_t, clip_r, clip_b);

            for (int i = 0; i < com->child_count; i++) {
                int cx = com->child_x[i] + x;
                int cy = com->child_y[i] + y - scroll_y;
                Component *child = component_get(cfg, com->child_id[i]);
                if (child) {
                    int vx = cx + child->x;
                    int vy = cy + child->y;
                    if (child->type == COMPONENT_TYPE_CONTAINER) {
                        if (child->scroll_position > child->scroll - child->height) {
                            child->scroll_position = child->scroll - child->height;
                        }
                        if (child->scroll_position < 0) {
                            child->scroll_position = 0;
                        }
                        component_draw(cfg, child, vx, vy, child->scroll_position);
                        if (child->scroll > child->height) {
                            draw_scrollbar(vx + child->width, vy, child->scroll_position, child->scroll, child->height, cfg->scrollbar0, cfg->scrollbar1);
                        }
                    } else {
                        component_draw(cfg, child, vx, vy, 0);
                    }
                }
            }

            pix2d_set_clip(prev_left, prev_top, prev_right, prev_bottom);
        }
        return;
    }

    if (com->type == COMPONENT_TYPE_BOX) {
        if (com->fill) {
            pix2d_fill_rect(x, y, com->width, com->height, com->colour);
        } else {
            pix2d_draw_rect(x, y, com->width, com->height, com->colour);
        }
        return;
    }

    if (com->type == COMPONENT_TYPE_SPRITE) {
        if (com->graphic) {
            pix32_draw(com->graphic, x, y);
        }
        return;
    }

    if (com->type == COMPONENT_TYPE_MODEL_3D) {
        if (com->model) {
            int prev_cx = pix3d_center_x;
            int prev_cy = pix3d_center_y;
            pix3d_center_x = x + com->width / 2;
            pix3d_center_y = y + com->height / 2;
            int eye_y = (pix3d_sin_table[com->xan & 2047] * com->zoom) >> 16;
            int eye_z = (pix3d_cos_table[com->xan & 2047] * com->zoom) >> 16;
            model_render_obj(com->model, 0, com->yan, 0, com->xan, 0, eye_y, eye_z);
            pix3d_center_x = prev_cx;
            pix3d_center_y = prev_cy;
        }
        return;
    }

    if (com->type == COMPONENT_TYPE_TEXT) {
        if (!com->font || !com->text) return;
        char formatted[512];
        if (cfg && cfg->varp_ctx) {
            varp_format_text(cfg->varp_ctx, cfg, com, com->text, formatted, sizeof(formatted));
        } else {
            strncpy(formatted, com->text, sizeof(formatted) - 1);
            formatted[sizeof(formatted) - 1] = '\0';
        }
        const char *text = formatted;
        char line_buf[256];
        int draw_y = y + com->font->height;

        while (text && *text) {
            const char *newline = strstr(text, "\\n");
            size_t seg_len = newline ? (size_t)(newline - text) : strlen(text);
            if (seg_len >= sizeof(line_buf)) seg_len = sizeof(line_buf) - 1;
            memcpy(line_buf, text, seg_len);
            line_buf[seg_len] = '\0';

            if (com->center) {
                pixfont_draw_string_taggable_center(com->font, x + com->width / 2, draw_y, com->colour, line_buf, com->shadowed);
            } else {
                pixfont_draw_string_taggable(com->font, x, draw_y, com->colour, line_buf, com->shadowed);
            }

            draw_y += com->font->height;
            if (newline) {
                text = newline + 2;
            } else {
                break;
            }
        }
        return;
    }

    if (com->type == COMPONENT_TYPE_INV_GRID) {
        int slot = 0;
        int total_slots = com->width * com->height;
        int dragged_slot = -1;
        int dragged_draw_x = 0, dragged_draw_y = 0;
        int dragged_obj_id = -1, dragged_count = 0;

        for (int r = 0; r < com->height; r++) {
            for (int c = 0; c < com->width; c++) {
                int slot_x = x + c * (com->margin_x + 32);
                int slot_y = y + r * (com->margin_y + 32);
                if (slot < 20) {
                    slot_x += com->inv_slot_offset_x[slot];
                    slot_y += com->inv_slot_offset_y[slot];
                }
                if (slot < total_slots && com->inv_slot_obj_id && com->inv_slot_obj_id[slot] > 0) {
                    int obj_id = com->inv_slot_obj_id[slot] - 1;
                    int count = com->inv_slot_obj_count ? com->inv_slot_obj_count[slot] : 1;

                    bool is_dragged = false;
                    if (cfg && cfg->drag.active && cfg->drag.component_id == com->id && cfg->drag.slot == slot) {
                        if (cfg->drag.threshold && cfg->drag.cycles >= 5) {
                            is_dragged = true;
                            dragged_slot = slot;
                            dragged_draw_x = slot_x + (cfg->mouse_x - cfg->drag.grab_x);
                            dragged_draw_y = slot_y + (cfg->mouse_y - cfg->drag.grab_y);
                            dragged_obj_id = obj_id;
                            dragged_count = count;
                        }
                    }

                    if (!is_dragged && cfg && cfg->obj_config) {
                        Pix32 *icon = obj_get_icon(cfg->obj_config, obj_id, count, NULL);
                        if (icon) {
                            bool is_selected = (cfg->selection && cfg->selection->item_selected &&
                                                cfg->selection->component == com->id && cfg->selection->slot == slot);
                            if (is_selected) {
                                pix32_draw_trans(icon, slot_x, slot_y, 128);
                            } else {
                                pix32_draw(icon, slot_x, slot_y);
                            }
                            if (icon->crop_w == 33 || count != 1) {
                                char count_str[32];
                                format_obj_count(count, count_str, sizeof(count_str));
                                if (cfg->font_p11) {
                                    pixfont_draw_string(cfg->font_p11, slot_x + 1, slot_y + 10, 0x000000, count_str);
                                    pixfont_draw_string(cfg->font_p11, slot_x, slot_y + 9, 0xFFFF00, count_str);
                                }
                            }
                        }
                    }
                } else if (slot < 20 && com->inv_slot_sprite[slot]) {
                    pix32_draw(com->inv_slot_sprite[slot], slot_x, slot_y);
                }
                slot++;
            }
        }

        // Render dragged item floating on top
        if (dragged_slot >= 0 && cfg && cfg->obj_config) {
            Pix32 *icon = obj_get_icon(cfg->obj_config, dragged_obj_id, dragged_count, NULL);
            if (icon) {
                pix32_draw_trans(icon, dragged_draw_x, dragged_draw_y, 128);
                if (icon->crop_w == 33 || dragged_count != 1) {
                    char count_str[32];
                    format_obj_count(dragged_count, count_str, sizeof(count_str));
                    if (cfg->font_p11) {
                        pixfont_draw_string(cfg->font_p11, dragged_draw_x + 1, dragged_draw_y + 10, 0x000000, count_str);
                        pixfont_draw_string(cfg->font_p11, dragged_draw_x, dragged_draw_y + 9, 0xFFFF00, count_str);
                    }
                }
            }
        }
        return;
    }
}
