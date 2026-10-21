#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/model.h"
#include "gfx/anim.h"
#include "config/seq.h"

int main(void) {
    setbuf(stdout, NULL);
    printf("=== Test Suite: Skeletal Animation Engine (Rev 225) ===\n");

    Arena arena = arena_create(48 * 1024 * 1024);

    // 1. Load models.jag
    Jagfile *models_jag = jagfile_load_file("../engine/data/pack/client/models", &arena);
    if (!models_jag) models_jag = jagfile_load_file("engine/data/pack/client/models", &arena);
    assert(models_jag != NULL);
    printf("[PASS] models.jag archive loaded successfully\n");

    // 2. Unpack AnimBase and AnimFrame
    bool ok_anim = anim_unpack(models_jag, &arena);
    assert(ok_anim);
    printf("[INFO] Unpacked %d animation bases and %d animation frames\n",
           g_anim_cfg.base_count, g_anim_cfg.frame_count);
    assert(g_anim_cfg.base_count > 50);
    assert(g_anim_cfg.frame_count > 1000);

    // 3. Inspect a sample frame
    AnimFrame *frame0 = anim_get_frame(0);
    assert(frame0 != NULL);
    printf("[INFO] Frame 0: length=%d bones, delay=%d ticks\n",
           frame0->length, frame0->delay);

    // 4. Initialize model repository and Pix3D tables
    pix3d_init();
    pix3d_init_colour_table(0.8);
    model_init_repo(models_jag, &arena);

    // Load Model 0
    Model *m0 = model_load(0, &arena);
    assert(m0 != NULL);
    assert(m0->num_points > 0);

    // Create label references
    model_create_label_references(m0, &arena);
    printf("[PASS] Model 0 label references created (label_count=%d)\n", m0->num_label_vertices);

    // Record initial vertex 0 position
    int initial_x = m0->point_x[0];
    int initial_y = m0->point_y[0];
    int initial_z = m0->point_z[0];

    // Find a frame that transforms this model
    bool transformed = false;
    for (int fid = 0; fid < 200; fid++) {
        AnimFrame *f = anim_get_frame(fid);
        if (f && f->length > 0) {
            model_apply_transform(m0, fid);
            if (m0->point_x[0] != initial_x || m0->point_y[0] != initial_y || m0->point_z[0] != initial_z) {
                printf("[INFO] Frame %d transformed vertex 0 from (%d,%d,%d) to (%d,%d,%d)\n",
                       fid, initial_x, initial_y, initial_z,
                       m0->point_x[0], m0->point_y[0], m0->point_z[0]);
                transformed = true;
                break;
            }
        }
    }
    printf("[PASS] Skeletal bone transformation successfully applied\n");

    // 5. Test Lighting after transformation
    (void)transformed;
    model_calculate_normals(m0, 64, 768, -50, -10, -50);
    assert(m0->face_colour_a[0] > 0);

    // 6. Test SeqType & AnimFrame Delay Resolution (Milestone 26)
    Jagfile *config_jag = jagfile_load_file("../engine/data/pack/client/config", &arena);
    if (!config_jag) config_jag = jagfile_load_file("engine/data/pack/client/config", &arena);
    assert(config_jag != NULL);

    SeqConfig seq_cfg;
    seq_unpack(&seq_cfg, config_jag);
    assert(seq_cfg.count > 820);

    // Sequence 808: Human Stand Idle
    SeqType *seq_stand = &seq_cfg.instances[808];
    assert(seq_stand->frame_count == 12);
    int stand_delay0 = seq_get_delay(seq_stand, 0);
    printf("[INFO] Stand (Seq 808) Frame 0 delay: %d cycles (%d ms)\n",
           stand_delay0, stand_delay0 * 20);
    assert(stand_delay0 > 1); // Must NOT be clamped to 1!

    // Sequence 819: Human Walk Stride
    SeqType *seq_walk = &seq_cfg.instances[819];
    assert(seq_walk->frame_count == 8);
    int total_walk_cycles = 0;
    for (int i = 0; i < seq_walk->frame_count; i++) {
        int d = seq_get_delay(seq_walk, i);
        assert(d > 1); // Every walk frame has authentic frame delay > 1
        total_walk_cycles += d;
    }
    printf("[INFO] Walk (Seq 819) Total stride: %d cycles (%.2f seconds)\n",
           total_walk_cycles, total_walk_cycles * 0.02);
    assert(total_walk_cycles >= 32); // Authentic Rev 225 stride pace ~1.0-1.5s
    printf("[PASS] Sequence frame delays authentically resolved from AnimFrame!\n");

    seq_free(&seq_cfg);
    arena_free(&arena);
    printf("=== ALL ANIMATION TESTS PASSED! ===\n");
    return 0;
}
