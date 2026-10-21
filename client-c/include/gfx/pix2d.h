#ifndef RS2_PIX2D_H
#define RS2_PIX2D_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern uint32_t *pix2d_pixels;
extern int32_t   pix2d_width;
extern int32_t   pix2d_height;
extern int32_t   pix2d_clip_top;
extern int32_t   pix2d_clip_bottom;
extern int32_t   pix2d_clip_left;
extern int32_t   pix2d_clip_right;
extern int32_t   pix2d_safe_width;
extern int32_t   pix2d_center_x;
extern int32_t   pix2d_center_y;

void pix2d_bind(uint32_t *pixels, int32_t width, int32_t height);
void pix2d_reset_clip(void);
void pix2d_set_clip(int32_t left, int32_t top, int32_t right, int32_t bottom);
void pix2d_cls(void);
void pix2d_fill_rect(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb);
void pix2d_fill_rect_trans(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb, int32_t alpha);
void pix2d_draw_rect(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb);
void pix2d_hline(int32_t x, int32_t y, int32_t width, uint32_t rgb);
void pix2d_vline(int32_t x, int32_t y, int32_t height, uint32_t rgb);
void pix2d_fill_circle(int32_t cx, int32_t cy, int32_t radius, uint32_t rgb, int32_t alpha);

#endif // RS2_PIX2D_H
