#include <stdio.h>
#include <stdlib.h>
#include "ui/component.h"
#include "ui/varp.h"
#include "core/arena.h"
#include "gfx/pix2d.h"
#include "gfx/pixfont.h"
#include <SDL2/SDL.h>

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    Arena arena = arena_create(16 * 1024 * 1024);
    Jagfile *interface_jag = jagfile_load_file("../engine/data/pack/client/interface", &arena);
    if (!interface_jag) interface_jag = jagfile_load_file("engine/data/pack/client/interface", &arena);
    Jagfile *media_jag = jagfile_load_file("../engine/data/pack/client/media", &arena);
    if (!media_jag) media_jag = jagfile_load_file("engine/data/pack/client/media", &arena);
    Jagfile *title_jag = jagfile_load_file("../engine/data/pack/client/title", &arena);
    if (!title_jag) title_jag = jagfile_load_file("engine/data/pack/client/title", &arena);

    PixFont font_p11, font_p12, font_b12, font_q8;
    pixfont_load(&font_p11, title_jag, "p11", &arena);
    pixfont_load(&font_p12, title_jag, "p12", &arena);
    pixfont_load(&font_b12, title_jag, "b12", &arena);
    pixfont_load(&font_q8, title_jag, "q8", &arena);

    PixFont *fonts[4] = { &font_p11, &font_p12, &font_b12, &font_q8 };
    ComponentConfig cfg;
    component_unpack(&cfg, media_jag, fonts, interface_jag, &arena);

    uint32_t pixels[789 * 532] = {0};
    pix2d_bind(pixels, 789, 532);

    Component *com = component_get(&cfg, 962);
    VarpContext varps;
    varp_init(&varps);

    // Call draw
    component_draw_with_varps(&cfg, com, 562, 231, 0, &varps);

    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels, 789, 532, 32, 789 * sizeof(uint32_t), SDL_PIXELFORMAT_RGB888);
    if (surface) {
        SDL_SaveBMP(surface, "verification/test_music_tab.bmp");
        SDL_FreeSurface(surface);
        printf("Saved verification/test_music_tab.bmp\n");
    }
    return 0;
}
