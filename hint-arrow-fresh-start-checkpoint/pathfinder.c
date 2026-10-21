#include "dash3d/pathfinder.h"
#include <string.h>

void pathfinder_init(Pathfinder *pf) {
    if (!pf) return;
    memset(pf, 0, sizeof(Pathfinder));
}

static bool reached_target(const CollisionMap *cm,int x,int z,const PathTarget *t) {
    if(x==t->x && z==t->z) return true;
    if(t->shape>=0) {
        if((t->shape<4 || t->shape==9) && collision_map_test_wall(cm,x,z,t->x,t->z,t->shape,t->rotation)) return true;
        if(t->shape<9 && collision_map_test_decor(cm,x,z,t->x,t->z,t->shape,t->rotation)) return true;
    }
    return t->width>0 && t->length>0 && collision_map_test_loc(cm,x,z,t->x,t->z,t->width,t->length,t->forceapproach);
}
bool pathfinder_find_path(Pathfinder *pf,const CollisionMap *cm,int src_x,int src_z,int dst_x,int dst_z,bool nearest,PathResult *result) {
    PathTarget target={.x=dst_x,.z=dst_z,.shape=-1};
    return pathfinder_find_target(pf,cm,src_x,src_z,&target,nearest,result);
}
bool pathfinder_find_target(Pathfinder *pf,const CollisionMap *cm,int src_x,int src_z,const PathTarget *target,bool find_nearest,PathResult *out_result) {
    if(!target) return false;
    int dst_x=target->x,dst_z=target->z;
    if (!pf || !cm || !out_result) return false;
    memset(out_result, 0, sizeof(PathResult));

    if (src_x < 0 || src_x >= COLLISION_MAX_X || src_z < 0 || src_z >= COLLISION_MAX_Z) return false;
    if (dst_x < 0 || dst_x >= COLLISION_MAX_X || dst_z < 0 || dst_z >= COLLISION_MAX_Z) return false;

    // Same tile - already reached
    if (reached_target(cm,src_x,src_z,target)) {
        out_result->step_count = 0;
        out_result->reached_destination = true;
        return true;
    }

    const int max_x = cm->size_x;
    const int max_z = cm->size_z;

    // Reset BFS tables
    for (int x = 0; x < max_x; x++) {
        for (int z = 0; z < max_z; z++) {
            pf->bfs_direction[x][z] = 0;
            pf->bfs_cost[x][z] = 99999999;
        }
    }

    pf->bfs_direction[src_x][src_z] = 99;
    pf->bfs_cost[src_x][src_z] = 0;

    int read_pos = 0;
    int write_pos = 0;
    pf->bfs_step_x[write_pos] = src_x;
    pf->bfs_step_z[write_pos] = src_z;
    write_pos = (write_pos + 1) % PATHFINDER_QUEUE_SIZE;

    bool reached = false;
    int cur_x = src_x;
    int cur_z = src_z;

    while (read_pos != write_pos) {
        cur_x = pf->bfs_step_x[read_pos];
        cur_z = pf->bfs_step_z[read_pos];
        read_pos = (read_pos + 1) % PATHFINDER_QUEUE_SIZE;

        if (reached_target(cm,cur_x,cur_z,target)) {
            reached = true;
            break;
        }

        int next_cost = pf->bfs_cost[cur_x][cur_z] + 1;

        // West: x - 1
        if (cur_x > 0 && pf->bfs_direction[cur_x - 1][cur_z] == 0 &&
            (cm->flags[cur_x - 1][cur_z] & 0x280108) == 0) {
            pf->bfs_step_x[write_pos] = cur_x - 1;
            pf->bfs_step_z[write_pos] = cur_z;
            write_pos = (write_pos + 1) % PATHFINDER_QUEUE_SIZE;
            pf->bfs_direction[cur_x - 1][cur_z] = 2;
            pf->bfs_cost[cur_x - 1][cur_z] = next_cost;
        }

        // East: x + 1
        if (cur_x < max_x - 1 && pf->bfs_direction[cur_x + 1][cur_z] == 0 &&
            (cm->flags[cur_x + 1][cur_z] & 0x280180) == 0) {
            pf->bfs_step_x[write_pos] = cur_x + 1;
            pf->bfs_step_z[write_pos] = cur_z;
            write_pos = (write_pos + 1) % PATHFINDER_QUEUE_SIZE;
            pf->bfs_direction[cur_x + 1][cur_z] = 8;
            pf->bfs_cost[cur_x + 1][cur_z] = next_cost;
        }

        // South: z - 1
        if (cur_z > 0 && pf->bfs_direction[cur_x][cur_z - 1] == 0 &&
            (cm->flags[cur_x][cur_z - 1] & 0x280102) == 0) {
            pf->bfs_step_x[write_pos] = cur_x;
            pf->bfs_step_z[write_pos] = cur_z - 1;
            write_pos = (write_pos + 1) % PATHFINDER_QUEUE_SIZE;
            pf->bfs_direction[cur_x][cur_z - 1] = 1;
            pf->bfs_cost[cur_x][cur_z - 1] = next_cost;
        }

        // North: z + 1
        if (cur_z < max_z - 1 && pf->bfs_direction[cur_x][cur_z + 1] == 0 &&
            (cm->flags[cur_x][cur_z + 1] & 0x280120) == 0) {
            pf->bfs_step_x[write_pos] = cur_x;
            pf->bfs_step_z[write_pos] = cur_z + 1;
            write_pos = (write_pos + 1) % PATHFINDER_QUEUE_SIZE;
            pf->bfs_direction[cur_x][cur_z + 1] = 4;
            pf->bfs_cost[cur_x][cur_z + 1] = next_cost;
        }

        // South-West: x - 1, z - 1
        if (cur_x > 0 && cur_z > 0 && pf->bfs_direction[cur_x - 1][cur_z - 1] == 0 &&
            (cm->flags[cur_x - 1][cur_z - 1] & 0x28010E) == 0 &&
            (cm->flags[cur_x - 1][cur_z] & 0x280108) == 0 &&
            (cm->flags[cur_x][cur_z - 1] & 0x280102) == 0) {
            pf->bfs_step_x[write_pos] = cur_x - 1;
            pf->bfs_step_z[write_pos] = cur_z - 1;
            write_pos = (write_pos + 1) % PATHFINDER_QUEUE_SIZE;
            pf->bfs_direction[cur_x - 1][cur_z - 1] = 3;
            pf->bfs_cost[cur_x - 1][cur_z - 1] = next_cost;
        }

        // South-East: x + 1, z - 1
        if (cur_x < max_x - 1 && cur_z > 0 && pf->bfs_direction[cur_x + 1][cur_z - 1] == 0 &&
            (cm->flags[cur_x + 1][cur_z - 1] & 0x280183) == 0 &&
            (cm->flags[cur_x + 1][cur_z] & 0x280180) == 0 &&
            (cm->flags[cur_x][cur_z - 1] & 0x280102) == 0) {
            pf->bfs_step_x[write_pos] = cur_x + 1;
            pf->bfs_step_z[write_pos] = cur_z - 1;
            write_pos = (write_pos + 1) % PATHFINDER_QUEUE_SIZE;
            pf->bfs_direction[cur_x + 1][cur_z - 1] = 9;
            pf->bfs_cost[cur_x + 1][cur_z - 1] = next_cost;
        }

        // North-West: x - 1, z + 1
        if (cur_x > 0 && cur_z < max_z - 1 && pf->bfs_direction[cur_x - 1][cur_z + 1] == 0 &&
            (cm->flags[cur_x - 1][cur_z + 1] & 0x280138) == 0 &&
            (cm->flags[cur_x - 1][cur_z] & 0x280108) == 0 &&
            (cm->flags[cur_x][cur_z + 1] & 0x280120) == 0) {
            pf->bfs_step_x[write_pos] = cur_x - 1;
            pf->bfs_step_z[write_pos] = cur_z + 1;
            write_pos = (write_pos + 1) % PATHFINDER_QUEUE_SIZE;
            pf->bfs_direction[cur_x - 1][cur_z + 1] = 6;
            pf->bfs_cost[cur_x - 1][cur_z + 1] = next_cost;
        }

        // North-East: x + 1, z + 1
        if (cur_x < max_x - 1 && cur_z < max_z - 1 && pf->bfs_direction[cur_x + 1][cur_z + 1] == 0 &&
            (cm->flags[cur_x + 1][cur_z + 1] & 0x2801E0) == 0 &&
            (cm->flags[cur_x + 1][cur_z] & 0x280180) == 0 &&
            (cm->flags[cur_x][cur_z + 1] & 0x280120) == 0) {
            pf->bfs_step_x[write_pos] = cur_x + 1;
            pf->bfs_step_z[write_pos] = cur_z + 1;
            write_pos = (write_pos + 1) % PATHFINDER_QUEUE_SIZE;
            pf->bfs_direction[cur_x + 1][cur_z + 1] = 12;
            pf->bfs_cost[cur_x + 1][cur_z + 1] = next_cost;
        }
    }

    bool reached_nearest = false;
    if (!reached) {
        if (find_nearest) {
            int min_cost = 100;
            for (int r = 1; r < 2; r++) {
                for (int nx = dst_x - r; nx <= dst_x + r; nx++) {
                    for (int nz = dst_z - r; nz <= dst_z + r; nz++) {
                        if (nx >= 0 && nz >= 0 && nx < max_x && nz < max_z &&
                            pf->bfs_cost[nx][nz] < min_cost) {
                            min_cost = pf->bfs_cost[nx][nz];
                            cur_x = nx;
                            cur_z = nz;
                            reached_nearest = true;
                            reached = true;
                        }
                    }
                }
                if (reached) break;
            }
        }
        if (!reached) return false;
    }

    out_result->reached_destination = !reached_nearest;
    out_result->reached_nearest = reached_nearest;

    // Backtrack from target (cur_x, cur_z) back to (src_x, src_z)
    int temp_x[MAX_PATH_STEPS];
    int temp_z[MAX_PATH_STEPS];
    int count = 0;

    int bx = cur_x;
    int bz = cur_z;

    while (bx != src_x || bz != src_z) {
        if (count < MAX_PATH_STEPS) {
            temp_x[count] = bx;
            temp_z[count] = bz;
            count++;
        }
        int dir = pf->bfs_direction[bx][bz];
        if ((dir & 0x2) != 0) {
            bx++;
        } else if ((dir & 0x8) != 0) {
            bx--;
        }
        if ((dir & 0x1) != 0) {
            bz++;
        } else if ((dir & 0x4) != 0) {
            bz--;
        }
        if (dir == 0 || dir == 99) break;
    }

    // Reverse path so that index 0 is first step from src towards dst
    out_result->step_count = count;
    for (int i = 0; i < count; i++) {
        out_result->steps_x[i] = temp_x[count - 1 - i];
        out_result->steps_z[i] = temp_z[count - 1 - i];
    }

    return (count > 0);
}
