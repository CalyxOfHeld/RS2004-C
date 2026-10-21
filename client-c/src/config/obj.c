#include "config/obj.h"
#include "gfx/pix2d.h"
#include "gfx/pix32.h"
#include "gfx/pix3d.h"
#include "gfx/model.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>
#include <ctype.h>

void obj_reset(ObjType* obj) {
    memset(obj, 0, sizeof(ObjType));
    obj->index = -1;
    obj->zoom2d = 2000;
    obj->code10 = -1;
    obj->cost = 1;
    obj->manwear = -1;
    obj->manwear2 = -1;
    obj->manwear3 = -1;
    obj->womanwear = -1;
    obj->womanwear2 = -1;
    obj->womanwear3 = -1;
    obj->manhead = -1;
    obj->manhead2 = -1;
    obj->womanhead = -1;
    obj->womanhead2 = -1;
    obj->certlink = -1;
    obj->certtemplate = -1;
}

void obj_unpack(ObjConfig* config, Jagfile* jag, bool members_world) {
    config->members_world = members_world;
    size_t idx_len = 0;
    uint8_t* idx_data = jagfile_read(jag, "obj.idx", NULL, &idx_len, NULL);
    if (!idx_data) {
        config->count = 0;
        config->offsets = NULL;
        config->dat = NULL;
        config->dat_len = 0;
        return;
    }

    Packet idx_pkt = packet_wrap(idx_data, idx_len);
    config->count = g2(&idx_pkt);
    config->offsets = (int*)malloc(config->count * sizeof(int));

    int offset = 2;
    for (int i = 0; i < config->count; i++) {
        config->offsets[i] = offset;
        offset += g2(&idx_pkt);
    }
    free(idx_data);

    size_t dat_sz = 0;
    config->dat = jagfile_read(jag, "obj.dat", NULL, &dat_sz, NULL);
    config->dat_len = (int32_t)dat_sz;

    config->icon_cache_size = 512;
    config->icon_cache = (ObjIconEntry*)calloc(config->icon_cache_size, sizeof(ObjIconEntry));
    config->icon_cache_cycle = 0;
}

void obj_decode(ObjType* obj, Packet* pkt) {
    while (1) {
        int code = g1(pkt);
        if (code == 0) {
            return;
        }

        if (code == 1) {
            obj->model = g2(pkt);
        } else if (code == 2) {
            gjstr(pkt, obj->name, sizeof(obj->name));
        } else if (code == 3) {
            gjstr(pkt, obj->desc, sizeof(obj->desc));
        } else if (code == 4) {
            obj->zoom2d = g2(pkt);
        } else if (code == 5) {
            obj->xan2d = g2(pkt);
        } else if (code == 6) {
            obj->yan2d = g2(pkt);
        } else if (code == 7) {
            obj->xof2d = g2(pkt);
            if (obj->xof2d > 32767) {
                obj->xof2d -= 65536;
            }
        } else if (code == 8) {
            obj->yof2d = g2(pkt);
            if (obj->yof2d > 32767) {
                obj->yof2d -= 65536;
            }
        } else if (code == 9) {
            obj->code9 = true;
        } else if (code == 10) {
            obj->code10 = g2(pkt);
        } else if (code == 11) {
            obj->stackable = true;
        } else if (code == 12) {
            obj->cost = g4(pkt);
        } else if (code == 16) {
            obj->members = true;
        } else if (code == 23) {
            obj->manwear = g2(pkt);
            obj->manwear_offset_y = g1b(pkt);
        } else if (code == 24) {
            obj->manwear2 = g2(pkt);
        } else if (code == 25) {
            obj->womanwear = g2(pkt);
            obj->womanwear_offset_y = g1b(pkt);
        } else if (code == 26) {
            obj->womanwear2 = g2(pkt);
        } else if (code >= 30 && code < 35) {
            int op_idx = code - 30;
            char temp[32];
            gjstr(pkt, temp, sizeof(temp));
            if (strcasecmp(temp, "hidden") != 0) {
                snprintf(obj->op[op_idx], sizeof(obj->op[op_idx]), "%s", temp);
                obj->op_valid[op_idx] = true;
            }
        } else if (code >= 35 && code < 40) {
            int iop_idx = code - 35;
            gjstr(pkt, obj->iop[iop_idx], sizeof(obj->iop[iop_idx]));
            obj->iop_valid[iop_idx] = true;
        } else if (code == 40) {
            int count = g1(pkt);
            obj->recol_count = count;
            obj->recol_s = (int*)malloc(count * sizeof(int));
            obj->recol_d = (int*)malloc(count * sizeof(int));
            for (int i = 0; i < count; i++) {
                obj->recol_s[i] = g2(pkt);
                obj->recol_d[i] = g2(pkt);
            }
        } else if (code == 78) {
            obj->manwear3 = g2(pkt);
        } else if (code == 79) {
            obj->womanwear3 = g2(pkt);
        } else if (code == 90) {
            obj->manhead = g2(pkt);
        } else if (code == 91) {
            obj->womanhead = g2(pkt);
        } else if (code == 92) {
            obj->manhead2 = g2(pkt);
        } else if (code == 93) {
            obj->womanhead2 = g2(pkt);
        } else if (code == 95) {
            obj->zan2d = g2(pkt);
        } else if (code == 97) {
            obj->certlink = g2(pkt);
        } else if (code == 98) {
            obj->certtemplate = g2(pkt);
        } else if (code >= 100 && code < 110) {
            int count_idx = code - 100;
            obj->countobj[count_idx] = g2(pkt);
            obj->countco[count_idx] = g2(pkt);
        }
    }
}

