#include "gfx/pix3d.h"
#include "gfx/pix3d_flat225.h"
#include "gfx/pix3d_gouraud225.h"
#include "gfx/pix2d.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

int32_t pix3d_sin_table[2048];
int32_t pix3d_cos_table[2048];
int32_t pix3d_div_table[512];
int32_t pix3d_div_table2[2048];
int32_t pix3d_colour_table[65536];

int32_t *pix3d_scanline = NULL;
int32_t  pix3d_center_x = 0;
int32_t  pix3d_center_y = 0;
int32_t  pix3d_trans = 0;
bool     pix3d_hclip = false;
bool     pix3d_low_detail = true;

static uint32_t gamma_correct(uint32_t rgb, double gamma) {
    double r = ((rgb >> 16) & 0xFF) / 256.0;
    double g = ((rgb >> 8) & 0xFF) / 256.0;
    double b = (rgb & 0xFF) / 256.0;

    int32_t int_r = (int32_t)(pow(r, gamma) * 256.0);
    int32_t int_g = (int32_t)(pow(g, gamma) * 256.0);
    int32_t int_b = (int32_t)(pow(b, gamma) * 256.0);

    if (int_r < 0) int_r = 0; else if (int_r > 255) int_r = 255;
    if (int_g < 0) int_g = 0; else if (int_g > 255) int_g = 255;
    if (int_b < 0) int_b = 0; else if (int_b > 255) int_b = 255;

    return ((uint32_t)int_r << 16) | ((uint32_t)int_g << 8) | (uint32_t)int_b;
}

uint32_t pix3d_gamma_correct(uint32_t rgb, double brightness) {
    return gamma_correct(rgb, brightness);
}

void pix3d_init(void) {
    for (int32_t i = 1; i < 512; i++) {
        pix3d_div_table[i] = 32768 / i;
    }
    for (int32_t i = 1; i < 2048; i++) {
        pix3d_div_table2[i] = 65536 / i;
    }
    for (int32_t i = 0; i < 2048; i++) {
        // Preserve the original Pix3D constant: substituting a more precise
        // 2*pi/2048 changes integer entries and shifts rotated sprite pixels.
        pix3d_sin_table[i] = (int32_t)(sin((double)i * 0.0030679615) * 65536.0);
        pix3d_cos_table[i] = (int32_t)(cos((double)i * 0.0030679615) * 65536.0);
    }
}

void pix3d_init_colour_table(double brightness) {
    int32_t offset = 0;
    for (int32_t y = 0; y < 512; y++) {
        double hue = (double)(y / 8) / 64.0 + 0.0078125;
        double sat = (double)(y & 0x7) / 8.0 + 0.0625;

        for (int32_t x = 0; x < 128; x++) {
            double light = (double)x / 128.0;
            double r = light;
            double g = light;
            double b = light;

            if (sat != 0.0) {
                double q = (light < 0.5) ? (light * (sat + 1.0)) : (light + sat - light * sat);
                double p = light * 2.0 - q;

                double t = hue + 1.0 / 3.0;
                if (t > 1.0) t -= 1.0;

                double d11 = hue - 1.0 / 3.0;
                if (d11 < 0.0) d11 += 1.0;

                if (t * 6.0 < 1.0)      r = p + (q - p) * 6.0 * t;
                else if (t * 2.0 < 1.0) r = q;
                else if (t * 3.0 < 2.0) r = p + (q - p) * (2.0 / 3.0 - t) * 6.0;
                else                    r = p;

                if (hue * 6.0 < 1.0)      g = p + (q - p) * 6.0 * hue;
                else if (hue * 2.0 < 1.0) g = q;
                else if (hue * 3.0 < 2.0) g = p + (q - p) * (2.0 / 3.0 - hue) * 6.0;
                else                      g = p;

                if (d11 * 6.0 < 1.0)      b = p + (q - p) * 6.0 * d11;
                else if (d11 * 2.0 < 1.0) b = q;
                else if (d11 * 3.0 < 2.0) b = p + (q - p) * (2.0 / 3.0 - d11) * 6.0;
                else                      b = p;
            }

            int32_t int_r = (int32_t)(r * 256.0);
            int32_t int_g = (int32_t)(g * 256.0);
            int32_t int_b = (int32_t)(b * 256.0);
            uint32_t rgb = ((uint32_t)int_r << 16) | ((uint32_t)int_g << 8) | (uint32_t)int_b;

            pix3d_colour_table[offset++] = gamma_correct(rgb, brightness);
        }
    }
}

