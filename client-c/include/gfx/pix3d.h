#ifndef RS2_PIX32_PIX3D_H
#define RS2_PIX32_PIX3D_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Phase C primitive opt-in. RS2_PIX3D_REV225=1 enables the verified flat and
 * Gouraud entries. Individual switches remain available for isolated checks.
 * The Model/World3D depth scene path is not switched by these flags. */
#ifndef RS2_PIX3D_REV225
#define RS2_PIX3D_REV225 0
#endif
#ifndef RS2_PIX3D_REV225_FLAT
#define RS2_PIX3D_REV225_FLAT RS2_PIX3D_REV225
#endif
#ifndef RS2_PIX3D_REV225_GOURAUD
#define RS2_PIX3D_REV225_GOURAUD RS2_PIX3D_REV225
#endif
#ifndef RS2_PIX3D_REV225_TEXTURE
#define RS2_PIX3D_REV225_TEXTURE 0
#endif

extern int32_t pix3d_sin_table[2048];
extern int32_t pix3d_cos_table[2048];
extern int32_t pix3d_div_table[512];
extern int32_t pix3d_div_table2[2048];
extern int32_t pix3d_colour_table[65536];

extern int32_t *pix3d_scanline;
extern int32_t  pix3d_center_x;
extern int32_t  pix3d_center_y;
extern int32_t  pix3d_trans;
extern bool     pix3d_hclip;
extern bool     pix3d_low_detail;

void pix3d_init(void);
void pix3d_init_colour_table(double brightness);
uint32_t pix3d_gamma_correct(uint32_t rgb, double brightness);
void pix3d_init_2d(void);
void pix3d_set_trans(int32_t trans);
void pix3d_set_hclip(bool hclip);

void pix3d_flat_triangle(
    int32_t xA, int32_t xB, int32_t xC,
    int32_t yA, int32_t yB, int32_t yC,
    uint32_t colour
);

void pix3d_gouraud_triangle(
    int32_t xA, int32_t xB, int32_t xC,
    int32_t yA, int32_t yB, int32_t yC,
    int32_t colourA, int32_t colourB, int32_t colourC
);

// Camera-space vertices; UVs are relative to the cache texture axis triangle.
typedef struct { double x, y, z, u, v, shade; } Pix3DVertex;
// Shared linear camera transform: do not quantize coplanar vertices before depth testing.
Pix3DVertex pix3d_camera_vertex(double x, double y, double z, int pitch, int yaw);
void pix3d_begin_scene(void);
void pix3d_set_texture_palette(int id, const uint32_t *palette, int count);
void pix3d_set_texture_palette_gamma(int id, const uint32_t *palette, int count, double brightness);
void pix3d_set_texture_palette_gamma225(int id,const uint32_t *palette,int count,double brightness);
void pix3d_texture_palette_brightness225(double brightness);
int pix3d_texture_average(int id);
void pix3d_end_scene(void);
void pix3d_set_texture(int id, const uint32_t *pixels, int width, int height);
void pix3d_scene_triangle(Pix3DVertex a, Pix3DVertex b, Pix3DVertex c, int texture, int alpha);
void pix3d_map_texture(Pix3DVertex *v, Pix3DVertex a, Pix3DVertex b, Pix3DVertex c);

#endif // RS2_PIX32_PIX3D_H
