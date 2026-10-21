#ifndef RS2_LOCATION_ANIM_H
#define RS2_LOCATION_ANIM_H
#include "dash3d/world3d.h"
#include "config/seq.h"
#include "config/loc.h"

typedef struct LocationAnimation {
    int id,level,x,z,layer,sequence,frame,cycle;
    bool random_start;
} LocationAnimation;

bool location_anim_add(World3D *scene,int id,int level,int x,int z,int layer,int sequence,bool random_start);
bool location_anim_packet(World3D *scene,int level,int x,int z,int layer,int sequence);
// Original client.pushLocs advances by elapsed logical ticks. False means expiry.
bool location_anim_advance(LocationAnimation *animation,const SeqType *seq,int elapsed,bool *changed);
void location_anim_tick(World3D *scene,LocConfig *locations,int elapsed);
#endif
