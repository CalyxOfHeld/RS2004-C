#include "entity/pathing_entity.h"
#include "config/seq.h"
#include <string.h>

void pathing_entity_init(PathingEntity *e) {
    if (!e) return;
    memset(e, 0, sizeof(PathingEntity));
    e->size = 1;
    e->seq_stand_id = -1;
    e->seq_turn_id = -1;
    e->seq_walk_id = -1;
    e->seq_turn_around_id = -1;
    e->seq_turn_left_id = -1;
    e->seq_turn_right_id = -1;
    e->seq_run_id = -1;
    e->spotanim_id = -1;
    e->primary_seq_id = -1;
    e->secondary_seq_id = -1;
    e->target_id = -1;
    e->combat_cycle = -1000;
}

void pathing_entity_teleport(PathingEntity *e, bool clear_seq, bool queue, int tile_x, int tile_z) {
    (void)clear_seq;
    if (!queue) {
        int dx = tile_x - e->path_tile_x[0];
        int dz = tile_z - e->path_tile_z[0];
        if (dx >= -8 && dx <= 8 && dz >= -8 && dz <= 8) {
            if (e->path_length < 9) {
                e->path_length++;
            }
            for (int i = e->path_length; i > 0; i--) {
                e->path_tile_x[i] = e->path_tile_x[i - 1];
                e->path_tile_z[i] = e->path_tile_z[i - 1];
                e->path_running[i] = e->path_running[i - 1];
            }
            e->path_tile_x[0] = tile_x;
            e->path_tile_z[0] = tile_z;
            e->path_running[0] = false;
            return;
        }
    }

    e->path_length = 0;
    e->seq_trigger = 0;
    e->path_tile_x[0] = tile_x;
    e->path_tile_z[0] = tile_z;
    e->x = e->path_tile_x[0] * 128 + e->size * 64;
    e->z = e->path_tile_z[0] * 128 + e->size * 64;
}

void pathing_entity_move_along_route(PathingEntity *e, bool run, int direction) {
    int cur_x = e->path_tile_x[0];
    int cur_z = e->path_tile_z[0];

    if (direction == 0) {
        cur_x--;
        cur_z++;
    } else if (direction == 1) {
        cur_z++;
    } else if (direction == 2) {
        cur_x++;
        cur_z++;
    } else if (direction == 3) {
        cur_x--;
    } else if (direction == 4) {
        cur_x++;
    } else if (direction == 5) {
        cur_x--;
        cur_z--;
    } else if (direction == 6) {
        cur_z--;
    } else if (direction == 7) {
        cur_x++;
        cur_z--;
    }

    if (e->path_length < 9) {
        e->path_length++;
    }
    for (int i = e->path_length; i > 0; i--) {
        e->path_tile_x[i] = e->path_tile_x[i - 1];
        e->path_tile_z[i] = e->path_tile_z[i - 1];
        e->path_running[i] = e->path_running[i - 1];
    }
    e->path_tile_x[0] = cur_x;
    e->path_tile_z[0] = cur_z;
    e->path_running[0] = run;
}

void pathing_entity_queue_path(PathingEntity *e, const int *steps_x, const int *steps_z, int count, bool running) {
    if (!e || !steps_x || !steps_z || count <= 0) return;
    if (count > 9) count = 9;

    e->path_length = count;
    e->seq_trigger = 0;

    // steps_x[0] is the immediate next tile, steps_x[count - 1] is destination
    // RS2 stores destination at index 0, immediate next tile at index count - 1
    for (int i = 0; i < count; i++) {
        e->path_tile_x[i] = steps_x[count - 1 - i];
        e->path_tile_z[i] = steps_z[count - 1 - i];
        e->path_running[i] = running;
    }
}

bool pathing_entity_set_sequence(PathingEntity *e,const SeqConfig *sequences,int id,int delay) {
    if(id<-1 || (id>=0 && sequences && id>=sequences->count)) return false;
    if(id==e->primary_seq_id)e->primary_seq_loop=0;
    if(id==-1 || e->primary_seq_id==-1 || !sequences || e->primary_seq_id>=sequences->count ||
       sequences->instances[id].priority>sequences->instances[e->primary_seq_id].priority || sequences->instances[e->primary_seq_id].priority==0) {
        e->primary_seq_id=id;e->primary_seq_frame=e->primary_seq_cycle=e->primary_seq_loop=0;e->primary_seq_delay=delay;
    }
    return true;
}