void obj_to_certificate(ObjConfig* config, ObjType* obj) {
    if (!config || obj->certtemplate < 0) return;

    ObjType template_obj;
    if (obj_get(config, obj->certtemplate, &template_obj)) {
        obj->model = template_obj.model;
        obj->zoom2d = template_obj.zoom2d;
        obj->xan2d = template_obj.xan2d;
        obj->yan2d = template_obj.yan2d;
        obj->zan2d = template_obj.zan2d;
        obj->xof2d = template_obj.xof2d;
        obj->yof2d = template_obj.yof2d;

        if (template_obj.recol_count > 0) {
            obj->recol_count = template_obj.recol_count;
            if (obj->recol_s) free(obj->recol_s);
            if (obj->recol_d) free(obj->recol_d);
            obj->recol_s = (int*)malloc(obj->recol_count * sizeof(int));
            obj->recol_d = (int*)malloc(obj->recol_count * sizeof(int));
            memcpy(obj->recol_s, template_obj.recol_s, obj->recol_count * sizeof(int));
            memcpy(obj->recol_d, template_obj.recol_d, obj->recol_count * sizeof(int));
        }
        obj_free_entry(&template_obj);
    }

    if (obj->certlink >= 0) {
        ObjType link_obj;
        if (obj_get(config, obj->certlink, &link_obj)) {
            strncpy(obj->name, link_obj.name, sizeof(obj->name) - 1);
            obj->name[sizeof(obj->name) - 1] = '\0';
            obj->members = link_obj.members;
            obj->cost = link_obj.cost;

            char first = (char)toupper((unsigned char)link_obj.name[0]);
            const char* article = (first == 'A' || first == 'E' || first == 'I' || first == 'O' || first == 'U') ? "an" : "a";
            snprintf(obj->desc, sizeof(obj->desc), "Swap this note at any bank for %s %s.", article, link_obj.name);

            obj_free_entry(&link_obj);
        }
    }

    obj->stackable = true;
}

bool obj_get(ObjConfig* config, int id, ObjType* out_obj) {
    if (!config || !config->dat || id < 0 || id >= config->count) {
        return false;
    }

    obj_reset(out_obj);
    out_obj->index = id;

    Packet pkt = packet_wrap(config->dat, config->dat_len);
    pkt.pos = (size_t)config->offsets[id];

    obj_decode(out_obj, &pkt);

    if (out_obj->certtemplate != -1) {
        obj_to_certificate(config, out_obj);
    }

    if (!config->members_world && out_obj->members) {
        strncpy(out_obj->name, "Members Object", sizeof(out_obj->name) - 1);
        strncpy(out_obj->desc, "Login to a members' server to use this object.", sizeof(out_obj->desc) - 1);
        memset(out_obj->op_valid, 0, sizeof(out_obj->op_valid));
        memset(out_obj->iop_valid, 0, sizeof(out_obj->iop_valid));
    }

    return true;
}

