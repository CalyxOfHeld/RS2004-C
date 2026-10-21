#ifndef RS2_PATHING_ENTITY_H
#define RS2_PATHING_ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "gfx/model.h"

typedef struct {
    int x; // sub-tile units (tile * 128 + size * 64)
    int z;
    int y;
    int yaw;
    int dst_yaw;
    int size;

    int seq_stand_id;
    int seq_turn_id;
    int seq_walk_id;
    int seq_turn_around_id;
    int seq_turn_left_id;
    int seq_turn_right_id;
    int seq_run_id;

    int primary_seq_id;
    int primary_seq_frame;
    int primary_seq_cycle;
    int primary_seq_delay;
    int primary_seq_loop;

    int secondary_seq_id;
    int secondary_seq_frame;
    int secondary_seq_cycle;
    int secondary_seq_delay;
    int secondary_seq_loop;

    int path_length;
    int path_tile_x[10];
    int path_tile_z[10];
    bool path_running[10];
    int seq_trigger;

    char chat[256];
    int chat_timer;
    int chat_color;
    int chat_style;

    int damage;
    int damage_type;
    int combat_cycle;
    int health;
    int total_health;

    int target_id;
    int target_tile_x;
    int target_tile_z;

    int spotanim_id,spotanim_height,spotanim_delay;
    int spotanim_frame,spotanim_cycle;
    int exact_start_x,exact_start_z,exact_end_x,exact_end_z,exact_start_cycle,exact_end_cycle,exact_facing;
    int cycle;
    int height;
} PathingEntity;

struct SeqConfig;

void pathing_entity_init(PathingEntity *e);
void pathing_entity_teleport(PathingEntity *e, bool clear_seq, bool queue, int tile_x, int tile_z);
void pathing_entity_move_along_route(PathingEntity *e, bool run, int direction);
void pathing_entity_queue_path(PathingEntity *e, const int *steps_x, const int *steps_z, int count, bool running);
void pathing_entity_update_movement(PathingEntity *e);
void pathing_entity_update_facing(PathingEntity *e);
void pathing_entity_update_sequences(PathingEntity *e, const struct SeqConfig *seq_cfg);

#endif // RS2_PATHING_ENTITY_H
