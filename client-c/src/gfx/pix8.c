#include "gfx/pix8.h"
#include "gfx/pix2d.h"
#include "core/packet.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Pix8 *pix8_load(Jagfile *jag, const char *name, int32_t index, Arena *arena) {
    if (!jag || !name || index<0) return NULL;

    char dat_name[64];
    snprintf(dat_name, sizeof(dat_name), "%s.dat", name);

    size_t dat_size = 0;
    uint8_t *dat_buf = jagfile_read(jag, dat_name, NULL, &dat_size, arena);
    if (!dat_buf) return NULL;

    size_t idx_size = 0;
    uint8_t *idx_buf = jagfile_read(jag, "index.dat", NULL, &idx_size, arena);
    if (!idx_buf) {
        if (!arena) free(dat_buf);
        return NULL;
    }

    Packet dat_pkt = packet_wrap(dat_buf, dat_size);
    Packet idx_pkt = packet_wrap(idx_buf, idx_size);

    idx_pkt.pos = (size_t)g2(&dat_pkt);

    Pix8 *sprite = arena ? (Pix8 *)arena_alloc_zero(arena, sizeof(Pix8))
                         : (Pix8 *)calloc(1, sizeof(Pix8));
    if (!sprite) return NULL;

    sprite->crop_w = g2(&idx_pkt);
    sprite->crop_h = g2(&idx_pkt);

    int32_t pal_count = g1(&idx_pkt);
    sprite->palette_count = pal_count;
    sprite->palette = arena ? (uint32_t *)arena_alloc(arena, pal_count * sizeof(uint32_t))
                            : (uint32_t *)malloc((pal_count ? pal_count:1) * sizeof(uint32_t));
    sprite->palette[0] = 0; // index 0 is transparent in RS2

    for (int32_t i = 0; i < pal_count - 1; i++) {
        sprite->palette[i + 1] = (uint32_t)g3(&idx_pkt);
    }

    for (int32_t i = 0; i < index; i++) {
        idx_pkt.pos += 2;
        int32_t prev_w = g2(&idx_pkt);
        int32_t prev_h = g2(&idx_pkt);
        dat_pkt.pos += (size_t)(prev_w * prev_h);
        idx_pkt.pos++;
    }

    sprite->crop_x = g1(&idx_pkt);
    sprite->crop_y = g1(&idx_pkt);
    sprite->width  = g2(&idx_pkt);
    sprite->height = g2(&idx_pkt);

    int32_t pixel_order = g1(&idx_pkt);
    int32_t total_pixels = sprite->width * sprite->height;

    if(idx_pkt.error || dat_pkt.error || !pal_count || sprite->width<=0 || sprite->height<=0
        || sprite->crop_x+sprite->width>sprite->crop_w || sprite->crop_y+sprite->height>sprite->crop_h
        || pixel_order>1 || dat_pkt.pos>dat_size || (size_t)total_pixels>dat_size-dat_pkt.pos) {
        if (!arena) {free(sprite->palette);free(sprite);free(dat_buf);free(idx_buf);}
        return NULL;
    }
    for(int i=0;i<total_pixels;i++) if(dat_buf[dat_pkt.pos+i]>=pal_count) {
        if (!arena) {free(sprite->palette);free(sprite);free(dat_buf);free(idx_buf);}
        return NULL;
    }
    sprite->pixels = arena ? (uint8_t *)arena_alloc(arena, total_pixels)
                           : (uint8_t *)malloc(total_pixels);

    if (pixel_order == 0) {
        for (int32_t i = 0; i < total_pixels; i++) {
            sprite->pixels[i] = (uint8_t)g1b(&dat_pkt);
        }
    } else if (pixel_order == 1) {
        for (int32_t x = 0; x < sprite->width; x++) {
            for (int32_t y = 0; y < sprite->height; y++) {
                sprite->pixels[x + y * sprite->width] = (uint8_t)g1b(&dat_pkt);
            }
        }
    }

    if (!arena) {
        free(dat_buf);
        free(idx_buf);
    }

    return sprite;
}

void pix8_crop(Pix8 *sprite, Arena *arena) {
    if (!sprite || (sprite->width == sprite->crop_w && sprite->height == sprite->crop_h)) {
        return;
    }

    int32_t total = sprite->crop_w * sprite->crop_h;
    uint8_t *cropped = arena ? (uint8_t *)arena_alloc_zero(arena, total)
                             : (uint8_t *)calloc(1, total);
    if (!cropped) return;

    int32_t src_idx = 0;
    for (int32_t y = 0; y < sprite->height; y++) {
        for (int32_t x = 0; x < sprite->width; x++) {
            int32_t dst_idx = (x + sprite->crop_x) + (y + sprite->crop_y) * sprite->crop_w;
            cropped[dst_idx] = sprite->pixels[src_idx++];
        }
    }

    if (!arena) {
        free(sprite->pixels);
    }
    sprite->pixels = cropped;
    sprite->width = sprite->crop_w;
    sprite->height = sprite->crop_h;
    sprite->crop_x = 0;
    sprite->crop_y = 0;
}

