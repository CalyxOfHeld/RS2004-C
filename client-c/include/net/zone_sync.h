#ifndef RS2_ZONE_SYNC_H
#define RS2_ZONE_SYNC_H
#include "dash3d/world.h"
typedef struct {int level,x,z,layer,id,shape,rotation;} ZoneLocationBaseline;
typedef struct {
    int base_x,base_z;
    bool has_base,dirty,trace;
    unsigned unsupported;
    ObjConfig *objects;
    int local_pid;
    ZoneLocationBaseline *changes;
    size_t change_count,change_capacity;
} ZoneSync;
void zone_sync_reset(ZoneSync *zone);
void zone_sync_dispose(ZoneSync *zone);
// 1 consumed (with explicitly reported unsupported event families), 0 unrelated,
// -1 malformed. Validates a complete enclosed packet before changing the scene.
int zone_sync_apply(ZoneSync *zone,World *world,World3D *scene,LocConfig *locs,CollisionMap *collision,int level,int opcode,const uint8_t *data,size_t length);
#endif
