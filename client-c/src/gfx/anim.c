#include "gfx/anim.h"
#include "core/packet.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

AnimConfig g_anim_cfg = {0};

bool anim_unpack(Jagfile *models_jag, Arena *arena) {
    if (!models_jag) return false;

    // 1. Unpack AnimBase
    size_t base_head_sz = 0, base_type_sz = 0, base_label_sz = 0;
    uint8_t *base_head_data  = jagfile_read(models_jag, "base_head.dat",  NULL, &base_head_sz,  arena);
    uint8_t *base_type_data  = jagfile_read(models_jag, "base_type.dat",  NULL, &base_type_sz,  arena);
    uint8_t *base_label_data = jagfile_read(models_jag, "base_label.dat", NULL, &base_label_sz, arena);

    if (!base_head_data || !base_type_data || !base_label_data) return false;

    Packet base_head = packet_wrap(base_head_data, base_head_sz);
    Packet base_type = packet_wrap(base_type_data, base_type_sz);
    Packet base_lbl  = packet_wrap(base_label_data, base_label_sz);

    int base_count = g2(&base_head);
    int max_base_id = g2(&base_head);
    g_anim_cfg.base_count = max_base_id + 1;
    g_anim_cfg.bases = (AnimBase *)arena_alloc(arena, g_anim_cfg.base_count * sizeof(AnimBase));
    memset(g_anim_cfg.bases, 0, g_anim_cfg.base_count * sizeof(AnimBase));

    for (int i = 0; i < base_count; i++) {
        int base_id = g2(&base_head);
        int length = g1(&base_head);

        AnimBase *ab = &g_anim_cfg.bases[base_id];
        ab->length = length;
        ab->types = (int *)arena_alloc(arena, length * sizeof(int));
        ab->label_counts = (int *)arena_alloc(arena, length * sizeof(int));
        ab->labels = (int **)arena_alloc(arena, length * sizeof(int *));

        for (int j = 0; j < length; j++) {
            ab->types[j] = g1(&base_type);
            int l_cnt = g1(&base_lbl);
            ab->label_counts[j] = l_cnt;
            ab->labels[j] = (int *)arena_alloc(arena, l_cnt * sizeof(int));
            for (int k = 0; k < l_cnt; k++) {
                ab->labels[j][k] = g1(&base_lbl);
            }
        }
    }

    // 2. Unpack AnimFrame
    size_t frame_head_sz = 0, frame_tran1_sz = 0, frame_tran2_sz = 0, frame_del_sz = 0;
    uint8_t *frame_head_data  = jagfile_read(models_jag, "frame_head.dat",  NULL, &frame_head_sz,  arena);
    uint8_t *frame_tran1_data = jagfile_read(models_jag, "frame_tran1.dat", NULL, &frame_tran1_sz, arena);
    uint8_t *frame_tran2_data = jagfile_read(models_jag, "frame_tran2.dat", NULL, &frame_tran2_sz, arena);
    uint8_t *frame_del_data   = jagfile_read(models_jag, "frame_del.dat",   NULL, &frame_del_sz,   arena);

    if (!frame_head_data || !frame_tran1_data || !frame_tran2_data || !frame_del_data) return false;

    Packet frame_head  = packet_wrap(frame_head_data, frame_head_sz);
    Packet frame_tran1 = packet_wrap(frame_tran1_data, frame_tran1_sz);
    Packet frame_tran2 = packet_wrap(frame_tran2_data, frame_tran2_sz);
    Packet frame_del   = packet_wrap(frame_del_data, frame_del_sz);

    int frame_count = g2(&frame_head);
    int max_frame_id = g2(&frame_head);
    g_anim_cfg.frame_count = max_frame_id + 1;
    g_anim_cfg.frames = (AnimFrame *)arena_alloc(arena, g_anim_cfg.frame_count * sizeof(AnimFrame));
    memset(g_anim_cfg.frames, 0, g_anim_cfg.frame_count * sizeof(AnimFrame));

    int temp_groups[500];
    int temp_x[500];
    int temp_y[500];
    int temp_z[500];

    for (int i = 0; i < frame_count; i++) {
        int frame_id = g2(&frame_head);
        AnimFrame *af = &g_anim_cfg.frames[frame_id];
        af->delay = g1(&frame_del);

        int base_id = g2(&frame_head);
        AnimBase *ab = &g_anim_cfg.bases[base_id];
        af->base = ab;

        int count = g1(&frame_head);
        int last_bone = -1;
        int active_count = 0;

        for (int j = 0; j < count; j++) {
            int flags = g1(&frame_tran1);
            if (flags > 0) {
                if (ab->types[j] != 0) {
                    for (int k = j - 1; k > last_bone; k--) {
                        if (ab->types[k] == 0) {
                            temp_groups[active_count] = k;
                            temp_x[active_count] = 0;
                            temp_y[active_count] = 0;
                            temp_z[active_count] = 0;
                            active_count++;
                            break;
                        }
                    }
                }

                temp_groups[active_count] = j;
                int default_val = (ab->types[j] == 3) ? 128 : 0;

                temp_x[active_count] = ((flags & 0x1) == 0) ? default_val : gsmart(&frame_tran2);
                temp_y[active_count] = ((flags & 0x2) == 0) ? default_val : gsmart(&frame_tran2);
                temp_z[active_count] = ((flags & 0x4) == 0) ? default_val : gsmart(&frame_tran2);

                last_bone = j;
                active_count++;
            }
        }

        af->length = active_count;
        af->groups = (int *)arena_alloc(arena, active_count * sizeof(int));
        af->x = (int *)arena_alloc(arena, active_count * sizeof(int));
        af->y = (int *)arena_alloc(arena, active_count * sizeof(int));
        af->z = (int *)arena_alloc(arena, active_count * sizeof(int));

        for (int k = 0; k < active_count; k++) {
            af->groups[k] = temp_groups[k];
            af->x[k] = temp_x[k];
            af->y[k] = temp_y[k];
            af->z[k] = temp_z[k];
        }
    }

    return true;
}

AnimFrame *anim_get_frame(int frame_id) {
    if (frame_id < 0 || frame_id >= g_anim_cfg.frame_count) return NULL;
    return &g_anim_cfg.frames[frame_id];
}
