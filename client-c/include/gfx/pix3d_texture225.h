#ifndef RS2_PIX3D_TEXTURE225_H
#define RS2_PIX3D_TEXTURE225_H
#include <stdbool.h>
#include <stdint.h>

/* Bundled revision-225 Pix3D.textureRaster. The supplied texels are the
 * original getTexels layout: 16,384 words in low detail or 65,536 in high. */
void pix3d225_texture_raster(uint32_t *dst,const uint32_t *texels,
    int32_t u,int32_t v,int32_t offset,int32_t left,int32_t right,
    int32_t shadeA,int32_t shadeB,int32_t a,int32_t b,int32_t c,
    int32_t da,int32_t db,int32_t dc,bool opaque);

/* args follow Java textureTriangle's first 18 arguments: y[3], x[3],
 * shade[3], and three camera-space texture-coordinate triples. */
void pix3d225_texture_triangle(const uint32_t *texels,bool opaque,const int32_t args[18]);

#endif
