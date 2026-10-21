#ifndef RS2_PIXFONT_H
#define RS2_PIXFONT_H

#include <stdint.h>
#include <stdbool.h>
#include "core/arena.h"
#include "core/jagfile.h"

typedef struct {
    int height;
    uint8_t *char_mask[94];
    int char_mask_width[94];
    int char_mask_height[94];
    int char_offset_x[94];
    int char_offset_y[94];
    int char_advance[95];
    int draw_width[256];
} PixFont;

void pixfont_init_lookup(void);
bool pixfont_load(PixFont *font, Jagfile *title_jag, const char *name, Arena *arena);
int  pixfont_string_width(PixFont *font, const char *str);
void pixfont_draw_string(PixFont *font, int x, int y, uint32_t color, const char *str);
void pixfont_draw_string_center(PixFont *font, int x, int y, uint32_t color, const char *str);
void pixfont_draw_string_right(PixFont *font, int x, int y, uint32_t color, const char *str);
void pixfont_draw_string_taggable(PixFont *font, int x, int y, uint32_t default_color, const char *str, bool shadow);
void pixfont_draw_string_taggable_center(PixFont *font, int x, int y, uint32_t default_color, const char *str, bool shadow);

#endif // RS2_PIXFONT_H