void pix3d_init_2d(void) {
    if (pix3d_scanline) {
        free(pix3d_scanline);
    }
    pix3d_scanline = (int32_t *)malloc(pix2d_height * sizeof(int32_t));
    for (int32_t y = 0; y < pix2d_height; y++) {
        pix3d_scanline[y] = y * pix2d_width;
    }
    pix3d_center_x = pix2d_width / 2;
    pix3d_center_y = pix2d_height / 2;
}

void pix3d_set_trans(int32_t trans) {
    pix3d_trans = trans;
}

void pix3d_set_hclip(bool hclip) {
    pix3d_hclip = hclip;
}

#if !RS2_PIX3D_REV225_FLAT
static void flat_raster(int32_t xA, int32_t xB, uint32_t *dst, int32_t off, uint32_t colour) {
    if (xB <= pix2d_clip_left || xA >= pix2d_clip_right) return;
    if (xA < pix2d_clip_left) xA = pix2d_clip_left;
    if (xB > pix2d_clip_right) xB = pix2d_clip_right;
    if (xA >= xB) return;

    off += xA;
    int32_t len = (xB - xA);

    if (pix3d_trans == 0) {
        for (int32_t i = 0; i < len; i++) {
            dst[off++] = colour;
        }
    } else {
        uint32_t inv_alpha = 256 - pix3d_trans;
        uint32_t alpha = pix3d_trans;
        uint32_t r0 = ((colour >> 16) & 0xFF) * inv_alpha;
        uint32_t g0 = ((colour >> 8) & 0xFF) * inv_alpha;
        uint32_t b0 = (colour & 0xFF) * inv_alpha;

        for (int32_t i = 0; i < len; i++) {
            uint32_t cur = dst[off];
            uint32_t r1 = ((cur >> 16) & 0xFF) * alpha;
            uint32_t g1 = ((cur >> 8) & 0xFF) * alpha;
            uint32_t b1 = (cur & 0xFF) * alpha;
            dst[off++] = (((r0 + r1) >> 8) << 16) | (((g0 + g1) >> 8) << 8) | ((b0 + b1) >> 8);
        }
    }
}
#endif

void pix3d_flat_triangle(
    int32_t xA, int32_t xB, int32_t xC,
    int32_t yA, int32_t yB, int32_t yC,
    uint32_t colour
) {
#if RS2_PIX3D_REV225_FLAT
    pix3d225_flat_triangle(xA, xB, xC, yA, yB, yC, colour);
#else
    // Sort vertices by Y ascending: yA <= yB <= yC
    if (yA > yB) { int32_t t = xA; xA = xB; xB = t; t = yA; yA = yB; yB = t; }
    if (yA > yC) { int32_t t = xA; xA = xC; xC = t; t = yA; yA = yC; yC = t; }
    if (yB > yC) { int32_t t = xB; xB = xC; xC = t; t = yB; yB = yC; yC = t; }

    if (yA >= pix2d_clip_bottom || yC < pix2d_clip_top) return;

    int32_t dxAB = (yB != yA) ? (((xB - xA) << 16) / (yB - yA)) : 0;
    int32_t dxBC = (yC != yB) ? (((xC - xB) << 16) / (yC - yB)) : 0;
    int32_t dxAC = (yC != yA) ? (((xC - xA) << 16) / (yC - yA)) : 0;

    int32_t curXA = xA << 16;
    int32_t curXB = xA << 16;

    // Top half: yA to yB
    for (int32_t y = yA; y < yB; y++) {
        if (y >= pix2d_clip_top && y < pix2d_clip_bottom) {
            int32_t x1 = curXA >> 16;
            int32_t x2 = curXB >> 16;
            if (x1 > x2) { int32_t t = x1; x1 = x2; x2 = t; }
            flat_raster(x1, x2, pix2d_pixels, pix3d_scanline[y], colour);
        }
        curXA += dxAC;
        curXB += dxAB;
    }

    curXB = xB << 16;

    // Bottom half: yB to yC
    for (int32_t y = yB; y < yC; y++) {
        if (y >= pix2d_clip_top && y < pix2d_clip_bottom) {
            int32_t x1 = curXA >> 16;
            int32_t x2 = curXB >> 16;
            if (x1 > x2) { int32_t t = x1; x1 = x2; x2 = t; }
            flat_raster(x1, x2, pix2d_pixels, pix3d_scanline[y], colour);
        }
        curXA += dxAC;
        curXB += dxBC;
    }
#endif
}

