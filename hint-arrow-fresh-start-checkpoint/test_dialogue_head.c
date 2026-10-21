#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "core/arena.h"
#include "core/jagfile.h"
#include "config/idk.h"
#include "config/obj.h"
#include "entity/player_entity.h"
#include "gfx/model.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"

int main(void) {
    setbuf(stdout, NULL);
    printf("=== Test Suite: Player Head Model & Dialogue Rendering ===\n");

    Arena arena = arena_create(48 * 1024 * 1024);

    // 1. Load Config Archive
    Jagfile *config_jag = jagfile_load_file("../engine/data/pack/client/config", &arena);
    if (!config_jag) config_jag = jagfile_load_file("engine/data/pack/client/config", &arena);
    assert(config_jag && "config.jag must load");

    Jagfile *models_jag = jagfile_load_file("../engine/data/pack/client/models", &arena);
    if (!models_jag) models_jag = jagfile_load_file("engine/data/pack/client/models", &arena);
    assert(models_jag && "models.jag must load");

    // Init Model loader
    model_init_repo(models_jag, &arena);

    // Unpack IDK config
    IdkConfig idk_cfg;
    idk_unpack(&idk_cfg, config_jag, &arena);
    assert(idk_cfg.count > 0);
    printf("[PASS] IDK config unpacked: %d identities\n", idk_cfg.count);

    // Unpack Obj config
    ObjConfig obj_cfg;
    obj_unpack(&obj_cfg, config_jag, true);
    assert(obj_cfg.count > 0);
    printf("[PASS] Obj config unpacked: %d items\n", obj_cfg.count);

    // 2. Setup Player Entity with standard male appearance
    PlayerEntity player;
    player_entity_init(&player);
    player.visible = true;
    player.gender = 0; // Male

    // Standard hair / head identities
    player.appearances[0] = 256 + 0;  // Head identity 0
    player.appearances[1] = 256 + 10; // Beard identity 10
    player.colors[0] = 7; // Hair color
    player.colors[4] = 4; // Skin color

    // 3. Generate Player Head Model (Un-helmeted)
    Model *head_model = player_entity_get_head_model(&player, &idk_cfg, &obj_cfg, &arena);
    assert(head_model != NULL);
    assert(head_model->num_points > 50);
    assert(head_model->num_faces > 50);
    printf("[PASS] Player head composite model created: %d vertices, %d faces\n",
           head_model->num_points, head_model->num_faces);

    // 4. Test equipped helmet (Iron full helm or Medium helm)
    // Find an object with manhead != -1
    int helm_id = -1;
    for (int i = 0; i < obj_cfg.count; i++) {
        ObjType obj;
        if (obj_get(&obj_cfg, i, &obj)) {
            if (obj.manhead != -1) {
                helm_id = i;
                obj_free_entry(&obj);
                break;
            }
            obj_free_entry(&obj);
        }
    }
    assert(helm_id != -1 && "Must find a headwear item in cache");
    printf("[INFO] Found headwear item %d for helmet test\n", helm_id);

    // Equip helmet
    player.appearances[0] = 512 + helm_id;
    Model *helm_head_model = player_entity_get_head_model(&player, &idk_cfg, &obj_cfg, &arena);
    assert(helm_head_model != NULL);
    assert(helm_head_model->num_points > 0);
    printf("[PASS] Helmet headwear model successfully composited: %d vertices, %d faces\n",
           helm_head_model->num_points, helm_head_model->num_faces);

    // 5. Test Rasterization into 3D buffer
    int fb_w = 500, fb_h = 500;
    uint32_t *fb = (uint32_t *)calloc(fb_w * fb_h, sizeof(uint32_t));
    pix2d_bind(fb, fb_w, fb_h);
    pix2d_cls();
    pix3d_init();
    pix3d_init_colour_table(0.8);
    pix3d_init_2d();

    model_render_obj(head_model, 0, 512, 0, 0, 0, 0, 400);

    int drawn_pixels = 0;
    for (int i = 0; i < fb_w * fb_h; i++) {
        if (fb[i] != 0) drawn_pixels++;
    }
    printf("[PASS] Rasterized player head model to buffer: %d pixels drawn\n", drawn_pixels);
    assert(drawn_pixels > 100);

    free(fb);
    idk_free(&idk_cfg);
    obj_free(&obj_cfg);
    arena_free(&arena);

    printf("=== ALL PLAYER HEAD & DIALOGUE TESTS PASSED! ===\n");
    return 0;
}
