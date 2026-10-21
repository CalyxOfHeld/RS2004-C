#ifndef RS2_PATHFINDER_H
#define RS2_PATHFINDER_H

#include <stdint.h>
#include <stdbool.h>
#include "dash3d/collision_map.h"

#define PATHFINDER_QUEUE_SIZE 4096
#define MAX_PATH_STEPS 104

typedef struct {
    int step_count;
    int steps_x[MAX_PATH_STEPS];
    int steps_z[MAX_PATH_STEPS];
    bool reached_destination;
    bool reached_nearest;
} PathResult;

typedef struct {
    int bfs_direction[COLLISION_MAX_X][COLLISION_MAX_Z];
    int bfs_cost[COLLISION_MAX_X][COLLISION_MAX_Z];
    int bfs_step_x[PATHFINDER_QUEUE_SIZE];
    int bfs_step_z[PATHFINDER_QUEUE_SIZE];
} Pathfinder;

typedef struct {
    int x,z,width,length,shape,rotation,forceapproach;
} PathTarget; // shape=-1: no wall/decor test; rectangle optional
void pathfinder_init(Pathfinder *pf);
bool pathfinder_find_target(Pathfinder *pf,const CollisionMap *cm,int src_x,int src_z,const PathTarget *target,bool find_nearest,PathResult *out_result);

/**
 * Authentic 2004 RS2 BFS tile pathfinder (client.java:8000-8150).
 *
 * @param pf Pathfinder context
 * @param cm CollisionMap with directional flags
 * @param src_x Starting tile X (0..103)
 * @param src_z Starting tile Z (0..103)
 * @param dst_x Target destination tile X (0..103)
 * @param dst_z Target destination tile Z (0..103)
 * @param find_nearest If true and target is blocked, finds closest adjacent tile
 * @param out_result Output path waypoints
 * @return true if a path was found, false otherwise
 */
bool pathfinder_find_path(Pathfinder *pf, const CollisionMap *cm,
                          int src_x, int src_z,
                          int dst_x, int dst_z,
                          bool find_nearest,
                          PathResult *out_result);

#endif // RS2_PATHFINDER_H