#if !RS2_PIX3D_REV225_GOURAUD
static void gouraud_raster(
    int32_t xA, int32_t xB,
    int32_t colA, int32_t colB,
    uint32_t *dst, int32_t off
) {
    if (xB <= pix2d_clip_left || xA >= pix2d_clip_right) return;
    int32_t width = xB - xA;
    if (width <= 0) return;

    int32_t dCol = (colB - colA) / width;
    if (xA < pix2d_clip_left) {
        colA += (pix2d_clip_left - xA) * dCol;
        xA = pix2d_clip_left;
    }
    if (xB > pix2d_clip_right) {
        xB = pix2d_clip_right;
    }
    if (xA >= xB) return;

    width = xB - xA;
    int32_t curCol = colA;
    off += xA;

    for (int32_t i = 0; i < width; i++) {
        int32_t idx = curCol >> 8;
        if (idx < 0) idx = 0;
        else if (idx > 65535) idx = 65535;
        dst[off++] = pix3d_colour_table[idx];
        curCol += dCol;
    }
}

#endif

void pix3d_gouraud_triangle(
    int32_t xA, int32_t xB, int32_t xC,
    int32_t yA, int32_t yB, int32_t yC,
    int32_t colourA, int32_t colourB, int32_t colourC
) {
#if RS2_PIX3D_REV225_GOURAUD
    /* Existing native callers pass 8.8 colours; Java takes palette indices. */
    pix3d225_gouraud_triangle(xA,xB,xC,yA,yB,yC,colourA/256,colourB/256,colourC/256);
#else
    // Sort vertices by Y ascending
    if (yA > yB) {
        int32_t t = xA; xA = xB; xB = t; t = yA; yA = yB; yB = t; t = colourA; colourA = colourB; colourB = t;
    }
    if (yA > yC) {
        int32_t t = xA; xA = xC; xC = t; t = yA; yA = yC; yC = t; t = colourA; colourA = colourC; colourC = t;
    }
    if (yB > yC) {
        int32_t t = xB; xB = xC; xC = t; t = yB; yB = yC; yC = t; t = colourB; colourB = colourC; colourC = t;
    }

    if (yA >= pix2d_clip_bottom || yC < pix2d_clip_top) return;

    int32_t dxAB = (yB != yA) ? (((xB - xA) << 16) / (yB - yA)) : 0;
    int32_t dxBC = (yC != yB) ? (((xC - xB) << 16) / (yC - yB)) : 0;
    int32_t dxAC = (yC != yA) ? (((xC - xA) << 16) / (yC - yA)) : 0;

    int32_t dcolAB = (yB != yA) ? ((colourB - colourA) / (yB - yA)) : 0;
    int32_t dcolBC = (yC != yB) ? ((colourC - colourB) / (yC - yB)) : 0;
    int32_t dcolAC = (yC != yA) ? ((colourC - colourA) / (yC - yA)) : 0;

    int32_t curXA = xA << 16;
    int32_t curXB = xA << 16;
    int32_t curColA = colourA;
    int32_t curColB = colourA;

    // Top half
    for (int32_t y = yA; y < yB; y++) {
        if (y >= pix2d_clip_top && y < pix2d_clip_bottom) {
            int32_t x1 = curXA >> 16;
            int32_t x2 = curXB >> 16;
            int32_t c1 = curColA;
            int32_t c2 = curColB;
            if (x1 > x2) {
                int32_t t = x1; x1 = x2; x2 = t; t = c1; c1 = c2; c2 = t;
            }
            gouraud_raster(x1, x2, c1, c2, pix2d_pixels, pix3d_scanline[y]);
        }
        curXA += dxAC;
        curXB += dxAB;
        curColA += dcolAC;
        curColB += dcolAB;
    }

    curXB = xB << 16;
    curColB = colourB;

    // Bottom half
    for (int32_t y = yB; y < yC; y++) {
        if (y >= pix2d_clip_top && y < pix2d_clip_bottom) {
            int32_t x1 = curXA >> 16;
            int32_t x2 = curXB >> 16;
            int32_t c1 = curColA;
            int32_t c2 = curColB;
            if (x1 > x2) {
                int32_t t = x1; x1 = x2; x2 = t; t = c1; c1 = c2; c2 = t;
            }
            gouraud_raster(x1, x2, c1, c2, pix2d_pixels, pix3d_scanline[y]);
        }
        curXA += dxAC;
        curXB += dxBC;
        curColA += dcolAC;
        curColB += dcolBC;
    }
#endif
}