bool pathing_entity_update_exact(PathingEntity *e,const SeqConfig *sequences,int cycle) {
    if(e->exact_start_cycle<=0 || cycle>e->exact_start_cycle) return false;
    int sx=e->exact_start_x*128+e->size*64,sz=e->exact_start_z*128+e->size*64;
    if(cycle<e->exact_end_cycle) {
        int remaining=e->exact_end_cycle-cycle;
        e->x+=(sx-e->x)/remaining;e->z+=(sz-e->z)/remaining;
    } else {
        bool move=cycle==e->exact_start_cycle || e->primary_seq_id<0 || e->primary_seq_delay!=0;
        if(!move && sequences && e->primary_seq_id<sequences->count)move=e->primary_seq_cycle+1>seq_get_delay(&sequences->instances[e->primary_seq_id],e->primary_seq_frame);
        if(move) {
            int duration=e->exact_start_cycle-e->exact_end_cycle,elapsed=cycle-e->exact_end_cycle;
            int ex=e->exact_end_x*128+e->size*64,ez=e->exact_end_z*128+e->size*64;
            // Invalid coincident/reversed timings cannot divide by zero in C.
            e->x=duration>0 ? (int)(((int64_t)sx*(duration-elapsed)+(int64_t)ex*elapsed)/duration):ex;
            e->z=duration>0 ? (int)(((int64_t)sz*(duration-elapsed)+(int64_t)ez*elapsed)/duration):ez;
        }
    }
    e->seq_trigger=0;
    static const int directions[]={1024,1536,0,512};
    if(e->exact_facing>=0 && e->exact_facing<4)e->dst_yaw=directions[e->exact_facing];
    if(cycle>=e->exact_end_cycle)e->yaw=e->dst_yaw;
    return true;
}

void pathing_entity_update_movement(PathingEntity *e) {
    if (!e) return;
    if (e->path_length == 0) {
        if (e->secondary_seq_id != e->seq_stand_id) {
            e->secondary_seq_id = e->seq_stand_id;
            e->secondary_seq_frame = 0;
            e->secondary_seq_cycle = 0;
        }
        e->seq_trigger = 0;
        return;
    }

    int cur_sub_x = e->x;
    int cur_sub_z = e->z;
    int target_sub_x = e->path_tile_x[e->path_length - 1] * 128 + e->size * 64;
    int target_sub_z = e->path_tile_z[e->path_length - 1] * 128 + e->size * 64;

    if (target_sub_x - cur_sub_x > 256 || target_sub_x - cur_sub_x < -256 ||
        target_sub_z - cur_sub_z > 256 || target_sub_z - cur_sub_z < -256) {
        e->x = target_sub_x;
        e->z = target_sub_z;
        return;
    }

    if (cur_sub_x < target_sub_x) {
        if (cur_sub_z < target_sub_z) {
            e->dst_yaw = 1280;
        } else if (cur_sub_z > target_sub_z) {
            e->dst_yaw = 1792;
        } else {
            e->dst_yaw = 1536;
        }
    } else if (cur_sub_x > target_sub_x) {
        if (cur_sub_z < target_sub_z) {
            e->dst_yaw = 768;
        } else if (cur_sub_z > target_sub_z) {
            e->dst_yaw = 256;
        } else {
            e->dst_yaw = 512;
        }
    } else if (cur_sub_z < target_sub_z) {
        e->dst_yaw = 1024;
    } else {
        e->dst_yaw = 0;
    }

    int yaw_diff = (e->dst_yaw - e->yaw) & 0x7FF;
    if (yaw_diff > 1024) {
        yaw_diff -= 2048;
    }
    int anim_id = e->seq_turn_around_id;
    if (yaw_diff >= -256 && yaw_diff <= 256) {
        anim_id = e->seq_walk_id;
    } else if (yaw_diff >= 256 && yaw_diff < 768) {
        anim_id = e->seq_turn_right_id;
    } else if (yaw_diff >= -768 && yaw_diff <= -256) {
        anim_id = e->seq_turn_left_id;
    }
    if (anim_id == -1) {
        anim_id = e->seq_walk_id;
    }
    if (e->secondary_seq_id == e->seq_stand_id) {
        e->secondary_seq_id = anim_id;
        e->secondary_seq_frame = 0;
        e->secondary_seq_cycle = 0;
    } else {
        e->secondary_seq_id = anim_id;
    }

    int speed = 4; // Subtile speed
    if (e->yaw != e->dst_yaw && e->target_id == -1) {
        speed = 2;
    }
    if (e->path_length > 2) {
        speed = 6;
    }
    if (e->path_length > 3) {
        speed = 8;
    }
    if (e->seq_trigger > 0 && e->path_length > 1) {
        speed = 8;
        e->seq_trigger--;
    }
    if (e->path_running[e->path_length - 1]) {
        speed <<= 1;
    }
    if (speed >= 8 && e->secondary_seq_id == e->seq_walk_id && e->seq_run_id != -1) {
        e->secondary_seq_id = e->seq_run_id;
    }

    if (cur_sub_x < target_sub_x) {
        e->x += speed;
        if (e->x > target_sub_x) e->x = target_sub_x;
    } else if (cur_sub_x > target_sub_x) {
        e->x -= speed;
        if (e->x < target_sub_x) e->x = target_sub_x;
    }

    if (cur_sub_z < target_sub_z) {
        e->z += speed;
        if (e->z > target_sub_z) e->z = target_sub_z;
    } else if (cur_sub_z > target_sub_z) {
        e->z -= speed;
        if (e->z < target_sub_z) e->z = target_sub_z;
    }

    if (e->x == target_sub_x && e->z == target_sub_z) {
        e->path_length--;
    }
}

