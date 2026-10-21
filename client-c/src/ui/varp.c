#include "ui/varp.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/model.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

static int g_level_experience[99];
static bool g_exp_init = false;

static void init_level_exp(void) {
    if (g_exp_init) return;
    int points = 0;
    for (int lvl = 1; lvl <= 99; lvl++) {
        g_level_experience[lvl - 1] = points / 4;
        points += (int)(lvl + 300.0 * pow(2.0, (double)lvl / 7.0));
    }
    g_exp_init = true;
}

void varp_init(VarpContext *ctx) {
    if (!ctx) return;
    init_level_exp();
    memset(ctx, 0, sizeof(VarpContext));
    for (int i = 0; i < 25; i++) {
        ctx->skill_levels[i] = 1;
        ctx->skill_base_levels[i] = 1;
    }
    ctx->skill_levels[3] = 10; // Hitpoints level 10
    ctx->skill_base_levels[3] = 10;
    ctx->energy = 100;
    ctx->combat_level = 3;
    for (int i = 20; i <= 25; i++) {
        ctx->varps[i] = -1; // Default all music tracks unlocked
    }
}

void varp_set(VarpContext *ctx, int varp_id, int value) {
    if (!ctx || varp_id < 0 || varp_id >= MAX_VARPS) return;
    ctx->varps[varp_id] = value;
}

int varp_get(const VarpContext *ctx, int varp_id) {
    if (!ctx || varp_id < 0 || varp_id >= MAX_VARPS) return 0;
    return ctx->varps[varp_id];
}

int varp_eval_script(const VarpContext *ctx, const ComponentConfig *cfg, const Component *com, int script_idx) {
    if (!ctx || !com || !com->scripts || script_idx < 0 || script_idx >= com->script_count) {
        return -2;
    }

    const int *script = com->scripts[script_idx];
    int slen = com->script_lengths[script_idx];
    int acc = 0;
    int pos = 0;

    while (pos < slen) {
        int opcode = script[pos++];
        if (opcode == 0) {
            return acc;
        } else if (opcode == 1) { // Current Skill Level
            if (pos < slen) {
                int skill = script[pos++];
                if (skill >= 0 && skill < 25) acc += ctx->skill_levels[skill];
            }
        } else if (opcode == 2) { // Base Skill Level
            if (pos < slen) {
                int skill = script[pos++];
                if (skill >= 0 && skill < 25) acc += ctx->skill_base_levels[skill];
            }
        } else if (opcode == 3) { // Skill Experience
            if (pos < slen) {
                int skill = script[pos++];
                if (skill >= 0 && skill < 25) acc += ctx->skill_exp[skill];
            }
        } else if (opcode == 4) { // Item count in inventory container
            if (pos + 1 < slen) {
                int com_id = script[pos++];
                int obj_id = script[pos++] + 1;
                Component *c = component_get((ComponentConfig*)cfg, com_id);
                if (c && c->inv_slot_obj_id) {
                    int total = c->width * c->height;
                    for (int i = 0; i < total; i++) {
                        if (c->inv_slot_obj_id[i] == obj_id) {
                            acc += (c->inv_slot_obj_count ? c->inv_slot_obj_count[i] : 1);
                        }
                    }
                }
            }
        } else if (opcode == 5) { // Varp value
            if (pos < slen) {
                int v_id = script[pos++];
                if (v_id >= 0 && v_id < MAX_VARPS) acc += ctx->varps[v_id];
            }
        } else if (opcode == 6) { // Level Experience for base level
            if (pos < slen) {
                int skill = script[pos++];
                if (skill >= 0 && skill < 25) {
                    int lvl = ctx->skill_base_levels[skill];
                    if (lvl >= 1 && lvl <= 99) {
                        acc += g_level_experience[lvl - 1];
                    }
                }
            }
        } else if (opcode == 7) { // Varp percentage
            if (pos < slen) {
                int v_id = script[pos++];
                if (v_id >= 0 && v_id < MAX_VARPS) acc += (ctx->varps[v_id] * 100) / 46875;
            }
        } else if (opcode == 8) { // Combat Level
            acc += ctx->combat_level;
        } else if (opcode == 9) { // Total Skill Levels
            int total = 0;
            for (int i = 0; i < 19; i++) {
                int s_idx = (i == 18) ? 20 : i;
                total += ctx->skill_base_levels[s_idx];
            }
            acc += total;
        } else if (opcode == 10) { // Item count > 0 flag
            if (pos + 1 < slen) {
                int com_id = script[pos++];
                int obj_id = script[pos++] + 1;
                Component *c = component_get((ComponentConfig*)cfg, com_id);
                if (c && c->inv_slot_obj_id) {
                    int total = c->width * c->height;
                    for (int i = 0; i < total; i++) {
                        if (c->inv_slot_obj_id[i] == obj_id) {
                            acc += 999999999;
                            break;
                        }
                    }
                }
            }
        } else if (opcode == 11) { // Energy
            acc += ctx->energy;
        } else if (opcode == 12) { // Weight Carried
            acc += ctx->weight;
        } else if (opcode == 13) { // Varp bit check
            if (pos + 1 < slen) {
                int v_id = script[pos++];
                int bit = script[pos++];
                if (v_id >= 0 && v_id < MAX_VARPS) {
                    acc += ((ctx->varps[v_id] & (1 << bit)) != 0 ? 1 : 0);
                }
            }
        }
    }
    return acc;
}