/* Shared reciprocal depth makes opaque world visibility independent of tile
 * insertion order. UI and item-icon rasterizers retain their existing path. */
static double *scene_depth;
static size_t scene_capacity;
static bool scene_active;
typedef struct { Pix3DVertex a,b,c; int texture,alpha; double depth; } AlphaTriangle;
static AlphaTriangle *alpha_triangles;
static size_t alpha_count, alpha_capacity;
static bool flushing_alpha;
static int alpha_compare(const void *a,const void *b) {
    double da=((const AlphaTriangle *)a)->depth, db=((const AlphaTriangle *)b)->depth;
    return da<db ? 1 : da>db ? -1 : 0;
}
static struct { uint32_t *pixels; int w, h; } textures[50];
static uint32_t texture_average[50];
/* Original getAverageTextureRGB caches on first query, not palette upload.
 * Brightness changes update only averages that have not yet been queried. */
static struct { bool enabled,queried; int count; uint32_t palette[256]; } average225[50];
// Reference Pix3D averages the gamma-corrected palette, not texel frequency.
void pix3d_set_texture_palette(int id, const uint32_t *palette, int count) {
    pix3d_set_texture_palette_gamma(id, palette, count, 0.8);
}
void pix3d_set_texture_palette_gamma(int id, const uint32_t *palette, int count, double brightness) {
    if(id<0 || id>=50 || !palette || count<=0) return;
    average225[id].enabled=false;
    uint64_t r=0,g=0,b=0;
    for(int i=0;i<count;i++) {
        uint32_t c=gamma_correct(palette[i],brightness);
        r+=(c>>16)&255;g+=(c>>8)&255;b+=c&255;
    }
    uint32_t rgb=(uint32_t)(r/count)<<16 | (uint32_t)(g/count)<<8 | (uint32_t)(b/count);
    texture_average[id]=gamma_correct(rgb,1.4);
    if(!texture_average[id]) texture_average[id]=1;
}
void pix3d_set_texture_palette_gamma225(int id,const uint32_t *palette,int count,double brightness) {
    if(id<0 || id>=50 || !palette || count<=0 || count>256)return;
    average225[id].count=count;
    memcpy(average225[id].palette,palette,(size_t)count*sizeof(*palette));
    average225[id].queried=false;
    pix3d_set_texture_palette_gamma(id,palette,count,brightness);
    average225[id].enabled=true;
}
void pix3d_texture_palette_brightness225(double brightness) {
    for(int id=0;id<50;id++)if(average225[id].enabled && !average225[id].queried) {
        pix3d_set_texture_palette_gamma(id,average225[id].palette,average225[id].count,brightness);
        average225[id].enabled=true;
    }
}
int pix3d_texture_average(int id) {
    if(id<0 || id>=50)return 0;
    if(average225[id].enabled)average225[id].queried=true;
    return (int)texture_average[id];
}


