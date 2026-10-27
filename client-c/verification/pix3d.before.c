#include "gfx/pix3d.h"
#include "gfx/pix2d.h"
#include <math.h>
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

void pix3d_init(void) {
    for (int32_t i = 1; i < 512; i++) {
        pix3d_div_table[i] = 32768 / i;
    }
    for (int32_t i = 1; i < 2048; i++) {
        pix3d_div_table2[i] = 65536 / i;
    }
    for (int32_t i = 0; i < 2048; i++) {
        pix3d_sin_table[i] = (int32_t)(sin((double)i * 0.0030679615757712823) * 65536.0);
        pix3d_cos_table[i] = (int32_t)(cos((double)i * 0.0030679615757712823) * 65536.0);
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

static void flat_raster(int32_t xA, int32_t xB, uint32_t *dst, int32_t off, uint32_t colour) {
    if (xB <= 0 || xA >= pix2d_width) return;
    if (xA < 0) xA = 0;
    if (xB > pix2d_width) xB = pix2d_width;
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

void pix3d_flat_triangle(
    int32_t xA, int32_t xB, int32_t xC,
    int32_t yA, int32_t yB, int32_t yC,
    uint32_t colour
) {
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
}

static void gouraud_raster(
    int32_t xA, int32_t xB,
    int32_t colA, int32_t colB,
    uint32_t *dst, int32_t off
) {
    if (xB <= 0 || xA >= pix2d_width) return;
    int32_t width = xB - xA;
    if (width <= 0) return;

    int32_t dCol = (colB - colA) / width;
    if (xA < 0) {
        colA -= xA * dCol;
        xA = 0;
    }
    if (xB > pix2d_width) {
        xB = pix2d_width;
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

void pix3d_gouraud_triangle(
    int32_t xA, int32_t xB, int32_t xC,
    int32_t yA, int32_t yB, int32_t yC,
    int32_t colourA, int32_t colourB, int32_t colourC
) {
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
}