void pathing_entity_update_facing(PathingEntity *e) {
    if (!e) return;
    int diff = (e->dst_yaw - e->yaw) & 0x7FF;
    if (diff == 0) return;

    if (diff < 32 || diff > 2016) {
        e->yaw = e->dst_yaw;
    } else if (diff > 1024) {
        e->yaw -= 32;
    } else {
        e->yaw += 32;
    }
    e->yaw &= 0x7FF;

    if (e->secondary_seq_id != e->seq_stand_id || e->yaw == e->dst_yaw) {
        return;
    }
    if (e->seq_turn_id != -1) {
        e->secondary_seq_id = e->seq_turn_id;
        return;
    }
    e->secondary_seq_id = e->seq_walk_id;
}

void pathing_entity_update_sequences(PathingEntity *e, const SeqConfig *seq_cfg) {
    if (!e || !seq_cfg) return;

    if (e->secondary_seq_id >= 0 && e->secondary_seq_id < seq_cfg->count) {
        const SeqType *seq = &seq_cfg->instances[e->secondary_seq_id];
        if (seq->frame_count > 0) {
            e->secondary_seq_cycle++;
            int delay = seq_get_delay(seq, e->secondary_seq_frame);
            if (delay <= 0) delay = 1;
            if (e->secondary_seq_frame < seq->frame_count && e->secondary_seq_cycle > delay) {
                e->secondary_seq_cycle = 1;
                e->secondary_seq_frame++;
            }
            if (e->secondary_seq_frame >= seq->frame_count) {
                e->secondary_seq_cycle = 1;
                e->secondary_seq_frame = 0;
            }
        }
    }

    if (e->primary_seq_id >= 0 && e->primary_seq_id < seq_cfg->count && e->primary_seq_delay == 0) {
        const SeqType *seq = &seq_cfg->instances[e->primary_seq_id];
        if (seq->frame_count > 0) {
            e->primary_seq_cycle++;
            int delay = seq_get_delay(seq, e->primary_seq_frame);
            if (delay <= 0) delay = 1;
            while (e->primary_seq_frame < seq->frame_count && e->primary_seq_cycle > delay) {
                e->primary_seq_cycle -= delay;
                e->primary_seq_frame++;
                if (e->primary_seq_frame < seq->frame_count) {
                    delay = seq_get_delay(seq, e->primary_seq_frame);
                    if (delay <= 0) delay = 1;
                }
            }
            if (e->primary_seq_frame >= seq->frame_count) {
                e->primary_seq_frame -= seq->replayoff;
                e->primary_seq_loop++;
                if (e->primary_seq_loop >= seq->replaycount) {
                    e->primary_seq_id = -1;
                }
                if (e->primary_seq_frame < 0 || e->primary_seq_frame >= seq->frame_count) {
                    e->primary_seq_id = -1;
                }
            }
        }
    }
    if (e->primary_seq_delay > 0) {
        e->primary_seq_delay--;
    }
}
