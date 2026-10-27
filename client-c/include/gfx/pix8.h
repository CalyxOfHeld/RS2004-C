#ifndef RS2_PIX8_H
#define RS2_PIX8_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "core/arena.h"
#include "core/jagfile.h"

typedef struct {
    uint8_t  *pixels;
    int32_t   width;
    int32_t   height;
    int32_t   crop_x;
    int32_t   crop_y;
    int32_t   crop_w;
    int32_t   crop_h;
    uint32_t *palette;
    int32_t   palette_count;
} Pix8;

Pix8 *pix8_load(Jagfile *jag, const char *name, int32_t index, Arena *arena);
void  pix8_draw(Pix8 *sprite, int32_t x, int32_t y);
void  pix8_flip_horizontally(Pix8 *sprite, Arena *arena);
void  pix8_flip_vertically(Pix8 *sprite, Arena *arena);
void  pix8_crop(Pix8 *sprite, Arena *arena);
void  pix8_shrink(Pix8 *sprite, Arena *arena);
void  pix8_free(Pix8 *sprite);

#endif // RS2_PIX8_H
