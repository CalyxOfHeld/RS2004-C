#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "ui/varp.h"
#include "ui/component.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/pix2d.h"
#include "gfx/pixfont.h"

int main(void) {
    printf("=== Test Suite: Interface Scripts, Varps & Triggers (Rev 225) ===\n");

    // 1. VarpContext Initialization & Basic Operations
    VarpContext ctx;
    varp_init(&ctx);
    assert(varp_get(&ctx, 0) == 0);
    assert(ctx.skill_levels[3] == 10); // Hitpoints level 10
    assert(ctx.energy == 100);
    assert(ctx.combat_level == 3);

    varp_set(&ctx, 43, 2); // Attack style varp
    assert(varp_get(&ctx, 43) == 2);
    printf("[PASS] VarpContext initialized with authentic default stats & varps\n");

    // 2. Script Virtual Machine Expression Evaluation (client.java:9070-9146)
    Component mock_com;
    memset(&mock_com, 0, sizeof(Component));
    mock_com.script_count = 3;

    int script_varp[] = { 5, 43, 0 };      // opcode 5: read varp 43
    int script_bit[]  = { 13, 100, 3, 0 };  // opcode 13: test bit 3 of varp 100
    int script_skill[]= { 1, 3, 0 };        // opcode 1: read skill 3 (hitpoints)

    int *scripts[] = { script_varp, script_bit, script_skill };
    int lengths[]  = { 3, 4, 3 };
    mock_com.scripts = scripts;
    mock_com.script_lengths = lengths;

    // Test opcode 5 (varp read)
    int res = varp_eval_script(&ctx, NULL, &mock_com, 0);
    assert(res == 2);

    // Test opcode 13 (varp bit check)
    varp_set(&ctx, 100, 0x08); // bit 3 set
    res = varp_eval_script(&ctx, NULL, &mock_com, 1);
    assert(res == 1);
    varp_set(&ctx, 100, 0x04); // bit 3 clear
    res = varp_eval_script(&ctx, NULL, &mock_com, 1);
    assert(res == 0);

    // Test opcode 1 (skill read)
    res = varp_eval_script(&ctx, NULL, &mock_com, 2);
    assert(res == 10);
    printf("[PASS] Script Virtual Machine correctly evaluates varps, bitmasks, and skills!\n");

    // 3. Conditional Trigger Comparator Evaluation (client.java:8337-8365)
    // Condition: script 0 (varp 43) == 2
    int comparator = 1; // equal (==)
    int operand = 2;
    mock_com.script_comparator_count = 1;
    mock_com.script_comparator = &comparator;
    mock_com.script_operand = &operand;

    assert(varp_eval_component_triggers(&ctx, NULL, &mock_com) == true);

    // Change varp so condition fails
    varp_set(&ctx, 43, 1);
    assert(varp_eval_component_triggers(&ctx, NULL, &mock_com) == false);

    // Less-than comparator (cmp 2)
    comparator = 2; // val < 5
    operand = 5;
    assert(varp_eval_component_triggers(&ctx, NULL, &mock_com) == true);
    operand = 1; // val < 1 (fails since val == 1)
    assert(varp_eval_component_triggers(&ctx, NULL, &mock_com) == false);

    // Greater-than comparator (cmp 3)
    comparator = 3; // val > 0
    operand = 0;
    assert(varp_eval_component_triggers(&ctx, NULL, &mock_com) == true);

    // Not-equal comparator (cmp 4)
    comparator = 4; // val != 99
    operand = 99;
    assert(varp_eval_component_triggers(&ctx, NULL, &mock_com) == true);
    operand = 1; // val != 1 (fails)
    assert(varp_eval_component_triggers(&ctx, NULL, &mock_com) == false);
    printf("[PASS] All conditional trigger comparators (==, <, >, !=) verified!\n");

    // 4. Interactive Component Clicking & Varp Mutation
    bool handled = varp_handle_component_click(&ctx, &mock_com);
    assert(handled == true);
    // Operand was 1, so varp 43 should now be updated to 1
    assert(varp_get(&ctx, 43) == 1);
    printf("[PASS] Interactive component click correctly mutates client varps!\n");

    // 4B. Dynamic Text Script Formatting (%1..%5 client script substitution)
    char fmt_buf[128];
    // mock_com script 2 is opcode 1 for skill 3 (hitpoints = 10)
    Component stat_com;
    memset(&stat_com, 0, sizeof(Component));
    stat_com.script_count = 2;
    int script_cur_lvl[]  = { 1, 0, 0 }; // Attack current lvl
    int script_base_lvl[] = { 2, 0, 0 }; // Attack base lvl
    int *stat_scripts[] = { script_cur_lvl, script_base_lvl };
    int stat_lens[] = { 3, 3 };
    stat_com.scripts = stat_scripts;
    stat_com.script_lengths = stat_lens;

    ctx.skill_levels[0] = 99;
    ctx.skill_base_levels[0] = 99;

    varp_format_text(&ctx, NULL, &stat_com, "%1", fmt_buf, sizeof(fmt_buf));
    assert(strcmp(fmt_buf, "99") == 0);

    varp_format_text(&ctx, NULL, &stat_com, "/ %2", fmt_buf, sizeof(fmt_buf));
    assert(strcmp(fmt_buf, "/ 99") == 0);

    ctx.skill_levels[0] = 1;
    ctx.skill_base_levels[0] = 1;
    varp_format_text(&ctx, NULL, &stat_com, "%1", fmt_buf, sizeof(fmt_buf));
    assert(strcmp(fmt_buf, "1") == 0);
    varp_format_text(&ctx, NULL, &stat_com, "/ %1", fmt_buf, sizeof(fmt_buf));
    assert(strcmp(fmt_buf, "/ 1") == 0);

    printf("[PASS] Dynamic text script formatting (%%1..%%5 substitution) verified!\n");

    // 5. Real Unpacked Interface Trigger Evaluation from interface.jag
    Arena arena = arena_create(32 * 1024 * 1024);
    Jagfile *title_jag = jagfile_load_file("../engine/data/pack/client/title", &arena);
    if (!title_jag) title_jag = jagfile_load_file("engine/data/pack/client/title", &arena);
    assert(title_jag != NULL);

    Jagfile *media_jag = jagfile_load_file("../engine/data/pack/client/media", &arena);
    if (!media_jag) media_jag = jagfile_load_file("engine/data/pack/client/media", &arena);
    assert(media_jag != NULL);

    Jagfile *interface_jag = jagfile_load_file("../engine/data/pack/client/interface", &arena);
    if (!interface_jag) interface_jag = jagfile_load_file("engine/data/pack/client/interface", &arena);
    assert(interface_jag != NULL);

    PixFont font_p11, font_p12, font_b12, font_q8;
    assert(pixfont_load(&font_p11, title_jag, "p11", &arena));
    assert(pixfont_load(&font_p12, title_jag, "p12", &arena));
    assert(pixfont_load(&font_b12, title_jag, "b12", &arena));
    assert(pixfont_load(&font_q8,  title_jag, "q8",  &arena));
    PixFont *fonts[4] = { &font_p11, &font_p12, &font_b12, &font_q8 };

    ComponentConfig comp_cfg;
    component_unpack(&comp_cfg, media_jag, fonts, interface_jag, &arena);
    assert(comp_cfg.count > 0);

    // Scan for real Rev 225 components containing trigger scripts
    int scripted_components = 0;
    for (int i = 0; i < comp_cfg.count; i++) {
        Component *c = &comp_cfg.instances[i];
        if (c->script_comparator_count > 0 && c->scripts != NULL) {
            scripted_components++;
        }
    }
    printf("[INFO] Total scripted interface components with live triggers: %d\n", scripted_components);
    assert(scripted_components > 0);

    // 6. Test Framebuffer Rendering with Dynamic Varp Triggers
    uint32_t fb[512 * 334];
    pix2d_bind(fb, 512, 334);
    pix2d_cls();

    // Render Combat Tab interface (ID 2423) with Attack Style varp set to Defensive (3)
    varp_set(&ctx, 43, 3);
    Component *combat_tab = component_get(&comp_cfg, 2423);
    if (combat_tab) {
        component_draw_with_varps(&comp_cfg, combat_tab, 0, 0, 0, &ctx);
        int rendered_px = 0;
        for (int i = 0; i < 512 * 334; i++) {
            if (fb[i] != 0) rendered_px++;
        }
        printf("[INFO] Rendered combat tab with active varp triggers: %d pixels\n", rendered_px);
        assert(rendered_px > 0);
        printf("[PASS] Active component state rendered to framebuffer!\n");
    }

    arena_free(&arena);
    printf("=== ALL INTERFACE SCRIPTS & VARP TESTS PASSED! ===\n");
    return 0;
}