void pix8_shrink(Pix8 *sprite, Arena *arena) {
    if (!sprite || sprite->crop_w < 2 || sprite->crop_h < 2) return;
    int32_t width = sprite->crop_w / 2;
    int32_t height = sprite->crop_h / 2;
    uint8_t *pixels = arena ? (uint8_t *)arena_alloc_zero(arena, (size_t)width * height)
                            : (uint8_t *)calloc((size_t)width * height, 1);
    if (!pixels) return;
    for (int32_t y = 0; y < sprite->height; y++) {
        for (int32_t x = 0; x < sprite->width; x++) {
            int32_t dst_x = (x + sprite->crop_x) >> 1;
            int32_t dst_y = (y + sprite->crop_y) >> 1;
            if (dst_x >= 0 && dst_x < width && dst_y >= 0 && dst_y < height)
                pixels[dst_x + dst_y * width] = sprite->pixels[x + y * sprite->width];
        }
    }
    if (!arena) free(sprite->pixels);
    sprite->pixels = pixels;
    sprite->width = width;
    sprite->height = height;
    sprite->crop_w = width;
    sprite->crop_h = height;
    sprite->crop_x = 0;
    sprite->crop_y = 0;
}

void pix8_draw(Pix8 *sprite, int32_t x, int32_t y) {
    if (!sprite || !pix2d_pixels) return;

    x += sprite->crop_x;
    y += sprite->crop_y;

    int32_t dst_off = x + y * pix2d_width;
    int32_t src_off = 0;
    int32_t h = sprite->height;
    int32_t w = sprite->width;
    int32_t dst_step = pix2d_width - w;
    int32_t src_step = 0;

    if (y < pix2d_clip_top) {
        int32_t dy = pix2d_clip_top - y;
        h -= dy;
        y = pix2d_clip_top;
        src_off += dy * w;
        dst_off += dy * pix2d_width;
    }
    if (y + h > pix2d_clip_bottom) {
        h = pix2d_clip_bottom - y;
    }
    if (x < pix2d_clip_left) {
        int32_t dx = pix2d_clip_left - x;
        w -= dx;
        x = pix2d_clip_left;
        src_off += dx;
        dst_off += dx;
        src_step += dx;
        dst_step += dx;
    }
    if (x + w > pix2d_clip_right) {
        int32_t dx = (x + w) - pix2d_clip_right;
        w -= dx;
        src_step += dx;
        dst_step += dx;
    }

    if (w <= 0 || h <= 0) return;

    for (int32_t row = 0; row < h; row++) {
        for (int32_t col = 0; col < w; col++) {
            uint8_t pal_idx = sprite->pixels[src_off++];
            if (pal_idx != 0) {
                pix2d_pixels[dst_off] = sprite->palette[pal_idx];
            }
            dst_off++;
        }
        dst_off += dst_step;
        src_off += src_step;
    }
}

void pix8_flip_horizontally(Pix8 *sprite, Arena *arena) {
    if (!sprite || !sprite->pixels) return;
    int total = sprite->width * sprite->height;
    uint8_t *flipped = (uint8_t*)(arena ? arena_alloc(arena, total) : malloc(total));
    int dst = 0;
    for (int y = 0; y < sprite->height; y++) {
        for (int x = sprite->width - 1; x >= 0; x--) {
            flipped[dst++] = sprite->pixels[x + y * sprite->width];
        }
    }
    if (!arena) free(sprite->pixels);
    sprite->pixels = flipped;
    sprite->crop_x = sprite->crop_w - sprite->width - sprite->crop_x;
}

void pix8_flip_vertically(Pix8 *sprite, Arena *arena) {
    if (!sprite || !sprite->pixels) return;
    int total = sprite->width * sprite->height;
    uint8_t *flipped = (uint8_t*)(arena ? arena_alloc(arena, total) : malloc(total));
    int dst = 0;
    for (int y = sprite->height - 1; y >= 0; y--) {
        for (int x = 0; x < sprite->width; x++) {
            flipped[dst++] = sprite->pixels[x + y * sprite->width];
        }
    }
    if (!arena) free(sprite->pixels);
    sprite->pixels = flipped;
    sprite->crop_y = sprite->crop_h - sprite->height - sprite->crop_y;
}

void pix8_free(Pix8 *sprite) {
    if (sprite) {
        free(sprite->pixels);
        free(sprite->palette);
        free(sprite);
    }
}
