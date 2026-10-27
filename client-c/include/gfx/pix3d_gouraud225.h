#ifndef RS2_PIX3D_GOURAUD225_H
#define RS2_PIX3D_GOURAUD225_H
#include <stdint.h>
#include <stdbool.h>

extern bool pix3d_jagged;
/* Java contract: triangle colours are raw palette indices; raster colours
 * are 8.8 fixed point. Uses Pix2D/Pix3D clip, alpha, palette and line globals.
 * The caller supplies valid palette indices throughout interpolation and,
 * without hclip, valid destination spans (jagged groups must fit divTable).
 * Alpha reads the next destination word, including at the span end. */
void pix3d225_gouraud_raster(uint32_t *dst,int32_t offset,
                            int32_t left,int32_t right,int32_t colourA,int32_t colourB);
void pix3d225_gouraud_triangle(int32_t xA,int32_t xB,int32_t xC,
                              int32_t yA,int32_t yB,int32_t yC,
                              int32_t colourA,int32_t colourB,int32_t colourC);
#endif
