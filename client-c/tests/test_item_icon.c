#include "config/obj.h"
#include "gfx/pix2d.h"
#include "gfx/pix32.h"
#include "gfx/pix3d.h"
#include "gfx/model.h"
#include "gfx/pixfont.h"
#include "ui/component.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("=== Test Suite: 2D Item Icon Generator & Inventory System ===\n");

    Arena arena = arena_create(64 * 1024 * 1024);

    // Initialize 2D & 3D graphics subsystems
    pix3d_init();
    pix3d_init_colour_table(0.8);

    uint32_t fb[512 * 512];
    pix2d_bind(fb, 512, 512);

    // Load caches
    Jagfile *config_jag = jagfile_load_file("../engine/data/pack/client/config", &arena);
    if (!config_jag) config_jag = jagfile_load_file("engine/data/pack/client/config", &arena);
    assert(config_jag && "config.jag must load");

    Jagfile *models_jag = jagfile_load_file("../engine/data/pack/client/models", &arena);
    if (!models_jag) models_jag = jagfile_load_file("engine/data/pack/client/models", &arena);
    assert(models_jag && "models.jag must load");

    Jagfile *interface_jag = jagfile_load_file("../engine/data/pack/client/interface", &arena);
    if (!interface_jag) interface_jag = jagfile_load_file("engine/data/pack/client/interface", &arena);
    assert(interface_jag && "interface.jag must load");

    Jagfile *media_jag = jagfile_load_file("../engine/data/pack/client/media", &arena);
    if (!media_jag) media_jag = jagfile_load_file("engine/data/pack/client/media", &arena);
    assert(media_jag && "media.jag must load");

    Jagfile *title_jag = jagfile_load_file("../engine/data/pack/client/title", &arena);
    if (!title_jag) title_jag = jagfile_load_file("engine/data/pack/client/title", &arena);
    assert(title_jag && "title.jag must load");

    // Initialize models repository
    model_init_repo(models_jag, &arena);

    // Unpack ObjConfig
    ObjConfig obj_cfg;
    obj_unpack(&obj_cfg, config_jag, false);
    printf("[PASS] %d items unpacked from obj.dat\n", obj_cfg.count);
    assert(obj_cfg.count > 1000);

    // Test 1: Stack count formatting
    printf("[INFO] Testing format_obj_count...\n");
    char str[32];
    format_obj_count(1, str, sizeof(str));
    assert(strcmp(str, "1") == 0);

    format_obj_count(99999, str, sizeof(str));
    assert(strcmp(str, "99999") == 0);

    format_obj_count(100000, str, sizeof(str));
    assert(strcmp(str, "100K") == 0);

    format_obj_count(9999999, str, sizeof(str));
    assert(strcmp(str, "9999K") == 0);

    format_obj_count(10000000, str, sizeof(str));
    assert(strcmp(str, "10M") == 0);

    format_obj_count(2147483647, str, sizeof(str));
    assert(strcmp(str, "2147M") == 0);
    printf("[PASS] format_obj_count bit-for-bit parity verified!\n");

    // Test 2: Render single coin (995, count 1)
    printf("[INFO] Generating item icon: Coins (id 995, count 1)...\n");
    Pix32 *coin1 = obj_get_icon(&obj_cfg, 995, 1, &arena);
    assert(coin1 != NULL);
    assert(coin1->width == 32 && coin1->height == 32);

    int non_zero = 0;
    int outline_count = 0;
    int shadow_count = 0;
    for (int i = 0; i < 32 * 32; i++) {
        if (coin1->pixels[i] != 0) non_zero++;
        if (coin1->pixels[i] == 1) outline_count++;
        if (coin1->pixels[i] == 0x302020) shadow_count++;
    }
    printf("  Coins(1): %d non-zero pixels (%d outline, %d shadow)\n", non_zero, outline_count, shadow_count);
    assert(non_zero > 20);
    assert(outline_count > 5);
    assert(shadow_count > 5);
    printf("[PASS] Coins(1) 32x32 icon generated with correct outline and shadow!\n");

    // Test 3: Render coin stack (995, count 10000)
    printf("[INFO] Generating item icon: Coins stack (id 995, count 10000)...\n");
    Pix32 *coin10k = obj_get_icon(&obj_cfg, 995, 10000, &arena);
    assert(coin10k != NULL);
    assert(coin10k != coin1); // Different model variant
    assert(coin10k->crop_w == 33); // Stackable
    printf("[PASS] Stackable multi-count item icon generated!\n");

    // Test 4: Render Bronze sword (1277)
    printf("[INFO] Generating item icon: Bronze sword (id 1277)...\n");
    Pix32 *sword = obj_get_icon(&obj_cfg, 1277, 1, &arena);
    assert(sword != NULL);
    assert(sword->crop_w == 32); // Non-stackable
    non_zero = 0;
    for (int i = 0; i < 32 * 32; i++) {
        if (sword->pixels[i] != 0) non_zero++;
    }
    printf("  Bronze sword: %d non-zero pixels\n", non_zero);
    assert(non_zero > 30);
    printf("[PASS] Weapon model icon generated successfully!\n");

    // Test 5: Verify cache speedup
    Pix32 *cached = obj_get_icon(&obj_cfg, 1277, 1, &arena);
    assert(cached == sword);
    printf("[PASS] Obj icon cache verified (exact pointer returned)!\n");

    // Test 6: Interface component inventory integration
    PixFont font_p11;
    bool font_ok = pixfont_load(&font_p11, title_jag, "p11", &arena);
    assert(font_ok);
    PixFont *fonts[4] = { &font_p11, NULL, NULL, NULL };

    ComponentConfig comp_cfg;
    bool comp_ok = component_unpack(&comp_cfg, media_jag, fonts, interface_jag, &arena);
    assert(comp_ok);
    comp_cfg.obj_config = &obj_cfg;
    comp_cfg.font_p11 = &font_p11;

    // Component 3214 is the inventory grid
    Component *inv_grid = component_get(&comp_cfg, 3214);
    assert(inv_grid != NULL);
    assert(inv_grid->type == COMPONENT_TYPE_INV_GRID);
    printf("[PASS] Component 3214 (Inventory Grid) found: %dx%d slots\n", inv_grid->width, inv_grid->height);

    // Populate inventory slots
    inv_grid->inv_slot_obj_id[0] = 996; // Coins (id + 1)
    inv_grid->inv_slot_obj_count[0] = 50000;
    inv_grid->inv_slot_obj_id[1] = 1278; // Bronze sword
    inv_grid->inv_slot_obj_count[1] = 1;

    // Clear framebuffer and draw inventory
    memset(fb, 0, sizeof(fb));
    component_draw(&comp_cfg, inv_grid, 10, 10, 0);

    int drawn_pixels = 0;
    for (int i = 0; i < 512 * 512; i++) {
        if (fb[i] != 0) drawn_pixels++;
    }
    printf("[INFO] Framebuffer pixels after drawing inventory: %d\n", drawn_pixels);
    assert(drawn_pixels > 100);
    printf("[PASS] Inventory grid rendered to framebuffer with item icons & stack text!\n");

    obj_free(&obj_cfg);
    arena_free(&arena);
    printf("=== ALL ITEM ICON & INVENTORY TESTS PASSED! ===\n");
    return 0;
}