void pix3d_set_texture(int id, const uint32_t *pixels, int width, int height) {
    if (id < 0 || id >= 50 || width <= 0 || height <= 0) return;
    free(textures[id].pixels);
    textures[id].pixels = malloc((size_t)width * height * sizeof(uint32_t));
    if (!textures[id].pixels) return;
    memcpy(textures[id].pixels, pixels, (size_t)width * height * sizeof(uint32_t));
    textures[id].w = width; textures[id].h = height;
}
Pix3DVertex pix3d_camera_vertex(double x,double y,double z,int pitch,int yaw) {
    double sy=pix3d_sin_table[yaw&2047]/65536.0, cy=pix3d_cos_table[yaw&2047]/65536.0;
    double sp=pix3d_sin_table[pitch&2047]/65536.0, cp=pix3d_cos_table[pitch&2047]/65536.0;
    double vx=x*cy+z*sy, vz=z*cy-x*sy;
    return (Pix3DVertex){vx,y*cp-vz*sp,y*sp+vz*cp,0,0,0};
}
void pix3d_begin_scene(void) {
    size_t count = (size_t)pix2d_width * pix2d_height;
    if (scene_capacity < count) {
        double *next = realloc(scene_depth, count * sizeof(double));
        if (!next) { scene_active = false; return; }
        scene_depth = next; scene_capacity = count;
    }
    memset(scene_depth, 0, count * sizeof(double));
    scene_active = true;
    alpha_count = 0;
}
void pix3d_end_scene(void) {
    if(alpha_count) qsort(alpha_triangles,alpha_count,sizeof(AlphaTriangle),alpha_compare);
    flushing_alpha=true;
    for(size_t i=0;i<alpha_count;i++) {
        AlphaTriangle *t=&alpha_triangles[i];
        pix3d_scene_triangle(t->a,t->b,t->c,t->texture,t->alpha);
    }
    flushing_alpha=false; alpha_count=0; scene_active=false;
}

