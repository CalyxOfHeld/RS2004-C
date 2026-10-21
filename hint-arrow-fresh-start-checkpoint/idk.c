#include "config/idk.h"
#include <stdlib.h>
#include <string.h>

void idk_unpack(IdkConfig *cfg, Jagfile *jag, Arena *arena) {
    size_t sz = 0;
    uint8_t *data = jagfile_read(jag, "idk.dat", NULL, &sz, arena);
    if (!data) return;

    Packet pkt = packet_wrap(data, sz);
    cfg->count = g2(&pkt);
    cfg->instances = (IdkType*)calloc(cfg->count, sizeof(IdkType));

    for (int i = 0; i < cfg->count; i++) {
        IdkType *idk = &cfg->instances[i];
        idk->type = -1;
        for (int h = 0; h < 5; h++) idk->heads[h] = -1;

        while (true) {
            int opcode = g1(&pkt);
            if (opcode == 0) break;

            if (opcode == 1) {
                idk->type = g1(&pkt);
            } else if (opcode == 2) {
                idk->model_count = g1(&pkt);
                idk->models = (int*)malloc(idk->model_count * sizeof(int));
                for (int m = 0; m < idk->model_count; m++) {
                    idk->models[m] = g2(&pkt);
                }
            } else if (opcode == 3) {
                idk->disable = true;
            } else if (opcode >= 40 && opcode < 50) {
                idk->recol_s[opcode - 40] = g2(&pkt);
                if (opcode - 40 + 1 > idk->recol_count) idk->recol_count = opcode - 40 + 1;
            } else if (opcode >= 50 && opcode < 60) {
                idk->recol_d[opcode - 50] = g2(&pkt);
            } else if (opcode >= 60 && opcode < 70) {
                idk->heads[opcode - 60] = g2(&pkt);
            }
        }
    }
}

IdkType* idk_get(IdkConfig *cfg, int id) {
    if (!cfg || id < 0 || id >= cfg->count) return NULL;
    return &cfg->instances[id];
}

Model* idk_get_model(IdkType *idk, Arena *arena) {
    if (!idk || !idk->models || idk->model_count == 0) return NULL;

    Model **sub = (Model**)malloc(idk->model_count * sizeof(Model*));
    for (int i = 0; i < idk->model_count; i++) {
        sub[i] = model_load(idk->models[i], arena);
    }

    Model *res = NULL;
    if (idk->model_count == 1) {
        res = sub[0];
    } else {
        res = model_create_composite(sub, idk->model_count, arena);
    }
    free(sub);

    if (res) {
        for (int i = 0; i < 6 && idk->recol_s[i] != 0; i++) {
            model_recolor(res, idk->recol_s[i], idk->recol_d[i]);
        }
    }
    return res;
}

Model* idk_get_head_model(IdkType *idk, Arena *arena) {
    if (!idk) return NULL;
    Model *sub[5];
    int count = 0;
    for (int i = 0; i < 5; i++) {
        if (idk->heads[i] != -1) {
            sub[count++] = model_load(idk->heads[i], arena);
        }
    }
    if (count == 0) return NULL;

    Model *res = model_create_composite(sub, count, arena);
    if (res) {
        for (int i = 0; i < 6 && idk->recol_s[i] != 0; i++) {
            model_recolor(res, idk->recol_s[i], idk->recol_d[i]);
        }
    }
    return res;
}

void idk_free(IdkConfig *cfg) {
    if (!cfg || !cfg->instances) return;
    for (int i = 0; i < cfg->count; i++) {
        if (cfg->instances[i].models) {
            free(cfg->instances[i].models);
        }
    }
    free(cfg->instances);
    cfg->instances = NULL;
    cfg->count = 0;
}
