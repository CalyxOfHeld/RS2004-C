#ifndef RS2_LOCATION_MERGE_H
#define RS2_LOCATION_MERGE_H
#include "net/zone_sync.h"
bool location_merge_packet(ZoneSync *zone,World *world,World3D *scene,LocConfig *locations,int level,const uint8_t *data);
bool location_merge_tick(ZoneSync *zone,World *world,World3D *scene,LocConfig *locations,CollisionMap *collision,int cycle);
#endif