void pix3d_map_texture(Pix3DVertex *v, Pix3DVertex a, Pix3DVertex b, Pix3DVertex c) {
    double bx=b.x-a.x, by=b.y-a.y, bz=b.z-a.z;
    double cx=c.x-a.x, cy=c.y-a.y, cz=c.z-a.z;
    double dx=v->x-a.x, dy=v->y-a.y, dz=v->z-a.z;
    double bb=bx*bx+by*by+bz*bz, cc=cx*cx+cy*cy+cz*cz;
    double bc=bx*cx+by*cy+bz*cz, db=dx*bx+dy*by+dz*bz, dc=dx*cx+dy*cy+dz*cz;
    double det=bb*cc-bc*bc;
    v->u = det ? (db*cc-dc*bc)/det : 0;
    v->v = det ? (dc*bb-db*bc)/det : 0;
}
static void scene_raster(Pix3DVertex a, Pix3DVertex b, Pix3DVertex c, int texture, int alpha) {
    double ax=pix3d_center_x+a.x*512/a.z, ay=pix3d_center_y+a.y*512/a.z;
    double bx=pix3d_center_x+b.x*512/b.z, by=pix3d_center_y+b.y*512/b.z;
    double cx=pix3d_center_x+c.x*512/c.z, cy=pix3d_center_y+c.y*512/c.z;
    double area=(ax-bx)*(cy-by)-(ay-by)*(cx-bx);
    if (area <= 0) return;
    int left=(int)floor(fmin(ax,fmin(bx,cx))), right=(int)ceil(fmax(ax,fmax(bx,cx)));
    int top=(int)floor(fmin(ay,fmin(by,cy))), bottom=(int)ceil(fmax(ay,fmax(by,cy)));
    if(left<pix2d_clip_left) left=pix2d_clip_left;
    if(right>pix2d_clip_right) right=pix2d_clip_right;
    if(top<pix2d_clip_top) top=pix2d_clip_top;
    if(bottom>pix2d_clip_bottom) bottom=pix2d_clip_bottom;
    bool textured=texture>=0 && texture<50 && textures[texture].pixels;
    double ia=1/a.z, ib=1/b.z, ic=1/c.z;
    for(int y=top;y<bottom;y++) for(int x=left;x<right;x++) {
        double px=x+0.5, py=y+0.5;
        double wa=((px-bx)*(cy-by)-(py-by)*(cx-bx))/area;
        double wb=((px-cx)*(ay-cy)-(py-cy)*(ax-cx))/area;
        double wc=1-wa-wb;
        if(wa < -1e-9 || wb < -1e-9 || wc < -1e-9) continue;
        double iz=wa*ia+wb*ib+wc*ic;
        int off=y*pix2d_width+x;
        // Equal-plane fragments follow reference model submission order (including
        // face priority). Allow only floating-point roundoff, not a depth bias.
        if(scene_active && iz < scene_depth[off] - 64 * DBL_EPSILON * fmax(iz,scene_depth[off])) continue;
        uint32_t rgb;
        double shade=wa*a.shade+wb*b.shade+wc*c.shade;
        if(textured) {
            double u=(wa*a.u*ia+wb*b.u*ib+wc*c.u*ic)/iz;
            double v=(wa*a.v*ia+wb*b.v*ib+wc*c.v*ic)/iz;
            int tw=textures[texture].w, th=textures[texture].h;
            int tx=(int)floor((u-floor(u))*tw), ty=(int)floor((v-floor(v))*th);
            rgb=textures[texture].pixels[ty*tw+tx];
            if(!rgb) continue; // transparent texels never write depth
            double light=(127-shade)/128;
            if(light<0) light=0;
            if(light>1) light=1;
            rgb=((uint32_t)(((rgb>>16)&255)*light)<<16) |
                ((uint32_t)(((rgb>>8)&255)*light)<<8) | (uint32_t)((rgb&255)*light);
        } else {
            int col=(int)shade;
            if(col<0) col=0;
            if(col>65535) col=65535;
            rgb=pix3d_colour_table[col];
        }
        if(alpha) {
            uint32_t old=pix2d_pixels[off]; int inv=256-alpha;
            rgb=((((rgb>>16)&255)*inv+((old>>16)&255)*alpha)>>8)<<16 |
                ((((rgb>>8)&255)*inv+((old>>8)&255)*alpha)>>8)<<8 |
                (((rgb&255)*inv+(old&255)*alpha)>>8);
        }
        pix2d_pixels[off]=rgb;
        if(scene_active && !alpha) scene_depth[off]=iz;
    }
}
void pix3d_scene_triangle(Pix3DVertex a, Pix3DVertex b, Pix3DVertex c, int texture, int alpha) {
    if(scene_active && alpha && !flushing_alpha) {
        if(alpha_count==alpha_capacity) {
            size_t capacity=alpha_capacity ? alpha_capacity*2 : 256;
            AlphaTriangle *next=realloc(alpha_triangles,capacity*sizeof(AlphaTriangle));
            if(!next) return;
            alpha_triangles=next; alpha_capacity=capacity;
        }
        alpha_triangles[alpha_count++]=(AlphaTriangle){a,b,c,texture,alpha,a.z+b.z+c.z};
        return;
    }
    Pix3DVertex input[3]={a,b,c}, clipped[4]; int count=0;
    for(int i=0;i<3;i++) {
        Pix3DVertex p=input[i], q=input[(i+1)%3];
        if(p.z>=50) clipped[count++]=p;
        if((p.z>=50)!=(q.z>=50)) {
            double t=(50-p.z)/(q.z-p.z);
            clipped[count++]=(Pix3DVertex){p.x+(q.x-p.x)*t,p.y+(q.y-p.y)*t,50,
                p.u+(q.u-p.u)*t,p.v+(q.v-p.v)*t,p.shade+(q.shade-p.shade)*t};
        }
    }
    for(int i=1;i+1<count;i++) scene_raster(clipped[0],clipped[i],clipped[i+1],texture,alpha);
}
