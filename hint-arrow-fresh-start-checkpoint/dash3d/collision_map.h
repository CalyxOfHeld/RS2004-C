#ifndef RS2_COLLISION_MAP_H
#define RS2_COLLISION_MAP_H

#include <stdint.h>
#include <stdbool.h>

#define COLLISION_MAX_X 104
#define COLLISION_MAX_Z 104

typedef struct {
    int size_x;
    int size_z;
    int flags[COLLISION_MAX_X][COLLISION_MAX_Z];
} CollisionMap;

void collision_map_init(CollisionMap *cm, int size_x, int size_z);
void collision_map_reset(CollisionMap *cm);
void collision_map_add_loc(CollisionMap *cm, int x, int z, int width, int length, bool blockrange);
void collision_map_del_loc(CollisionMap *cm, int x, int z, int width, int length, bool blockrange);

void collision_map_block_ground(CollisionMap *cm,int x,int z,bool block);
void collision_map_add_wall(CollisionMap *cm,int x,int z,int shape,int angle,bool blockrange);
void collision_map_del_wall(CollisionMap *cm,int x,int z,int shape,int angle,bool blockrange);
bool collision_map_test_wall(const CollisionMap *cm,int srcX,int srcZ,int dstX,int dstZ,int shape,int angle);
bool collision_map_test_decor(const CollisionMap *cm,int srcX,int srcZ,int dstX,int dstZ,int shape,int angle);
bool collision_map_test_loc(const CollisionMap *cm,int srcX,int srcZ,int dstX,int dstZ,int dstSizeX,int dstSizeZ,int forceapproach);

#endif // RS2_COLLISION_MAP_H
