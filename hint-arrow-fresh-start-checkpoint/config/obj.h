#ifndef OBJ_TYPE_H
#define OBJ_TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include "core/jagfile.h"
#include "core/packet.h"
#include "gfx/model.h"
#include "gfx/pix32.h"

typedef struct ObjType {
    int index;
    int model;
    char name[64];
    char desc[128];
    int* recol_s;
    int* recol_d;
    int recol_count;
    int zoom2d;
    int xan2d;
    int yan2d;
    int zan2d;
    int xof2d;
    int yof2d;
    bool code9;
    int code10;
    bool stackable;
    int cost;
    bool members;
    char op[5][32];
    bool op_valid[5];
    char iop[5][32];
    bool iop_valid[5];
    int manwear;
    int manwear2;
    int manwear3;
    int8_t manwear_offset_y;
    int womanwear;
    int womanwear2;
    int womanwear3;
    int8_t womanwear_offset_y;
    int manhead;
    int manhead2;
    int womanhead;
    int womanhead2;
    int certlink;
    int certtemplate;
    int countobj[10];
    int countco[10];
} ObjType;

typedef struct ObjIconEntry {
    int id;
    int count;
    Pix32 *icon;
    uint32_t last_cycle;
} ObjIconEntry;

typedef struct ObjConfig {
    int count;
    int* offsets;
    uint8_t* dat;
    int32_t dat_len;
    bool members_world;
    ObjIconEntry *icon_cache;
    int icon_cache_size;
    uint32_t icon_cache_cycle;
} ObjConfig;

void obj_unpack(ObjConfig* config, Jagfile* jag, bool members_world);
void obj_reset(ObjType* obj);
void obj_decode(ObjType* obj, Packet* pkt);
bool obj_get(ObjConfig* config, int id, ObjType* out_obj);
Model* obj_get_worn_model(ObjType* obj, int gender, Arena* arena);
Model* obj_get_head_model(ObjType* obj, int gender, Arena* arena);
void obj_to_certificate(ObjConfig* config, ObjType* obj);
Pix32* obj_get_icon(ObjConfig* config, int id, int count, Arena* arena);
void format_obj_count(int count, char* out, size_t max_len);
void obj_free_entry(ObjType* obj);
void obj_free(ObjConfig* config);

#endif // OBJ_TYPE_H