Model* obj_get_worn_model(ObjType* obj, int gender, Arena* arena) {
    if (!obj) return NULL;
    int m1 = (gender == 1) ? obj->womanwear : obj->manwear;
    if (m1 == -1) return NULL;

    int m2 = (gender == 1) ? obj->womanwear2 : obj->manwear2;
    int m3 = (gender == 1) ? obj->womanwear3 : obj->manwear3;

    Model *res = model_load(m1, arena);
    if (!res) return NULL;

    if (m2 != -1) {
        Model *sub[3];
        sub[0] = res;
        sub[1] = model_load(m2, arena);
        int count = 2;
        if (m3 != -1) {
            sub[2] = model_load(m3, arena);
            count = 3;
        }
        res = model_create_composite(sub, count, arena);
    }

    int8_t offset_y = (gender == 1) ? obj->womanwear_offset_y : obj->manwear_offset_y;
    if (offset_y != 0 && res) {
        model_translate(res, offset_y, 0, 0);
    }

    if (obj->recol_s && res) {
        for (int i = 0; i < obj->recol_count; i++) {
            model_recolor(res, obj->recol_s[i], obj->recol_d[i]);
        }
    }
    return res;
}

Model* obj_get_head_model(ObjType* obj, int gender, Arena* arena) {
    if (!obj) return NULL;
    int h1 = (gender == 1) ? obj->womanhead : obj->manhead;
    if (h1 == -1) return NULL;

    int h2 = (gender == 1) ? obj->womanhead2 : obj->manhead2;
    Model *res = model_load(h1, arena);
    if (!res) return NULL;

    if (h2 != -1) {
        Model *sub[2];
        sub[0] = res;
        sub[1] = model_load(h2, arena);
        res = model_create_composite(sub, 2, arena);
    }

    if (obj->recol_s && res) {
        for (int i = 0; i < obj->recol_count; i++) {
            model_recolor(res, obj->recol_s[i], obj->recol_d[i]);
        }
    }
    return res;
}

void obj_free_entry(ObjType* obj) {
    if (!obj) return;
    if (obj->recol_s) free(obj->recol_s);
    if (obj->recol_d) free(obj->recol_d);
    obj->recol_s = NULL;
    obj->recol_d = NULL;
}

void format_obj_count(int count, char* out, size_t max_len) {
    if (!out || max_len == 0) return;
    if (count < 100000) {
        snprintf(out, max_len, "%d", count);
    } else if (count < 10000000) {
        snprintf(out, max_len, "%dK", count / 1000);
    } else {
        snprintf(out, max_len, "%dM", count / 1000000);
    }
}

