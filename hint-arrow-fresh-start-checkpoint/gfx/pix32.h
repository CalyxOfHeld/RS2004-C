#ifndef RS2_PIX32_H
#define RS2_PIX32_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "core/arena.h"
#include "core/jagfile.h"

typedef struct {
    uint32_t *pixels;
    int32_t   width;
    int32_t   height;
    int32_t   crop_x;
    int32_t   crop_y;
    int32_t   crop_w;
    int32_t   crop_h;
    bool      from_arena;
} Pix32;

Pix32 *pix32_create(int32_t width, int32_t height, Arena *arena);
Pix32 *pix32_load(Jagfile *jag, const char *name, int32_t index, Arena *arena);
void   pix32_bind(Pix32 *sprite);
void   pix32_draw(Pix32 *sprite, int32_t x, int32_t y);
void   pix32_draw_trans(Pix32 *sprite, int32_t x, int32_t y, int32_t alpha);
void   pix32_crop(Pix32 *sprite, int32_t x, int32_t y, int32_t w, int32_t h);
void   pix32_draw_rotated_masked(Pix32 *sprite, int32_t angle, int32_t mask_w, const int32_t *mask_offsets, int32_t mask_h, int32_t src_cy, int32_t zoom, int32_t src_cx, int32_t dst_x, int32_t dst_y, const int32_t *mask_lengths);
void   pix32_free(Pix32 *sprite);

#endif // RS2_PIX32_H
