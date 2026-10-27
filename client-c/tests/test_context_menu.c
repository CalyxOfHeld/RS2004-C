#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/pix2d.h"
#include "gfx/pixfont.h"
#include "ui/context_menu.h"
#include "../verification/completion/tools/mouse_mode_reference_control.h"

int main(void) {
    setbuf(stdout, NULL);
    printf("=== Test Suite: Right-Click Context Menu & Interaction ===\n");

    Arena arena = arena_create(16 * 1024 * 1024);

    // 1. Load Font from title.jag
    Jagfile *title_jag = jagfile_load_file("../engine/data/pack/client/title", &arena);
    if (!title_jag) title_jag = jagfile_load_file("engine/data/pack/client/title", &arena);
    assert(title_jag != NULL);

    PixFont font_b12;
    assert(pixfont_load(&font_b12, title_jag, "b12", &arena));
    printf("[PASS] Font b12 loaded for context menu\n");

    // 2. Initialize Menu
    ContextMenu menu;
    context_menu_init(&menu);
    assert(!menu.visible);

    // 3. Populate Options
    context_menu_add_option(&menu, "Cancel", 0);
    context_menu_add_option(&menu, "Walk here", 1);
    context_menu_add_option(&menu, "Examine @yel@Hans", 2);
    context_menu_add_option(&menu, "Talk-to @yel@Hans", 3);
    assert(menu.size == 4);
    printf("[PASS] 4 Context Menu options added\n");

    // 4. Open Menu at (200, 150)
    context_menu_open(&menu, 200, 150, &font_b12);
    assert(menu.visible);
    assert(menu.width > 100);
    assert(menu.height > 70);
    printf("[INFO] Context Menu opened: %dx%d at (%d, %d)\n",
           menu.width, menu.height, menu.x, menu.y);

    // 5. Render Menu to Framebuffer
    int fb_w = 765, fb_h = 503;
    uint32_t *fb = (uint32_t *)calloc(fb_w * fb_h, sizeof(uint32_t));
    pix2d_bind(fb, fb_w, fb_h);
    pix2d_cls();

    context_menu_draw(&menu, menu.x + 10, menu.y + 25, &font_b12);

    int menu_pixels = 0;
    for (int y = menu.y; y < menu.y + menu.height; y++) {
        for (int x = menu.x; x < menu.x + menu.width; x++) {
            if (fb[x + y * fb_w] != 0) menu_pixels++;
        }
    }
    printf("[INFO] Menu rendered pixels: %d\n", menu_pixels);
    assert(menu_pixels > 500);
    printf("[PASS] Context Menu correctly rendered with border and header\n");

    // 6. Test Option Selection Click
    // Top option is index 3 ("Talk-to @yel@Hans" action 3) at y = menu.y + 18 + 5
    int act = context_menu_handle_click(&menu, menu.x + 20, menu.y + 25);
    assert(act == 3);
    assert(!menu.visible);
    printf("[PASS] Context Menu click dispatched Action ID %d ('Talk-to @yel@Hans')\n", act);

    // 7. Test Click Outside
    context_menu_open(&menu, 200, 150, &font_b12);
    assert(menu.visible);
    int cancel_act = context_menu_handle_click(&menu, 10, 10);
    assert(cancel_act == -1);
    assert(!menu.visible);
    printf("[PASS] Click outside dismisses Context Menu\n");

    // Original Java uses the same strict bounds for highlighting and clicking.
    // The last highlighted pixel of the top row must not pick the next row.
    MenuAction action;
    context_menu_open(&menu, 200, 150, &font_b12);
    assert(context_menu_pick_action(&menu, menu.x + 20, menu.y + 33, &action));
    assert(action.target == 3);
    context_menu_open(&menu, 200, 150, &font_b12);
    assert(context_menu_handle_click(&menu, menu.x + 20, menu.y + 33) == 3);
    context_menu_open(&menu, 200, 150, &font_b12);
    assert(context_menu_handle_click(&menu, menu.x + 20, menu.y + 34) == 2);
    context_menu_open(&menu, 200, 150, &font_b12);
    assert(!context_menu_pick_action(&menu, menu.x + 20, menu.y + 18, &action));
    context_menu_open(&menu, 200, 150, &font_b12);
    assert(context_menu_handle_click(&menu, menu.x, menu.y + 25) == -1);
    puts("[PASS] Menu row/header/left-border hit boundaries match original Java");

    mouse_mode_registered(&font_b12);
    free(fb);
    arena_free(&arena);
    printf("=== ALL CONTEXT MENU TESTS PASSED! ===\n");
    return 0;
}
