#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/pix2d.h"
#include "gfx/pixfont.h"
#include "ui/component.h"

#include "../verification/completion/tools/interface_models_reference_control.h"
#include "../verification/completion/tools/last_login_reference_control.h"
#include "../verification/completion/tools/continue_reference_control.h"

int main(void) {
    setbuf(stdout, NULL);
    printf("=== Test Suite: Interface Component System (Rev 225) ===\n");

    Arena arena = arena_create(48 * 1024 * 1024);

    // 1. Load Archives
    Jagfile *title_jag = jagfile_load_file("../engine/data/pack/client/title", &arena);
    if (!title_jag) title_jag = jagfile_load_file("engine/data/pack/client/title", &arena);
    assert(title_jag != NULL);

    Jagfile *media_jag = jagfile_load_file("../engine/data/pack/client/media", &arena);
    if (!media_jag) media_jag = jagfile_load_file("engine/data/pack/client/media", &arena);
    assert(media_jag != NULL);

    Jagfile *interface_jag = jagfile_load_file("../engine/data/pack/client/interface", &arena);
    if (!interface_jag) interface_jag = jagfile_load_file("engine/data/pack/client/interface", &arena);
    assert(interface_jag != NULL);
    printf("[PASS] Archives loaded: title, media, interface\n");

    // 2. Load Fonts
    PixFont font_p11, font_p12, font_b12, font_q8;
    assert(pixfont_load(&font_p11, title_jag, "p11", &arena));
    assert(pixfont_load(&font_p12, title_jag, "p12", &arena));
    assert(pixfont_load(&font_b12, title_jag, "b12", &arena));
    assert(pixfont_load(&font_q8,  title_jag, "q8",  &arena));
    PixFont *fonts[4] = { &font_p11, &font_p12, &font_b12, &font_q8 };
    printf("[PASS] 4 RS2 Bitmap Fonts unpacked for interface system\n");

    // 3. Unpack Interface Components
    ComponentConfig comp_cfg;
    bool ok_comp = component_unpack(&comp_cfg, media_jag, fonts, interface_jag, &arena);
    assert(ok_comp);
    printf("[INFO] Total unpacked components: %d\n", comp_cfg.count);
    assert(comp_cfg.count > 3000);

    // 4. Verify Inventory Interface (3213 / 3214)
    Component *inv_layer = component_get(&comp_cfg, 3213);
    assert(inv_layer != NULL);
    printf("[INFO] Component 3213 (Inventory Layer): type=%d, child_count=%d\n",
           inv_layer->type, inv_layer->child_count);
    assert(inv_layer->type == COMPONENT_TYPE_CONTAINER);

    Component *inv_grid = component_get(&comp_cfg, 3214);
    assert(inv_grid != NULL);
    printf("[INFO] Component 3214 (Inventory Grid): type=%d, width=%d, height=%d (total slots=%d)\n",
           inv_grid->type, inv_grid->width, inv_grid->height, inv_grid->width * inv_grid->height);
    assert(inv_grid->type == COMPONENT_TYPE_INV_GRID);
    assert(inv_grid->width == 4 && inv_grid->height == 7); // 4 columns * 7 rows = 28 slots!

    // 5. Test Interface Rendering to Framebuffer
    int fb_w = 765, fb_h = 503;
    uint32_t *fb = (uint32_t *)calloc(fb_w * fb_h, sizeof(uint32_t));
    pix2d_bind(fb, fb_w, fb_h);
    pix2d_cls();

    // Render Inventory interface at sidebar position (562, 231)
    component_draw(&comp_cfg, inv_layer, 562, 231, 0);

    // Test text component rendering
    int text_comp_count = 0;
    for (int i = 0; i < comp_cfg.count; i++) {
        Component *c = &comp_cfg.instances[i];
        if (c->type == COMPONENT_TYPE_TEXT && c->text && strlen(c->text) > 0) {
            text_comp_count++;
            if (text_comp_count == 1) {
                printf("[INFO] Sample Text Component %d: '%s' (font_id=%d, colour=0x%06X)\n",
                       c->id, c->text, c->font_id, (unsigned int)c->colour);
                component_draw(&comp_cfg, c, 50, 50, 0);
            }
        }
    }
    printf("[INFO] Total text components with strings: %d\n", text_comp_count);
    assert(text_comp_count > 100);

    // Verify UI pixels rendered
    int ui_pixels = 0;
    for (int i = 0; i < fb_w * fb_h; i++) {
        if (fb[i] != 0) ui_pixels++;
    }
    printf("[INFO] Rendered interface pixels: %d\n", ui_pixels);
    assert(ui_pixels > 0);

    Arena model_arena=arena_create(64*1024*1024);
    Jagfile *model_jag=jagfile_load_file("../engine/data/pack/client/models",&model_arena),*config_jag=jagfile_load_file("../engine/data/pack/client/config",&model_arena);assert(model_jag && config_jag);model_init_repo(model_jag,&model_arena);ObjConfig objects={0};obj_unpack(&objects,config_jag,true);comp_cfg.obj_config=&objects;VarpContext varps={0};
    const char *reference=getenv("RS2_INTERFACE_MODEL_ORACLE");interface_models_reference_run(reference && *reference?reference:"tests/fixtures/interface-models-reference.bin",&comp_cfg,&varps,&arena,NULL);component_models_clear225(&comp_cfg);obj_free(&objects);arena_free(&model_arena);
    last_login_registered_reference(&comp_cfg);
    continue_registered_reference(&comp_cfg);
    free(fb);
    arena_free(&arena);
    printf("=== ALL INTERFACE COMPONENT TESTS PASSED! ===\n");
    return 0;
}
