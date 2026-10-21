#ifndef RS2_ANIM_H
#define RS2_ANIM_H

#include <stdint.h>
#include <stdbool.h>
#include "core/arena.h"
#include "core/jagfile.h"

typedef struct AnimBase {
    int length;
    int *types;
    int *label_counts;
    int **labels;
} AnimBase;

typedef struct AnimFrame {
    int delay;
    AnimBase *base;
    int length;
    int *groups;
    int *x;
    int *y;
    int *z;
} AnimFrame;

typedef struct AnimConfig {
    int base_count;
    AnimBase *bases;
    int frame_count;
    AnimFrame *frames;
} AnimConfig;

extern AnimConfig g_anim_cfg;

bool anim_unpack(Jagfile *models_jag, Arena *arena);
AnimFrame *anim_get_frame(int frame_id);

#endif // RS2_ANIM_H
