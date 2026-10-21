#include "entity/player_entity.h"
#include "core/jstring.h"
#include <string.h>
#include <stdlib.h>

static const int PLAYER_COLORS[5][16] = {
    { 6798, 107, 10283, 16, 4797, 7744, 5799, 4634, 33697, 22433, 2983, 54193 },
    { 8741, 12, 64030, 43162, 7735, 8404, 1701, 38430, 24094, 10153, 56621, 4783, 1341, 16578, 35003, 25239 },
    { 25238, 8742, 12, 64030, 43162, 7735, 8404, 1701, 38430, 24094, 10153, 56621, 4783, 1341, 16578, 35003 },
    { 4626, 11146, 6439, 12, 4758, 10270 },
    { 4550, 4537, 5681, 5673, 5790, 6806, 8076, 4574 }
};
static const int PLAYER_COLOR_COUNTS[5] = { 12, 16, 16, 6, 8 };

static const int PLAYER_TORSO_TRIM[16] = {
    9104, 10275, 7595, 3610, 7975, 8526, 918, 38802, 24466, 10145, 58654, 5027, 1457, 16565, 34991, 25486
};

void player_entity_init(PlayerEntity *p) {
    if (!p) return;
    memset(p, 0, sizeof(PlayerEntity));
    pathing_entity_init(&p->base);
}

void player_entity_read_appearance(PlayerEntity *p, Packet *pkt) {
    if (!p || !pkt) return;

    p->gender = g1(pkt);
    p->headicons = g1(pkt);

    for (int i = 0; i < 12; i++) {
        int v = g1(pkt);
        if (v == 0) {
            p->appearances[i] = 0;
        } else {
            p->appearances[i] = (v << 8) | g1(pkt);
        }
    }

    for (int i = 0; i < 5; i++) {
        int c = g1(pkt);
        if (c < 0 || c >= PLAYER_COLOR_COUNTS[i]) c = 0;
        p->colors[i] = c;
    }

    p->base.seq_stand_id = g2(pkt);
    if (p->base.seq_stand_id == 65535) p->base.seq_stand_id = -1;
    p->base.seq_turn_id = g2(pkt);
    if (p->base.seq_turn_id == 65535) p->base.seq_turn_id = -1;
    p->base.seq_walk_id = g2(pkt);
    if (p->base.seq_walk_id == 65535) p->base.seq_walk_id = -1;
    p->base.seq_turn_around_id = g2(pkt);
    if (p->base.seq_turn_around_id == 65535) p->base.seq_turn_around_id = -1;
    p->base.seq_turn_left_id = g2(pkt);
    if (p->base.seq_turn_left_id == 65535) p->base.seq_turn_left_id = -1;
    p->base.seq_turn_right_id = g2(pkt);
    if (p->base.seq_turn_right_id == 65535) p->base.seq_turn_right_id = -1;
    p->base.seq_run_id = g2(pkt);
    if (p->base.seq_run_id == 65535) p->base.seq_run_id = -1;

    uint64_t name37 = g8(pkt);
    jstring_from_base37(name37, p->name, sizeof(p->name));
    jstring_format_name(p->name);

    p->combat_level = g1(pkt);
    p->visible = true;

    p->appearance_hashcode = 0;
    for (int i = 0; i < 12; i++) {
        p->appearance_hashcode <<= 4;
        if (p->appearances[i] >= 256) {
            p->appearance_hashcode += (p->appearances[i] - 256);
        }
    }
    for (int i = 0; i < 5; i++) {
        p->appearance_hashcode <<= 3;
        p->appearance_hashcode += p->colors[i];
    }
}

Model* player_entity_get_model(PlayerEntity *p, IdkConfig *idk_cfg, ObjConfig *obj_cfg, SeqConfig *seq_cfg, Arena *arena) {
    (void)seq_cfg;
    if (!p || !p->visible) return NULL;

    Model *parts[12];
    int part_count = 0;

    for (int i = 0; i < 12; i++) {
        int app = p->appearances[i];
        if (app >= 256 && app < 512) {
            if (idk_cfg) {
                IdkType *idk = idk_get(idk_cfg, app - 256);
                if (idk) {
                    Model *m = idk_get_model(idk, arena);
                    if (m) parts[part_count++] = m;
                }
            }
        } else if (app >= 512) {
            if (obj_cfg) {
                ObjType obj;
                if (obj_get(obj_cfg, app - 512, &obj)) {
                    Model *m = obj_get_worn_model(&obj, p->gender, arena);
                    if (m) parts[part_count++] = m;
                    obj_free_entry(&obj);
                }
            }
        }
    }

    if (part_count == 0) return NULL;
    Model *res = (part_count == 1) ? parts[0] : model_create_composite(parts, part_count, arena);
    if (!res) return NULL;

    for (int i = 0; i < 5; i++) {
        if (p->colors[i] != 0 && p->colors[i] < PLAYER_COLOR_COUNTS[i]) {
            model_recolor(res, PLAYER_COLORS[i][0], PLAYER_COLORS[i][p->colors[i]]);
            if (i == 1) {
                model_recolor(res, PLAYER_TORSO_TRIM[0], PLAYER_TORSO_TRIM[p->colors[1]]);
            }
        }
    }

    model_calculate_normals(res, 64, 850, -30, -50, -30);
    return res;
}

Model* player_entity_get_head_model(PlayerEntity *p, IdkConfig *idk_cfg, ObjConfig *obj_cfg, Arena *arena) {
    if (!p || !p->visible) return NULL;

    Model *parts[12];
    int part_count = 0;

    for (int i = 0; i < 12; i++) {
        int app = p->appearances[i];
        if (app >= 256 && app < 512) {
            if (idk_cfg) {
                IdkType *idk = idk_get(idk_cfg, app - 256);
                if (idk) {
                    Model *m = idk_get_head_model(idk, arena);
                    if (m) parts[part_count++] = m;
                }
            }
        } else if (app >= 512) {
            if (obj_cfg) {
                ObjType obj;
                if (obj_get(obj_cfg, app - 512, &obj)) {
                    Model *m = obj_get_head_model(&obj, p->gender, arena);
                    if (m) parts[part_count++] = m;
                    obj_free_entry(&obj);
                }
            }
        }
    }

    if (part_count == 0) return NULL;
    Model *res = (part_count == 1) ? parts[0] : model_create_composite(parts, part_count, arena);
    if (!res) return NULL;

    for (int i = 0; i < 5; i++) {
        if (p->colors[i] != 0 && p->colors[i] < PLAYER_COLOR_COUNTS[i]) {
            model_recolor(res, PLAYER_COLORS[i][0], PLAYER_COLORS[i][p->colors[i]]);
            if (i == 1) {
                model_recolor(res, PLAYER_TORSO_TRIM[0], PLAYER_TORSO_TRIM[p->colors[1]]);
            }
        }
    }

    model_calculate_normals(res, 64, 850, -30, -50, -30);
    return res;
}
