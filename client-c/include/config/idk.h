#ifndef RS2_IDK_H
#define RS2_IDK_H

#include <stdint.h>
#include <stdbool.h>
#include "core/jagfile.h"
#include "core/packet.h"
#include "gfx/model.h"

typedef struct {
    int type;
    int model_count;
    int *models;
    int recol_s[6];
    int recol_d[6];
    int recol_count;
    int heads[5];
    bool disable;
} IdkType;

typedef struct {
    int count;
    IdkType *instances;
} IdkConfig;

void idk_unpack(IdkConfig *cfg, Jagfile *jag, Arena *arena);
IdkType* idk_get(IdkConfig *cfg, int id);
Model* idk_get_model(IdkType *idk, Arena *arena);
Model* idk_get_head_model(IdkType *idk, Arena *arena);
void idk_free(IdkConfig *cfg);

#endif // RS2_IDK_H
