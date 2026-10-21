#include "config/npc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>

void npc_reset(NpcType* npc) {
    memset(npc, 0, sizeof(NpcType));
    npc->index = -1;
    npc->size = 1;
    npc->readyanim = -1;
    npc->walkanim = -1;
    npc->walkanim_b = -1;
    npc->walkanim_r = -1;
    npc->walkanim_l = -1;
    npc->anim_has_alpha = false;
    npc->resizex = -1;
    npc->resizey = -1;
    npc->resizez = -1;
    npc->minimap = true;
    npc->vislevel = -1;
    npc->resizeh = 128;
    npc->resizev = 128;
}

void npc_unpack(NpcConfig* config, Jagfile* jag) {
    size_t idx_len = 0;
    uint8_t* idx_data = jagfile_read(jag, "npc.idx", NULL, &idx_len, NULL);
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
    config->dat = jagfile_read(jag, "npc.dat", NULL, &dat_sz, NULL);
    config->dat_len = (int32_t)dat_sz;
}

void npc_decode(NpcType* npc, Packet* pkt) {
    while (1) {
        int code = g1(pkt);
        if (code == 0) {
            return;
        }

        if (code == 1) {
            int count = g1(pkt);
            npc->model_count = count;
            npc->models = (int*)malloc(count * sizeof(int));
            for (int i = 0; i < count; i++) {
                npc->models[i] = g2(pkt);
            }
        } else if (code == 2) {
            gjstr(pkt, npc->name, sizeof(npc->name));
        } else if (code == 3) {
            gjstr(pkt, npc->desc, sizeof(npc->desc));
        } else if (code == 12) {
            npc->size = g1b(pkt);
        } else if (code == 13) {
            npc->readyanim = g2(pkt);
        } else if (code == 14) {
            npc->walkanim = g2(pkt);
        } else if (code == 16) {
            npc->anim_has_alpha = true;
        } else if (code == 17) {
            npc->walkanim = g2(pkt);
            npc->walkanim_b = g2(pkt);
            npc->walkanim_r = g2(pkt);
            npc->walkanim_l = g2(pkt);
        } else if (code >= 30 && code < 40) {
            int op_idx = code - 30;
            if (op_idx < 5) {
                char temp[32];
                gjstr(pkt, temp, sizeof(temp));
                if (strcasecmp(temp, "hidden") != 0) {
                    snprintf(npc->op[op_idx], sizeof(npc->op[op_idx]), "%s", temp);
                    npc->op_valid[op_idx] = true;
                }
            }
        } else if (code == 40) {
            int count = g1(pkt);
            npc->recol_count = count;
            npc->recol_s = (int*)malloc(count * sizeof(int));
            npc->recol_d = (int*)malloc(count * sizeof(int));
            for (int i = 0; i < count; i++) {
                npc->recol_s[i] = g2(pkt);
                npc->recol_d[i] = g2(pkt);
            }
        } else if (code == 60) {
            int count = g1(pkt);
            npc->head_count = count;
            npc->heads = (int*)malloc(count * sizeof(int));
            for (int i = 0; i < count; i++) {
                npc->heads[i] = g2(pkt);
            }
        } else if (code == 90) {
            npc->resizex = g2(pkt);
        } else if (code == 91) {
            npc->resizey = g2(pkt);
        } else if (code == 92) {
            npc->resizez = g2(pkt);
        } else if (code == 93) {
            npc->minimap = false;
        } else if (code == 95) {
            npc->vislevel = g2(pkt);
        } else if (code == 97) {
            npc->resizeh = g2(pkt);
        } else if (code == 98) {
            npc->resizev = g2(pkt);
        }
    }
}

bool npc_get(NpcConfig* config, int id, NpcType* out_npc) {
    if (!config || !config->dat || id < 0 || id >= config->count) {
        return false;
    }

    npc_reset(out_npc);
    out_npc->index = id;

    Packet pkt = packet_wrap(config->dat, config->dat_len);
    pkt.pos = (size_t)config->offsets[id];

    npc_decode(out_npc, &pkt);
    return true;
}

Model* npc_get_model(NpcType* npc, Arena* arena) {
    if (!npc || !npc->models || npc->model_count == 0) return NULL;

    Model **sub = (Model**)malloc(npc->model_count * sizeof(Model*));
    for (int i = 0; i < npc->model_count; i++) {
        sub[i] = model_load(npc->models[i], arena);
    }

    Model *res = NULL;
    if (npc->model_count == 1) {
        res = sub[0];
    } else {
        res = model_create_composite(sub, npc->model_count, arena);
    }
    free(sub);

    if (res) {
        if (npc->recol_s) {
            for (int i = 0; i < npc->recol_count; i++) {
                model_recolor(res, npc->recol_s[i], npc->recol_d[i]);
            }
        }
        model_calculate_normals(res, 64, 850, -30, -50, -30);
    }
    return res;
}

Model* npc_get_head_model(NpcType* npc, Arena* arena) {
    if (!npc || !npc->heads || npc->head_count == 0) return NULL;

    Model **sub = (Model**)malloc(npc->head_count * sizeof(Model*));
    for (int i = 0; i < npc->head_count; i++) {
        sub[i] = model_load(npc->heads[i], arena);
    }

    Model *res = NULL;
    if (npc->head_count == 1) {
        res = sub[0];
    } else {
        res = model_create_composite(sub, npc->head_count, arena);
    }
    free(sub);

    if (res) {
        if (npc->recol_s) {
            for (int i = 0; i < npc->recol_count; i++) {
                model_recolor(res, npc->recol_s[i], npc->recol_d[i]);
            }
        }
        model_calculate_normals(res, 64, 850, -30, -50, -30);
    }
    return res;
}

void npc_free_entry(NpcType* npc) {
    if (!npc) return;
    if (npc->models) free(npc->models);
    if (npc->heads) free(npc->heads);
    if (npc->recol_s) free(npc->recol_s);
    if (npc->recol_d) free(npc->recol_d);
    npc->models = NULL;
    npc->heads = NULL;
    npc->recol_s = NULL;
    npc->recol_d = NULL;
}

void npc_free(NpcConfig* config) {
    if (!config) return;
    if (config->offsets) free(config->offsets);
    if (config->dat) free(config->dat);
    config->offsets = NULL;
    config->dat = NULL;
    config->count = 0;
}
