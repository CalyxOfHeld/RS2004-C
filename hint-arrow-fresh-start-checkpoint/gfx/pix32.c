#include "gfx/pix32.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "core/packet.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Pix32 *pix32_create(int32_t width, int32_t height, Arena *arena) {
    Pix32 *sprite = arena ? (Pix32 *)arena_alloc_zero(arena, sizeof(Pix32))
                          : (Pix32 *)calloc(1, sizeof(Pix32));
    if (!sprite) return NULL;

    sprite->width  = sprite->crop_w = width;
    sprite->height = sprite->crop_h = height;
    sprite->crop_x = sprite->crop_y = 0;
    sprite->from_arena = (arena != NULL);

    int32_t total = width * height;
    sprite->pixels = arena ? (uint32_t *)arena_alloc_zero(arena, total * sizeof(uint32_t))
                           : (uint32_t *)calloc(total, sizeof(uint32_t));
    return sprite;
}

Pix32 *pix32_load(Jagfile *jag, const char *name, int32_t index, Arena *arena) {
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

    Pix32 *sprite = arena ? (Pix32 *)arena_alloc_zero(arena, sizeof(Pix32))
                          : (Pix32 *)calloc(1, sizeof(Pix32));
    if (!sprite) return NULL;

    sprite->crop_w = g2(&idx_pkt);
    sprite->crop_h = g2(&idx_pkt);
    sprite->from_arena = (arena != NULL);

    int32_t pal_count = g1(&idx_pkt);
    uint32_t *palette = (uint32_t *)calloc(pal_count ? pal_count:1, sizeof(uint32_t));
    palette[0] = 0;

    for (int32_t i = 0; i < pal_count - 1; i++) {
        uint32_t rgb = (uint32_t)g3(&idx_pkt);
        if (rgb == 0) rgb = 1; // 0 is reserved for transparent
        palette[i + 1] = rgb;
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
    int32_t total = sprite->width * sprite->height;

    if(idx_pkt.error || dat_pkt.error || !pal_count || sprite->width<=0 || sprite->height<=0
        || sprite->crop_x+sprite->width>sprite->crop_w || sprite->crop_y+sprite->height>sprite->crop_h
        || pixel_order>1 || dat_pkt.pos>dat_size || (size_t)total>dat_size-dat_pkt.pos) {
        free(palette);if(!arena) {free(sprite);free(dat_buf);free(idx_buf);}
        return NULL;
    }
    for(int i=0;i<total;i++) if(dat_buf[dat_pkt.pos+i]>=pal_count) {
        free(palette);if(!arena) {free(sprite);free(dat_buf);free(idx_buf);}
        return NULL;
    }
    sprite->pixels = arena ? (uint32_t *)arena_alloc(arena, total * sizeof(uint32_t))
                           : (uint32_t *)malloc(total * sizeof(uint32_t));

    if (pixel_order == 0) {
        for (int32_t i = 0; i < total; i++) {
            sprite->pixels[i] = palette[g1(&dat_pkt)];
        }
    } else if (pixel_order == 1) {
        for (int32_t x = 0; x < sprite->width; x++) {
            for (int32_t y = 0; y < sprite->height; y++) {
                sprite->pixels[x + y * sprite->width] = palette[g1(&dat_pkt)];
            }
        }
    }

    free(palette);
    if (!arena) {
        free(dat_buf);
        free(idx_buf);
    }

    return sprite;
}

void pix32_bind(Pix32 *sprite) {
    if (sprite) {
        pix2d_bind(sprite->pixels, sprite->width, sprite->height);
    }
}

void pix32_draw(Pix32 *sprite, int32_t x, int32_t y) {
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
            uint32_t rgb = sprite->pixels[src_off++];
            if (rgb != 0) {
                pix2d_pixels[dst_off] = rgb;
            }
            dst_off++;
        }
        dst_off += dst_step;
        src_off += src_step;
    }
}

void pix32_draw_trans(Pix32 *sprite, int32_t x, int32_t y, int32_t alpha) {
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

    uint32_t inv_alpha = 256 - alpha;

    for (int32_t row = 0; row < h; row++) {
        for (int32_t col = 0; col < w; col++) {
            uint32_t rgb = sprite->pixels[src_off++];
            if (rgb != 0) {
                uint32_t cur = pix2d_pixels[dst_off];
                uint32_t r = (((rgb >> 16) & 0xFF) * alpha + ((cur >> 16) & 0xFF) * inv_alpha) >> 8;
                uint32_t g = (((rgb >> 8) & 0xFF) * alpha + ((cur >> 8) & 0xFF) * inv_alpha) >> 8;
                uint32_t b = ((rgb & 0xFF) * alpha + (cur & 0xFF) * inv_alpha) >> 8;
                pix2d_pixels[dst_off] = (r << 16) | (g << 8) | b;
            }
            dst_off++;
        }
        dst_off += dst_step;
        src_off += src_step;
    }
}

