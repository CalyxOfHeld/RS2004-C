#ifndef RS2_PIX3D_TEXTURE_CACHE225_H
#define RS2_PIX3D_TEXTURE_CACHE225_H
#include "gfx/pix8.h"
#include <stdbool.h>
#include <stdint.h>

/* Pixel-value preparation from bundled Pix3D.getTexels. Image preparation
 * (crop or shrink) must already follow Pix3D.unpackTextures. */
void pix3d225_texture_cache_init(bool low_detail);
double pix3d225_effective_brightness(double requested);
// Sample the same brightness formula without shifting the existing protocol RNG.
double pix3d225_preview_brightness(double requested);
void pix3d225_texture_cache_set_image(int id,const Pix8 *image);
void pix3d225_texture_cache_set_brightness(double effective_brightness);
const uint32_t *pix3d225_texture_cache_get(int id,bool *opaque);
void pix3d225_texture_cache_triangle(int id,const int32_t args[18]);
void pix3d225_texture_cache_clear(void);
/* Changes when image preparation, detail or brightness invalidates texels. */
uint64_t pix3d225_texture_cache_generation(void);

uint64_t pix3d225_texture_cache_cycle(void);
bool pix3d225_texture_cache_animate_used(uint64_t since,int elapsed);

#endif
