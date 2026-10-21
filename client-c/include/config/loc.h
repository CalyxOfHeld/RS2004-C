#ifndef LOC_TYPE_H
#define LOC_TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include "core/arena.h"
#include "core/jagfile.h"
#include "core/packet.h"
#include "gfx/model.h"

typedef struct LocType {
    int index;
    int* models;
    int* shapes;
    int model_count;
    char name[64];
    char desc[128];
    int* recol_s;
    int* recol_d;
    int recol_count;
    int width;
    int length;
    bool blockwalk;
    bool blockrange;
    bool active;
    bool hillskew;
    bool sharelight;
    bool occlude;
    int anim;
    int wallwidth;
    int8_t ambient;
    int8_t contrast;
    char op[5][32];
    bool op_valid[5];
    bool anim_has_alpha;
    int mapfunction;
    int mapscene;
    bool mirror;
    bool shadow;
    int resizex;
    int resizey;
    int resizez;
    int forceapproach;
    int offsetx;
    int offsety;
    int offsetz;
    bool forcedecor;
} LocType;

typedef struct LocConfig {
    int count;
    int* offsets;
    uint8_t* dat;
    int32_t dat_len;
} LocConfig;

void loc_unpack(LocConfig* config, Jagfile* jag);
void loc_reset(LocType* loc);
void loc_decode(LocType* loc, Packet* pkt);
bool loc_get(LocConfig* config, int id, LocType* out_loc);
void loc_free_entry(LocType* loc);
void loc_free(LocConfig* config);
// Original scene rebuild clears static bases; unload also clears frame models.
void loc_clear_model_cache(bool all);
Model* loc_get_model(const LocType* loc, int shape, int rotation, Arena* arena);
Model* loc_get_model_frame(const LocType* loc, int shape, int rotation, int frame, Arena* arena);
void loc_skew_model(Model *model,const LocType *loc,int sw,int se,int ne,int nw);

#endif // LOC_TYPE_H