bool varp_eval_component_triggers(const VarpContext *ctx, const ComponentConfig *cfg, const Component *com) {
    if (!ctx || !com || com->script_comparator_count <= 0 || !com->script_comparator) {
        return false;
    }

    for (int i = 0; i < com->script_comparator_count; i++) {
        int val = varp_eval_script(ctx, cfg, com, i);
        int op  = com->script_operand[i];
        int cmp = com->script_comparator[i];

        if (cmp == 2) { // Less than (<)
            if (val >= op) return false;
        } else if (cmp == 3) { // Greater than (>)
            if (val <= op) return false;
        } else if (cmp == 4) { // Not equal (!=)
            if (val == op) return false;
        } else { // Equal (==)
            if (val != op) return false;
        }
    }
    return true;
}

bool varp_handle_component_click(VarpContext *ctx, const Component *com) {
    if (!ctx || !com) return false;
    if (com->script_count > 0 && com->scripts && com->scripts[0] && com->script_lengths[0] >= 2) {
        if (com->scripts[0][0] == 5) { // Varp operation
            int varp_id = com->scripts[0][1];
            if (varp_id >= 0 && varp_id < MAX_VARPS) {
                if (com->script_comparator_count > 0 && com->script_operand) {
                    ctx->varps[varp_id] = com->script_operand[0];
                } else {
                    ctx->varps[varp_id] = 1 - ctx->varps[varp_id]; // Toggle 0/1
                }
                return true;
            }
        }
    }
    return false;
}

