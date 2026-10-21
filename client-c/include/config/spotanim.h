#ifndef RS2_SPOTANIM_H
#define RS2_SPOTANIM_H
#include "config/seq.h"
#include "gfx/model.h"
typedef struct {
    int model,anim,resizeh,resizev,orientation,ambient,contrast;
    bool anim_has_alpha;
    int recol_s[6],recol_d[6];
    Model *base;
} SpotAnimType;
typedef struct {int count;SpotAnimType *instances;const SeqConfig *sequences;} SpotAnimConfig;
bool spotanim_unpack(SpotAnimConfig *cfg,Jagfile *jag,const SeqConfig *seqs);
void spotanim_free(SpotAnimConfig *cfg);
const SeqType *spotanim_seq(const SpotAnimConfig *cfg,int id);
// Copies own vertices/alpha into the frame arena; cached base is never animated.
Model *spotanim_model(SpotAnimConfig *cfg,int id,int frame,int pitch,int offset,bool oriented,Arena *arena);
#endif
