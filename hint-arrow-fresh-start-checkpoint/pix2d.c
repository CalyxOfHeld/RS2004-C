#include "gfx/pix2d.h"
#include <string.h>
#include <math.h>

uint32_t *pix2d_pixels = NULL;
int32_t   pix2d_width = 0;
int32_t   pix2d_height = 0;
int32_t   pix2d_clip_top = 0;
int32_t   pix2d_clip_bottom = 0;
int32_t   pix2d_clip_left = 0;
int32_t   pix2d_clip_right = 0;
int32_t   pix2d_safe_width = 0;
int32_t   pix2d_center_x = 0;
int32_t   pix2d_center_y = 0;

void pix2d_bind(uint32_t *pixels, int32_t width, int32_t height) {
    pix2d_pixels = pixels;
    pix2d_width = width;
    pix2d_height = height;
    pix2d_set_clip(0, 0, width, height);
}

void pix2d_reset_clip(void) {
    pix2d_clip_left = 0;
    pix2d_clip_top = 0;
    pix2d_clip_right = pix2d_width;
    pix2d_clip_bottom = pix2d_height;
    pix2d_safe_width = pix2d_clip_right - 1;
    pix2d_center_x = pix2d_clip_right / 2;
    pix2d_center_y = pix2d_clip_bottom / 2;
}

void pix2d_set_clip(int32_t left, int32_t top, int32_t right, int32_t bottom) {
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > pix2d_width) right = pix2d_width;
    if (bottom > pix2d_height) bottom = pix2d_height;

    pix2d_clip_left = left;
    pix2d_clip_top = top;
    pix2d_clip_right = right;
    pix2d_clip_bottom = bottom;
    pix2d_safe_width = pix2d_clip_right - 1;
    pix2d_center_x = pix2d_clip_right / 2;
    pix2d_center_y = pix2d_clip_bottom / 2;
}

void pix2d_cls(void) {
    if (!pix2d_pixels) return;
    memset(pix2d_pixels, 0, pix2d_width * pix2d_height * sizeof(uint32_t));
}

void pix2d_fill_rect(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb) {
    if (x < pix2d_clip_left) {
        width -= (pix2d_clip_left - x);
        x = pix2d_clip_left;
    }
    if (y < pix2d_clip_top) {
        height -= (pix2d_clip_top - y);
        y = pix2d_clip_top;
    }
    if (x + width > pix2d_clip_right) {
        width = pix2d_clip_right - x;
    }
    if (y + height > pix2d_clip_bottom) {
        height = pix2d_clip_bottom - y;
    }
    if (width <= 0 || height <= 0) return;

    int32_t step = pix2d_width - width;
    int32_t offset = x + y * pix2d_width;

    for (int32_t i = -height; i < 0; i++) {
        for (int32_t j = -width; j < 0; j++) {
            pix2d_pixels[offset++] = rgb;
        }
        offset += step;
    }
}

void pix2d_fill_rect_trans(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb, int32_t alpha) {
    if (x < pix2d_clip_left) {
        width -= (pix2d_clip_left - x);
        x = pix2d_clip_left;
    }
    if (y < pix2d_clip_top) {
        height -= (pix2d_clip_top - y);
        y = pix2d_clip_top;
    }
    if (x + width > pix2d_clip_right) {
        width = pix2d_clip_right - x;
    }
    if (y + height > pix2d_clip_bottom) {
        height = pix2d_clip_bottom - y;
    }
    if (width <= 0 || height <= 0) return;

    uint32_t inv_alpha = 256 - alpha;
    uint32_t r0 = ((rgb >> 16) & 0xFF) * alpha;
    uint32_t g0 = ((rgb >> 8) & 0xFF) * alpha;
    uint32_t b0 = (rgb & 0xFF) * alpha;

    int32_t step = pix2d_width - width;
    int32_t offset = x + y * pix2d_width;

    for (int32_t i = 0; i < height; i++) {
        for (int32_t j = -width; j < 0; j++) {
            uint32_t cur = pix2d_pixels[offset];
            uint32_t r1 = ((cur >> 16) & 0xFF) * inv_alpha;
            uint32_t g1 = ((cur >> 8) & 0xFF) * inv_alpha;
            uint32_t b1 = (cur & 0xFF) * inv_alpha;

            uint32_t mixed = (((r0 + r1) >> 8) << 16) |
                             (((g0 + g1) >> 8) << 8) |
                             ((b0 + b1) >> 8);
            pix2d_pixels[offset++] = mixed;
        }
        offset += step;
    }
}

void pix2d_hline(int32_t x, int32_t y, int32_t width, uint32_t rgb) {
    if (y < pix2d_clip_top || y >= pix2d_clip_bottom) return;
    if (x < pix2d_clip_left) {
        width -= (pix2d_clip_left - x);
        x = pix2d_clip_left;
    }
    if (x + width > pix2d_clip_right) {
        width = pix2d_clip_right - x;
    }
    if (width <= 0) return;

    int32_t off = x + y * pix2d_width;
    for (int32_t i = 0; i < width; i++) {
        pix2d_pixels[off + i] = rgb;
    }
}

void pix2d_vline(int32_t x, int32_t y, int32_t height, uint32_t rgb) {
    if (x < pix2d_clip_left || x >= pix2d_clip_right) return;
    if (y < pix2d_clip_top) {
        height -= (pix2d_clip_top - y);
        y = pix2d_clip_top;
    }
    if (y + height > pix2d_clip_bottom) {
        height = pix2d_clip_bottom - y;
    }
    if (height <= 0) return;

    int32_t off = x + y * pix2d_width;
    for (int32_t i = 0; i < height; i++) {
        pix2d_pixels[off + i * pix2d_width] = rgb;
    }
}

void pix2d_draw_rect(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb) {
    pix2d_hline(x, y, width, rgb);
    pix2d_hline(x, y + height - 1, width, rgb);
    pix2d_vline(x, y, height, rgb);
    pix2d_vline(x + width - 1, y, height, rgb);
}

void pix2d_fill_circle(int32_t cx, int32_t cy, int32_t radius, uint32_t rgb, int32_t alpha) {
    uint32_t inv_alpha = 256 - alpha;
    uint32_t r0 = ((rgb >> 16) & 0xFF) * alpha;
    uint32_t g0 = ((rgb >> 8) & 0xFF) * alpha;
    uint32_t b0 = (rgb & 0xFF) * alpha;

    int32_t y_start = cy - radius;
    if (y_start < 0) y_start = 0;

    int32_t y_end = cy + radius;
    if (y_end >= pix2d_height) y_end = pix2d_height - 1;

    for (int32_t y = y_start; y <= y_end; y++) {
        int32_t dy = y - cy;
        int32_t dx = (int32_t)sqrt(radius * radius - dy * dy);

        int32_t x_start = cx - dx;
        if (x_start < 0) x_start = 0;

        int32_t x_end = cx + dx;
        if (x_end >= pix2d_width) x_end = pix2d_width - 1;

        int32_t offset = x_start + y * pix2d_width;
        for (int32_t x = x_start; x <= x_end; x++) {
            uint32_t cur = pix2d_pixels[offset];
            uint32_t r1 = ((cur >> 16) & 0xFF) * inv_alpha;
            uint32_t g1 = ((cur >> 8) & 0xFF) * inv_alpha;
            uint32_t b1 = (cur & 0xFF) * inv_alpha;

            uint32_t mixed = (((r0 + r1) >> 8) << 16) |
                             (((g0 + g1) >> 8) << 8) |
                             ((b0 + b1) >> 8);
            pix2d_pixels[offset++] = mixed;
        }
    }
}