void varp_format_text(const VarpContext *ctx, const ComponentConfig *cfg, const Component *com, const char *raw_text, char *out_buf, size_t out_sz) {
    if (!out_buf || out_sz == 0) return;
    if (!raw_text) {
        out_buf[0] = '\0';
        return;
    }

    char buf[512];
    strncpy(buf, raw_text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    if (strchr(buf, '%') != NULL) {
        while (1) {
            char *p1 = strstr(buf, "%1");
            char *p2 = strstr(buf, "%2");
            char *p3 = strstr(buf, "%3");
            char *p4 = strstr(buf, "%4");
            char *p5 = strstr(buf, "%5");

            char *target = NULL;
            int script_idx = -1;

            if (p1 && (!target || p1 < target)) { target = p1; script_idx = 0; }
            if (p2 && (!target || p2 < target)) { target = p2; script_idx = 1; }
            if (p3 && (!target || p3 < target)) { target = p3; script_idx = 2; }
            if (p4 && (!target || p4 < target)) { target = p4; script_idx = 3; }
            if (p5 && (!target || p5 < target)) { target = p5; script_idx = 4; }

            if (!target) break;

            int val = varp_eval_script(ctx, cfg, com, script_idx);
            char val_str[32];
            if (val < 999999999) {
                snprintf(val_str, sizeof(val_str), "%d", val);
            } else {
                snprintf(val_str, sizeof(val_str), "*");
            }

            char temp[512];
            size_t prefix_len = (size_t)(target - buf);
            size_t val_len = strlen(val_str);
            size_t suffix_len = strlen(target + 2);

            if (prefix_len + val_len + suffix_len >= sizeof(temp)) {
                break;
            }

            memcpy(temp, buf, prefix_len);
            memcpy(temp + prefix_len, val_str, val_len);
            memcpy(temp + prefix_len + val_len, target + 2, suffix_len + 1);
            memcpy(buf, temp, sizeof(buf));
        }
    }

    strncpy(out_buf, buf, out_sz - 1);
    out_buf[out_sz - 1] = '\0';
}

void component_draw_with_varps(ComponentConfig *cfg, Component *com, int x, int y, int scroll_y, const VarpContext *varps) {
    if (!cfg || !com) return;

    bool active = varps ? varp_eval_component_triggers(varps, cfg, com) : false;

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
                        component_draw_with_varps(cfg, child, vx, vy, child->scroll_position, varps);
                        if (child->scroll > child->height) {
                            draw_scrollbar(vx + child->width, vy, child->scroll_position, child->scroll, child->height, cfg ? cfg->scrollbar0 : NULL, cfg ? cfg->scrollbar1 : NULL);
                        }
                    } else {
                        component_draw_with_varps(cfg, child, vx, vy, 0, varps);
                    }
                }
            }

            pix2d_set_clip(prev_left, prev_top, prev_right, prev_bottom);
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

    if (com->type == COMPONENT_TYPE_BOX) {
        uint32_t col = active ? com->active_colour : com->colour;
        if (com->fill) {
            pix2d_fill_rect(x, y, com->width, com->height, col);
        } else {
            pix2d_draw_rect(x, y, com->width, com->height, col);
        }
        return;
    }

    if (com->type == COMPONENT_TYPE_SPRITE) {
        Pix32 *sprite = (active && com->active_graphic) ? com->active_graphic : com->graphic;
        if (sprite) {
            pix32_draw(sprite, x, y);
        }
        return;
    }

    if (com->type == COMPONENT_TYPE_TEXT) {
        if (!com->font) return;
        const char *raw_text = (active && com->active_text) ? com->active_text : com->text;
        if (!raw_text) return;

        char formatted[512];
        varp_format_text(varps, cfg, com, raw_text, formatted, sizeof(formatted));

        uint32_t col = active ? com->active_colour : com->colour;
        char line_buf[256];
        int draw_y = y + com->font->height;
        const char *text = formatted;

        while (text && *text) {
            const char *newline = strstr(text, "\\n");
            size_t seg_len = newline ? (size_t)(newline - text) : strlen(text);
            if (seg_len >= sizeof(line_buf)) seg_len = sizeof(line_buf) - 1;
            memcpy(line_buf, text, seg_len);
            line_buf[seg_len] = '\0';

            if (com->center) {
                pixfont_draw_string_taggable_center(com->font, x + com->width / 2, draw_y, col, line_buf, com->shadowed);
            } else {
                pixfont_draw_string_taggable(com->font, x, draw_y, col, line_buf, com->shadowed);
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

    // Default fallback to base component drawing (e.g. inventory grids)
    component_draw(cfg, com, x, y, scroll_y);
}
