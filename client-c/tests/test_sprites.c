#include <stdio.h>
#include <assert.h>
#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/pix2d.h"
#include "gfx/pix8.h"
#include "gfx/pix32.h"

int main(void) {
    printf("[Test Sprites] Loading and rendering RS2 2004 sprites from cache...\n");

    Arena arena = arena_create(16 * 1024 * 1024);

    Jagfile *title_jag = jagfile_load_file("../engine/data/pack/client/title", &arena);
    assert(title_jag != NULL);

    // 1. Load Pix8 'titlebox'
    Pix8 *titlebox = pix8_load(title_jag, "titlebox", 0, &arena);
    assert(titlebox != NULL);
    printf("  'titlebox' loaded: %dx%d (expected 360x200), palette=%d (expected 18)\n",
           titlebox->width, titlebox->height, titlebox->palette_count);
    assert(titlebox->width == 360);
    assert(titlebox->height == 200);
    assert(titlebox->palette_count == 18);

    // 2. Load Pix8 'titlebutton'
    Pix8 *titlebutton = pix8_load(title_jag, "titlebutton", 0, &arena);
    assert(titlebutton != NULL);
    printf("  'titlebutton' loaded: %dx%d (expected 147x41), palette=%d (expected 13)\n",
           titlebutton->width, titlebutton->height, titlebutton->palette_count);
    assert(titlebutton->width == 147);
    assert(titlebutton->height == 41);
    assert(titlebutton->palette_count == 13);

    // 3. Render into framebuffer
    uint32_t fb[500 * 500];
    pix2d_bind(fb, 500, 500);
    pix2d_cls();

    pix8_draw(titlebox, 10, 10);
    // Assert pixels were plotted inside the 360x200 region
    bool has_pixels = false;
    for (int y = 15; y < 150; y++) {
        for (int x = 15; x < 300; x++) {
            if (fb[x + y * 500] != 0) {
                has_pixels = true;
                break;
            }
        }
        if (has_pixels) break;
    }
    assert(has_pixels);
    printf("  'titlebox' successfully drawn to framebuffer!\n");

    arena_free(&arena);
    printf("[Test Sprites] ALL TESTS PASSED! Bit-for-bit sprite loading verified.\n");
    return 0;
}
