#ifndef RS2_PIX3D_FLAT225_H
#define RS2_PIX3D_FLAT225_H

#include <stdint.h>

/* Revision-225 contract: vertical bounds are [0, clip_bottom); with hclip,
 * horizontal spans are [0, safe_width). The original ignores clip_top/left.
 * With hclip disabled the caller must provide valid horizontal coordinates.
 * Translucent spans read the next destination word, including at the span end.
 * Uses pix3d_trans, pix3d_hclip, pix3d_scanline and the bound Pix2D surface. */
void pix3d225_flat_raster(uint32_t *dst, int32_t offset, uint32_t rgb,
                         int32_t left, int32_t right);
void pix3d225_flat_triangle(int32_t xA, int32_t xB, int32_t xC,
                           int32_t yA, int32_t yB, int32_t yC, uint32_t rgb);

#endif