Pix32* obj_get_icon(ObjConfig* config, int id, int count, Arena* arena) {
    if (!config || id < 0 || id >= config->count) return NULL;

    // Check cache
    if (config->icon_cache) {
        for (int i = 0; i < config->icon_cache_size; i++) {
            if (config->icon_cache[i].id == id && config->icon_cache[i].icon) {
                if (config->icon_cache[i].icon->crop_h == count || config->icon_cache[i].icon->crop_h == -1) {
                    config->icon_cache[i].last_cycle = ++config->icon_cache_cycle;
                    return config->icon_cache[i].icon;
                }
            }
        }
    }

    ObjType type;
    if (!obj_get(config, id, &type)) {
        return NULL;
    }

    // If countobj is empty, count doesn't change appearance
    bool has_count_variants = false;
    for (int i = 0; i < 10; i++) {
        if (type.countobj[i] != 0) {
            has_count_variants = true;
            break;
        }
    }
    if (!has_count_variants) {
        count = -1;
    }

    if (count > 1) {
        int count_id = -1;
        for (int i = 0; i < 10; i++) {
            if (count >= type.countco[i] && type.countco[i] != 0) {
                count_id = type.countobj[i];
            }
        }
        if (count_id != -1) {
            obj_free_entry(&type);
            if (!obj_get(config, count_id, &type)) {
                return NULL;
            }
        }
    }

    Pix32 *icon = pix32_create(32, 32, arena);
    if (!icon) {
        obj_free_entry(&type);
        return NULL;
    }

    // Save previous 2D/3D state
    uint32_t *prev_pixels = pix2d_pixels;
    int32_t prev_w = pix2d_width;
    int32_t prev_h = pix2d_height;
    int32_t prev_clip_left = pix2d_clip_left;
    int32_t prev_clip_right = pix2d_clip_right;
    int32_t prev_clip_top = pix2d_clip_top;
    int32_t prev_clip_bottom = pix2d_clip_bottom;
    int32_t prev_center_x = pix3d_center_x;
    int32_t prev_center_y = pix3d_center_y;

    pix2d_bind(icon->pixels, 32, 32);
    pix2d_fill_rect(0, 0, 32, 32, 0);
    pix3d_init_2d();

    Model *model = model_load(type.model, arena);
    if (model) {
        if (type.recol_s && type.recol_count > 0) {
            for (int i = 0; i < type.recol_count; i++) {
                model_recolor(model, type.recol_s[i], type.recol_d[i]);
            }
        }
        model_calculate_normals(model, 64, 768, -50, -10, -50);

        int32_t max_y = 0;
        for (int i = 0; i < model->num_points; i++) {
            if (-model->point_y[i] > max_y) {
                max_y = -model->point_y[i];
            }
        }

        int32_t sin_xan = (pix3d_sin_table[type.xan2d & 2047] * type.zoom2d) >> 16;
        int32_t cos_xan = (pix3d_cos_table[type.xan2d & 2047] * type.zoom2d) >> 16;
        model_render_obj(model, 0, type.yan2d, type.zan2d, type.xan2d,
                         type.xof2d,
                         sin_xan + max_y / 2 + type.yof2d,
                         cos_xan + type.yof2d);
    }

    // Dark outline pass
    for (int x = 31; x >= 0; x--) {
        for (int y = 31; y >= 0; y--) {
            if (icon->pixels[x + y * 32] == 0) {
                if (x > 0 && icon->pixels[(x - 1) + y * 32] > 1) {
                    icon->pixels[x + y * 32] = 1;
                } else if (y > 0 && icon->pixels[x + (y - 1) * 32] > 1) {
                    icon->pixels[x + y * 32] = 1;
                } else if (x < 31 && icon->pixels[(x + 1) + y * 32] > 1) {
                    icon->pixels[x + y * 32] = 1;
                } else if (y < 31 && icon->pixels[x + (y + 1) * 32] > 1) {
                    icon->pixels[x + y * 32] = 1;
                }
            }
        }
    }

    // Shadow pass
    for (int x = 31; x >= 0; x--) {
        for (int y = 31; y >= 0; y--) {
            if (icon->pixels[x + y * 32] == 0 && x > 0 && y > 0 &&
                icon->pixels[(x - 1) + (y - 1) * 32] > 0) {
                icon->pixels[x + y * 32] = 0x302020;
            }
        }
    }

    // Cert/note template overlay
    if (type.certtemplate != -1) {
        Pix32 *unnoted = obj_get_icon(config, type.certlink, 10, arena);
        if (unnoted) {
            int prev_cw = unnoted->crop_w;
            int prev_ch = unnoted->crop_h;
            unnoted->crop_w = 32;
            unnoted->crop_h = 32;
            pix32_crop(unnoted, 5, 5, 22, 22);
            unnoted->crop_w = prev_cw;
            unnoted->crop_h = prev_ch;
        }
    }

    // Restore previous 2D/3D state
    pix2d_bind(prev_pixels, prev_w, prev_h);
    pix2d_set_clip(prev_clip_left, prev_clip_top, prev_clip_right, prev_clip_bottom);
    pix3d_center_x = prev_center_x;
    pix3d_center_y = prev_center_y;
    pix3d_init_2d();

    if (type.stackable) {
        icon->crop_w = 33;
    } else {
        icon->crop_w = 32;
    }
    icon->crop_h = count;

    // Cache the icon
    if (config->icon_cache && config->icon_cache_size > 0) {
        int slot = -1;
        uint32_t oldest = 0xFFFFFFFF;
        for (int i = 0; i < config->icon_cache_size; i++) {
            if (config->icon_cache[i].id == id || config->icon_cache[i].icon == NULL) {
                slot = i;
                break;
            }
            if (config->icon_cache[i].last_cycle < oldest) {
                oldest = config->icon_cache[i].last_cycle;
                slot = i;
            }
        }
        if (slot >= 0) {
            config->icon_cache[slot].id = id;
            config->icon_cache[slot].count = count;
            config->icon_cache[slot].icon = icon;
            config->icon_cache[slot].last_cycle = ++config->icon_cache_cycle;
        }
    }

    obj_free_entry(&type);
    return icon;
}

void obj_free(ObjConfig* config) {
    if (!config) return;
    if (config->icon_cache) {
        for (int i = 0; i < config->icon_cache_size; i++) {
            if (config->icon_cache[i].icon) {
                pix32_free(config->icon_cache[i].icon);
            }
        }
        free(config->icon_cache);
        config->icon_cache = NULL;
        config->icon_cache_size = 0;
    }
    if (config->offsets) free(config->offsets);
    if (config->dat) free(config->dat);
    config->offsets = NULL;
    config->dat = NULL;
    config->count = 0;
}
