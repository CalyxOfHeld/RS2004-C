#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/pix2d.h"
#include "gfx/pixfont.h"
#include "gfx/media.h"

int main(void) {
    setbuf(stdout, NULL);
    printf("=== Test Suite: 2D Media UI & Bitmap Fonts ===\n");

    Arena arena = arena_create(32 * 1024 * 1024);

    // 1. Load Jagfiles
    Jagfile *title_jag = jagfile_load_file("../engine/data/pack/client/title", &arena);
    if (!title_jag) title_jag = jagfile_load_file("engine/data/pack/client/title", &arena);
    assert(title_jag != NULL);
    printf("[PASS] title.jag loaded successfully\n");

    Jagfile *media_jag = jagfile_load_file("../engine/data/pack/client/media", &arena);
    if (!media_jag) media_jag = jagfile_load_file("engine/data/pack/client/media", &arena);
    assert(media_jag != NULL);
    printf("[PASS] media.jag loaded successfully\n");

    // 2. Load Bitmap Fonts from title.jag
    PixFont font_p11, font_p12, font_b12, font_q8;
    bool ok_p11 = pixfont_load(&font_p11, title_jag, "p11", &arena);
    assert(ok_p11);
    bool ok_p12 = pixfont_load(&font_p12, title_jag, "p12", &arena);
    assert(ok_p12);
    bool ok_b12 = pixfont_load(&font_b12, title_jag, "b12", &arena);
    assert(ok_b12);
    bool ok_q8  = pixfont_load(&font_q8, title_jag, "q8", &arena);
    assert(ok_q8);
    printf("[PASS] 4 RS2 Bitmap Fonts unpacked (p11, p12, b12, q8)\n");

    // Test string widths
    int w_test = pixfont_string_width(&font_b12, "Choose Option");
    printf("[INFO] 'Choose Option' width in b12 font: %d px\n", w_test);
    assert(w_test > 50 && w_test < 150);

    // 3. Load Media Sprites from media.jag
    Media media;
    bool ok_media = media_load(&media, media_jag, &arena);
    assert(ok_media);
    assert(media.chatback != NULL);
    assert(media.invback != NULL);
    assert(media.mapback != NULL);
    assert(media.backbase1 != NULL);
    assert(media.backbase2 != NULL);
    assert(media.sideicons[3] != NULL); // Inventory tab icon
    assert(media.redstone1 != NULL);
    printf("[PASS] Full 2004scape Media artwork loaded (frames, tabs, compass, icons)\n");

    // 4. Render Shell to Framebuffer
    int fb_w = 765, fb_h = 503;
    uint32_t *fb = (uint32_t *)calloc(fb_w * fb_h, sizeof(uint32_t));
    pix2d_bind(fb, fb_w, fb_h);
    pix2d_cls();

    // Draw full game frame shell
    media_draw_shell(&media);

    // Draw tabs (with Inventory tab 3 selected)
    media_draw_tabs(&media, 3);

    // Draw chat text using font engine
    pixfont_draw_string_taggable(&font_p12, 30, 400, 0x000000, "@yel@Welcome to RuneScape.", true);
    pixfont_draw_string_taggable(&font_p12, 30, 415, 0x000000, "@whi@Hans: @bla@Hello, adventurer! Welcome to Lumbridge.", false);

    // Verify chatbox area has non-zero pixels
    int chat_pixels = 0;
    for (int y = 375; y < 470; y++) {
        for (int x = 25; x < 490; x++) {
            if (fb[x + y * fb_w] != 0) chat_pixels++;
        }
    }
    printf("[INFO] Chatbox rendered pixels: %d\n", chat_pixels);
    assert(chat_pixels > 10000);

    // Verify sidebar area has non-zero pixels
    int sidebar_pixels = 0;
    for (int y = 235; y < 460; y++) {
        for (int x = 565; x < 740; x++) {
            if (fb[x + y * fb_w] != 0) sidebar_pixels++;
        }
    }
    printf("[INFO] Sidebar rendered pixels: %d\n", sidebar_pixels);
    assert(sidebar_pixels > 10000);

    // Verify mapback area has non-zero pixels
    int mapback_pixels = 0;
    for (int y = 10; y < 150; y++) {
        for (int x = 565; x < 740; x++) {
            if (fb[x + y * fb_w] != 0) mapback_pixels++;
        }
    }
    printf("[INFO] Mapback rendered pixels: %d\n", mapback_pixels);
    assert(mapback_pixels > 5000);

    free(fb);
    arena_free(&arena);

    printf("=== ALL MEDIA & UI TESTS PASSED! ===\n");
    return 0;
}