void pix32_crop(Pix32 *sprite, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!sprite || !pix2d_pixels) return;
    int32_t src_w = sprite->width;
    int32_t src_x = 0;
    int32_t src_y = 0;
    int32_t crop_w = sprite->crop_w;
    int32_t crop_h = sprite->crop_h;
    if (crop_w <= 0 || crop_h <= 0 || w <= 0 || h <= 0) return;

    int32_t step_x = (crop_w << 16) / w;
    int32_t step_y = (crop_h << 16) / h;
    x += (sprite->crop_x * w + crop_w - 1) / crop_w;
    y += (sprite->crop_y * h + crop_h - 1) / crop_h;
    if ((sprite->crop_x * w) % crop_w != 0) {
        src_x = ((crop_w - (sprite->crop_x * w) % crop_w) << 16) / w;
    }
    if ((sprite->crop_y * h) % crop_h != 0) {
        src_y = ((crop_h - (sprite->crop_y * h) % crop_h) << 16) / h;
    }
    w = w * (sprite->width - (src_x >> 16)) / crop_w;
    h = h * (sprite->height - (src_y >> 16)) / crop_h;

    int32_t dst_off = x + y * pix2d_width;
    int32_t dst_step = pix2d_width - w;

    if (y < pix2d_clip_top) {
        int32_t dy = pix2d_clip_top - y;
        h -= dy;
        y = pix2d_clip_top;
        dst_off += dy * pix2d_width;
        src_y += step_y * dy;
    }
    if (y + h > pix2d_clip_bottom) {
        h -= (y + h - pix2d_clip_bottom);
    }
    if (x < pix2d_clip_left) {
        int32_t dx = pix2d_clip_left - x;
        w -= dx;
        x = pix2d_clip_left;
        dst_off += dx;
        src_x += step_x * dx;
        dst_step += dx;
    }
    if (x + w > pix2d_clip_right) {
        int32_t dx = (x + w) - pix2d_clip_right;
        w -= dx;
        dst_step += dx;
    }

    if (w <= 0 || h <= 0) return;

    int32_t initial_src_x = src_x;
    for (int32_t row = 0; row < h; row++) {
        int32_t row_off = (src_y >> 16) * src_w;
        for (int32_t col = 0; col < w; col++) {
            uint32_t rgb = sprite->pixels[(src_x >> 16) + row_off];
            if (rgb != 0) {
                pix2d_pixels[dst_off] = rgb;
            }
            dst_off++;
            src_x += step_x;
        }
        src_y += step_y;
        src_x = initial_src_x;
        dst_off += dst_step;
    }
}

void pix32_draw_rotated_masked(
    Pix32 *sprite,
    int32_t angle,
    int32_t mask_w,
    const int32_t *mask_offsets,
    int32_t mask_h,
    int32_t src_cy,
    int32_t zoom,
    int32_t src_cx,
    int32_t dst_x,
    int32_t dst_y,
    const int32_t *mask_lengths
) {
    if (!sprite || !sprite->pixels || !pix2d_pixels) return;
    int32_t var12 = -mask_w / 2;
    int32_t var13 = -mask_h / 2;
    int32_t sin_val = pix3d_sin_table[angle & 2047];
    int32_t cos_val = pix3d_cos_table[angle & 2047];
    int32_t var24 = (sin_val * zoom) >> 8;
    int32_t var25 = (cos_val * zoom) >> 8;
    int32_t var16 = (src_cx << 16) + var13 * var24 + var12 * var25;
    int32_t var17 = (src_cy << 16) + (var13 * var25 - var12 * var24);
    int32_t dst_line = dst_x + dst_y * pix2d_width;

    for (int32_t y = 0; y < mask_h; y++) {
        int32_t x_off = mask_offsets[y];
        int32_t dst_idx = dst_line + x_off;
        int32_t u = var16 + var25 * x_off;
        int32_t v = var17 - var24 * x_off;
        int32_t len = mask_lengths[y];

        for (int32_t i = 0; i < len; i++) {
            int32_t px = u >> 16;
            int32_t py = v >> 16;
            if (px >= 0 && px < sprite->width && py >= 0 && py < sprite->height) {
                uint32_t col = sprite->pixels[px + py * sprite->width];
                if (col != 0) {
                    pix2d_pixels[dst_idx] = col;
                }
            }
            dst_idx++;
            u += var25;
            v -= var24;
        }

        var16 += var24;
        var17 += var25;
        dst_line += pix2d_width;
    }
}

void pix32_free(Pix32 *sprite) {
    if (sprite) {
        if (!sprite->from_arena) {
            free(sprite->pixels);
            free(sprite);
        }
    }
}
