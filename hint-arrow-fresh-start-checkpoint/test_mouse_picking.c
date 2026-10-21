#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "ui/mouse_picking.h"
#include "ui/context_menu.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/model.h"

int main(void) {
    printf("=== Test Suite: 3D Mouse Picking & Entity Hovering ===\n");

    // 1. Bitset encoding & decoding
    // Type 1 (NPC), ID 0 (Hans), Tile (34, 34)
    uint32_t npc_bitset = mouse_picker_make_bitset(1, 0, 34, 34);
    assert(((npc_bitset >> 29) & 0x3) == 1);
    assert(((npc_bitset >> 14) & 0x7FFF) == 0);
    assert(((npc_bitset >> 7) & 0x7F) == 34);
    assert((npc_bitset & 0x7F) == 34);
    printf("[PASS] Bitset encoding & bit-field extraction verified (type=1, id=0, tile=34,34)\n");

    // Type 2 (Loc), ID 1530 (Door), Tile (32, 33)
    uint32_t loc_bitset = mouse_picker_make_bitset(2, 1530, 32, 33);
    assert(((loc_bitset >> 29) & 0x3) == 2);
    assert(((loc_bitset >> 14) & 0x7FFF) == 1530);
    assert(((loc_bitset >> 7) & 0x7F) == 33);
    assert((loc_bitset & 0x7F) == 32);
    printf("[PASS] Bitset encoding for scenery location verified (loc_id=1530)\n");

    // 2. Mouse Picker State
    MousePicker picker;
    mouse_picker_init(&picker);
    assert(!picker.check_hover);
    assert(picker.picked_count == 0);

    mouse_picker_reset(&picker, 250, 180);
    assert(picker.check_hover);
    assert(picker.mouse_x == 250);
    assert(picker.mouse_y == 180);

    mouse_picker_add_picked(&picker, loc_bitset);
    mouse_picker_add_picked(&picker, npc_bitset);
    assert(picker.picked_count == 2);

    // Duplicate bitset should not be added twice
    mouse_picker_add_picked(&picker, npc_bitset);
    assert(picker.picked_count == 2);
    printf("[PASS] MousePicker duplicate rejection and hover state verified\n");

    // 3. Dynamic Menu Construction
    ContextMenu menu;
    context_menu_init(&menu);

    // Create a mock Hans NPC
    NpcEntity hans;
    memset(&hans, 0, sizeof(NpcEntity));
    hans.has_type = true;
    strncpy(hans.type.name, "Hans", sizeof(hans.type.name));
    hans.type.vislevel = 0;
    hans.type.op_valid[0] = true;
    strncpy(hans.type.op[0], "Talk-to", sizeof(hans.type.op[0]));

    mouse_picker_build_menu(&picker, &menu, NULL, NULL, NULL, &hans, 1, NULL, 0, 3);
    assert(menu.size > 0);
    printf("[INFO] Constructed Context Menu Options (%d total):\n", menu.size);
    for (int i = 0; i < menu.size; i++) {
        printf("  [%d] %s (action=%d)\n", i, menu.options[i], menu.action_ids[i]);
    }

    // Topmost option should be "Talk-to @yel@Hans"
    const char *primary = mouse_picker_get_primary_action(&menu);
    printf("[INFO] Primary Action Text: '%s'\n", primary);
    assert(strstr(primary, "Talk-to") != NULL);
    assert(strstr(primary, "Hans") != NULL);
    printf("[PASS] Primary hover action dynamically resolves to Talk-to Hans!\n");

    // Cancel option must always be at index 0 (bottom)
    assert(strcmp(menu.options[0], "Cancel") == 0);
    printf("[PASS] Context Menu bottom Cancel option verified\n");

    // 4. Test 3D Model Pickable Hit Testing
    Arena arena = arena_create(16 * 1024 * 1024);
    Jagfile *models_jag = jagfile_load_file("../engine/data/pack/client/models", &arena);
    if (!models_jag) models_jag = jagfile_load_file("engine/data/pack/client/models", &arena);
    assert(models_jag != NULL);
    model_init_repo(models_jag, &arena);

    Model *m = model_load(0, &arena);
    assert(m != NULL);
    model_calculate_normals(m, 64, 768, -50, -10, -50);

    uint32_t fb[512 * 334];
    pix2d_bind(fb, 512, 334);
    pix2d_cls();
    pix3d_init();
    pix3d_init_colour_table(0.8);
    pix3d_init_2d();
    pix3d_center_x = 256;
    pix3d_center_y = 167;

    // Pick test at screen center (256, 167) where model 0 is drawn at distance 400
    bool hit_center = model_draw_pickable(m, 0, 0, 0, 0, 0, 400, 256, 167);
    printf("[INFO] Model 0 pick test at center (256, 167): hit=%d\n", hit_center);
    assert(hit_center == true);

    // Pick test outside model bounds (10, 10)
    bool hit_outside = model_draw_pickable(m, 0, 0, 0, 0, 0, 400, 10, 10);
    printf("[INFO] Model 0 pick test outside bounds (10, 10): hit=%d\n", hit_outside);
    assert(hit_outside == false);
    printf("[PASS] Pixel-perfect 3D triangle hit testing verified!\n");

    arena_free(&arena);
    printf("=== ALL MOUSE PICKING TESTS PASSED! ===\n");
    return 0;
}
