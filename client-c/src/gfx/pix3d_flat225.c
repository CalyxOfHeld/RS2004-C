/* Revision 225 (May 18, 2004), bundled jagex2/graphics/Pix3D.java:
 * flatTriangle lines 1014-1410; flatRaster lines 1413-1473.
 * Reproduce with verification/completion/tools/port_pix3d_flat.py.
 * Branches/edge updates retain Java names; do not sort/rewrite the vertices.
 */
#include "gfx/pix3d_flat225.h"
#include "gfx/pix3d.h"
#include "gfx/pix2d.h"

/* Defined Java int arithmetic, including negative shifts and overflow. */
static int32_t jint(uint32_t v) {
    return v <= INT32_MAX ? (int32_t)v : -1 - (int32_t)(UINT32_MAX-v);
}
static int32_t jadd(int32_t a, int32_t b) { return jint((uint32_t)a+(uint32_t)b); }
static int32_t jsub(int32_t a, int32_t b) { return jint((uint32_t)a-(uint32_t)b); }
static int32_t jmul(int32_t a, int32_t b) { return jint((uint32_t)a*(uint32_t)b); }
static int32_t jshl16(int32_t a) { return jint((uint32_t)a<<16); }
static int32_t jshr16(int32_t a) {
    uint32_t v=(uint32_t)a>>16;
    return jint(a<0 ? v|UINT32_C(0xFFFF0000) : v);
}
static int32_t jdiv(int32_t a, int32_t b) {
    return a==INT32_MIN && b==-1 ? INT32_MIN : a/b;
}

/* Original four-word groups and tail are equivalent to this sequential loop.
 * Keep source/destination channel truncation separate. Java evaluates the
 * destination index's postincrement before reading the RHS: blend dst[i+1]. */
void pix3d225_flat_raster(uint32_t *dst, int32_t offset, uint32_t rgb,
                         int32_t left, int32_t right) {
    if (pix3d_hclip) {
        if (right > pix2d_safe_width) right=pix2d_safe_width;
        if (left < 0) left=0;
    }
    if (left >= right) return;
    offset+=left;
    if (pix3d_trans==0) {
        for (int32_t i=left;i<right;i++) dst[offset++]=rgb;
        return;
    }
    uint32_t alpha=(uint32_t)pix3d_trans, inv=256u-alpha;
    uint32_t src=(((rgb & 0xFF00FFu)*inv>>8)&0xFF00FFu)
                + (((rgb & 0xFF00u)*inv>>8)&0xFF00u);
    for (int32_t i=left;i<right;i++,offset++) {
        uint32_t next=dst[offset+1];
        dst[offset]=src+(((next & 0xFF00FFu)*alpha>>8)&0xFF00FFu)
                       + (((next & 0xFF00u)*alpha>>8)&0xFF00u);
    }
}

void pix3d225_flat_triangle(int32_t xA, int32_t xB, int32_t xC,
                           int32_t yA, int32_t yB, int32_t yC, uint32_t rgb) {
    int32_t arg0=yA, arg1=yB, arg2=yC, arg3=xA, arg4=xB, arg5=xC;
    int32_t arg6=jint(rgb);
    int32_t var7 = 0;
    if (arg1 != arg0) {
        var7 = jdiv(jshl16(jsub(arg4, arg3)), jsub(arg1, arg0));
    }
    int32_t var8 = 0;
    if (arg2 != arg1) {
        var8 = jdiv(jshl16(jsub(arg5, arg4)), jsub(arg2, arg1));
    }
    int32_t var9 = 0;
    if (arg2 != arg0) {
        var9 = jdiv(jshl16(jsub(arg3, arg5)), jsub(arg0, arg2));
    }
    if (arg0 <= arg1 && arg0 <= arg2) {
        if (arg0 < pix2d_clip_bottom) {
            if (arg1 > pix2d_clip_bottom) {
                arg1 = pix2d_clip_bottom;
            }
            if (arg2 > pix2d_clip_bottom) {
                arg2 = pix2d_clip_bottom;
            }
            if (arg1 < arg2) {
                arg5 = arg3 = jshl16(arg3);
                if (arg0 < 0) {
                    arg5 = jsub(arg5, jmul(var9, arg0));
                    arg3 = jsub(arg3, jmul(var7, arg0));
                    arg0 = 0;
                }
                arg4 = jshl16(arg4);
                if (arg1 < 0) {
                    arg4 = jsub(arg4, jmul(var8, arg1));
                    arg1 = 0;
                }
                if ((arg0 != arg1 && var9 < var7) || (arg0 == arg1 && var9 > var8)) {
                    arg2 = jsub(arg2, arg1);
                    arg1 = jsub(arg1, arg0);
                    arg0 = pix3d_scanline[arg0];
                    while (true) {
                        arg1--;
                        if (arg1 < 0) {
                            while (true) {
                                arg2--;
                                if (arg2 < 0) {
                                    return;
                                }
                                pix3d225_flat_raster(pix2d_pixels, arg0, (uint32_t)arg6, jshr16(arg5), jshr16(arg4));
                                arg5 = jadd(arg5, var9);
                                arg4 = jadd(arg4, var8);
                                arg0 = jadd(arg0, pix2d_width);
                            }
                        }
                        pix3d225_flat_raster(pix2d_pixels, arg0, (uint32_t)arg6, jshr16(arg5), jshr16(arg3));
                        arg5 = jadd(arg5, var9);
                        arg3 = jadd(arg3, var7);
                        arg0 = jadd(arg0, pix2d_width);
                    }
                } else {
                    arg2 = jsub(arg2, arg1);
                    arg1 = jsub(arg1, arg0);
                    arg0 = pix3d_scanline[arg0];
                    while (true) {
                        arg1--;
                        if (arg1 < 0) {
                            while (true) {
                                arg2--;
                                if (arg2 < 0) {
                                    return;
                                }
                                pix3d225_flat_raster(pix2d_pixels, arg0, (uint32_t)arg6, jshr16(arg4), jshr16(arg5));
                                arg5 = jadd(arg5, var9);
                                arg4 = jadd(arg4, var8);
                                arg0 = jadd(arg0, pix2d_width);
                            }
                        }
                        pix3d225_flat_raster(pix2d_pixels, arg0, (uint32_t)arg6, jshr16(arg3), jshr16(arg5));
                        arg5 = jadd(arg5, var9);
                        arg3 = jadd(arg3, var7);
                        arg0 = jadd(arg0, pix2d_width);
                    }
                }
            } else {
                arg4 = arg3 = jshl16(arg3);
                if (arg0 < 0) {
                    arg4 = jsub(arg4, jmul(var9, arg0));
                    arg3 = jsub(arg3, jmul(var7, arg0));
                    arg0 = 0;
                }
                arg5 = jshl16(arg5);
                if (arg2 < 0) {
                    arg5 = jsub(arg5, jmul(var8, arg2));
                    arg2 = 0;
                }
                if ((arg0 != arg2 && var9 < var7) || (arg0 == arg2 && var8 > var7)) {
                    arg1 = jsub(arg1, arg2);
                    arg2 = jsub(arg2, arg0);
                    arg0 = pix3d_scanline[arg0];
                    while (true) {
                        arg2--;
                        if (arg2 < 0) {
                            while (true) {
                                arg1--;
                                if (arg1 < 0) {
                                    return;
                                }
                                pix3d225_flat_raster(pix2d_pixels, arg0, (uint32_t)arg6, jshr16(arg5), jshr16(arg3));
                                arg5 = jadd(arg5, var8);
                                arg3 = jadd(arg3, var7);
                                arg0 = jadd(arg0, pix2d_width);
                            }
                        }
                        pix3d225_flat_raster(pix2d_pixels, arg0, (uint32_t)arg6, jshr16(arg4), jshr16(arg3));
                        arg4 = jadd(arg4, var9);
                        arg3 = jadd(arg3, var7);
                        arg0 = jadd(arg0, pix2d_width);
                    }
                } else {
                    arg1 = jsub(arg1, arg2);
                    arg2 = jsub(arg2, arg0);
                    arg0 = pix3d_scanline[arg0];
                    while (true) {
                        arg2--;
                        if (arg2 < 0) {
                            while (true) {
                                arg1--;
                                if (arg1 < 0) {
                                    return;
                                }
                                pix3d225_flat_raster(pix2d_pixels, arg0, (uint32_t)arg6, jshr16(arg3), jshr16(arg5));
                                arg5 = jadd(arg5, var8);
                                arg3 = jadd(arg3, var7);
                                arg0 = jadd(arg0, pix2d_width);
                            }
                        }
                        pix3d225_flat_raster(pix2d_pixels, arg0, (uint32_t)arg6, jshr16(arg3), jshr16(arg4));
                        arg4 = jadd(arg4, var9);
                        arg3 = jadd(arg3, var7);
                        arg0 = jadd(arg0, pix2d_width);
                    }
                }
            }
        }
    } else if (arg1 <= arg2) {
        if (arg1 < pix2d_clip_bottom) {
            if (arg2 > pix2d_clip_bottom) {
                arg2 = pix2d_clip_bottom;
            }
            if (arg0 > pix2d_clip_bottom) {
                arg0 = pix2d_clip_bottom;
            }
            if (arg2 < arg0) {
                arg3 = arg4 = jshl16(arg4);
                if (arg1 < 0) {
                    arg3 = jsub(arg3, jmul(var7, arg1));
                    arg4 = jsub(arg4, jmul(var8, arg1));
                    arg1 = 0;
                }
                arg5 = jshl16(arg5);
                if (arg2 < 0) {
                    arg5 = jsub(arg5, jmul(var9, arg2));
                    arg2 = 0;
                }
                if ((arg1 != arg2 && var7 < var8) || (arg1 == arg2 && var7 > var9)) {
                    arg0 = jsub(arg0, arg2);
                    arg2 = jsub(arg2, arg1);
                    arg1 = pix3d_scanline[arg1];
                    while (true) {
                        arg2--;
                        if (arg2 < 0) {
                            while (true) {
                                arg0--;
                                if (arg0 < 0) {
                                    return;
                                }
                                pix3d225_flat_raster(pix2d_pixels, arg1, (uint32_t)arg6, jshr16(arg3), jshr16(arg5));
                                arg3 = jadd(arg3, var7);
                                arg5 = jadd(arg5, var9);
                                arg1 = jadd(arg1, pix2d_width);
                            }
                        }
                        pix3d225_flat_raster(pix2d_pixels, arg1, (uint32_t)arg6, jshr16(arg3), jshr16(arg4));
                        arg3 = jadd(arg3, var7);
                        arg4 = jadd(arg4, var8);
                        arg1 = jadd(arg1, pix2d_width);
                    }
                } else {
                    arg0 = jsub(arg0, arg2);
                    arg2 = jsub(arg2, arg1);
                    arg1 = pix3d_scanline[arg1];
                    while (true) {
                        arg2--;
                        if (arg2 < 0) {
                            while (true) {
                                arg0--;
                                if (arg0 < 0) {
                                    return;
                                }
                                pix3d225_flat_raster(pix2d_pixels, arg1, (uint32_t)arg6, jshr16(arg5), jshr16(arg3));
                                arg3 = jadd(arg3, var7);
                                arg5 = jadd(arg5, var9);
                                arg1 = jadd(arg1, pix2d_width);
                            }
                        }
                        pix3d225_flat_raster(pix2d_pixels, arg1, (uint32_t)arg6, jshr16(arg4), jshr16(arg3));
                        arg3 = jadd(arg3, var7);
                        arg4 = jadd(arg4, var8);
                        arg1 = jadd(arg1, pix2d_width);
                    }
                }
            } else {
                arg5 = arg4 = jshl16(arg4);
                if (arg1 < 0) {
                    arg5 = jsub(arg5, jmul(var7, arg1));
                    arg4 = jsub(arg4, jmul(var8, arg1));
                    arg1 = 0;
                }
                arg3 = jshl16(arg3);
                if (arg0 < 0) {
                    arg3 = jsub(arg3, jmul(var9, arg0));
                    arg0 = 0;
                }
                if (var7 < var8) {
                    arg2 = jsub(arg2, arg0);
                    arg0 = jsub(arg0, arg1);
                    arg1 = pix3d_scanline[arg1];
                    while (true) {
                        arg0--;
                        if (arg0 < 0) {
                            while (true) {
                                arg2--;
                                if (arg2 < 0) {
                                    return;
                                }
                                pix3d225_flat_raster(pix2d_pixels, arg1, (uint32_t)arg6, jshr16(arg3), jshr16(arg4));
                                arg3 = jadd(arg3, var9);
                                arg4 = jadd(arg4, var8);
                                arg1 = jadd(arg1, pix2d_width);
                            }
                        }
                        pix3d225_flat_raster(pix2d_pixels, arg1, (uint32_t)arg6, jshr16(arg5), jshr16(arg4));
                        arg5 = jadd(arg5, var7);
                        arg4 = jadd(arg4, var8);
                        arg1 = jadd(arg1, pix2d_width);
                    }
                } else {
                    arg2 = jsub(arg2, arg0);
                    arg0 = jsub(arg0, arg1);
                    arg1 = pix3d_scanline[arg1];
                    while (true) {
                        arg0--;
                        if (arg0 < 0) {
                            while (true) {
                                arg2--;
                                if (arg2 < 0) {
                                    return;
                                }
                                pix3d225_flat_raster(pix2d_pixels, arg1, (uint32_t)arg6, jshr16(arg4), jshr16(arg3));
                                arg3 = jadd(arg3, var9);
                                arg4 = jadd(arg4, var8);
                                arg1 = jadd(arg1, pix2d_width);
                            }
                        }
                        pix3d225_flat_raster(pix2d_pixels, arg1, (uint32_t)arg6, jshr16(arg4), jshr16(arg5));
                        arg5 = jadd(arg5, var7);
                        arg4 = jadd(arg4, var8);
                        arg1 = jadd(arg1, pix2d_width);
                    }
                }
            }
        }
    } else if (arg2 < pix2d_clip_bottom) {
        if (arg0 > pix2d_clip_bottom) {
            arg0 = pix2d_clip_bottom;
        }
        if (arg1 > pix2d_clip_bottom) {
            arg1 = pix2d_clip_bottom;
        }
        if (arg0 < arg1) {
            arg4 = arg5 = jshl16(arg5);
            if (arg2 < 0) {
                arg4 = jsub(arg4, jmul(var8, arg2));
                arg5 = jsub(arg5, jmul(var9, arg2));
                arg2 = 0;
            }
            arg3 = jshl16(arg3);
            if (arg0 < 0) {
                arg3 = jsub(arg3, jmul(var7, arg0));
                arg0 = 0;
            }
            if (var8 < var9) {
                arg1 = jsub(arg1, arg0);
                arg0 = jsub(arg0, arg2);
                arg2 = pix3d_scanline[arg2];
                while (true) {
                    arg0--;
                    if (arg0 < 0) {
                        while (true) {
                            arg1--;
                            if (arg1 < 0) {
                                return;
                            }
                            pix3d225_flat_raster(pix2d_pixels, arg2, (uint32_t)arg6, jshr16(arg4), jshr16(arg3));
                            arg4 = jadd(arg4, var8);
                            arg3 = jadd(arg3, var7);
                            arg2 = jadd(arg2, pix2d_width);
                        }
                    }
                    pix3d225_flat_raster(pix2d_pixels, arg2, (uint32_t)arg6, jshr16(arg4), jshr16(arg5));
                    arg4 = jadd(arg4, var8);
                    arg5 = jadd(arg5, var9);
                    arg2 = jadd(arg2, pix2d_width);
                }
            } else {
                arg1 = jsub(arg1, arg0);
                arg0 = jsub(arg0, arg2);
                arg2 = pix3d_scanline[arg2];
                while (true) {
                    arg0--;
                    if (arg0 < 0) {
                        while (true) {
                            arg1--;
                            if (arg1 < 0) {
                                return;
                            }
                            pix3d225_flat_raster(pix2d_pixels, arg2, (uint32_t)arg6, jshr16(arg3), jshr16(arg4));
                            arg4 = jadd(arg4, var8);
                            arg3 = jadd(arg3, var7);
                            arg2 = jadd(arg2, pix2d_width);
                        }
                    }
                    pix3d225_flat_raster(pix2d_pixels, arg2, (uint32_t)arg6, jshr16(arg5), jshr16(arg4));
                    arg4 = jadd(arg4, var8);
                    arg5 = jadd(arg5, var9);
                    arg2 = jadd(arg2, pix2d_width);
                }
            }
        } else {
            arg3 = arg5 = jshl16(arg5);
            if (arg2 < 0) {
                arg3 = jsub(arg3, jmul(var8, arg2));
                arg5 = jsub(arg5, jmul(var9, arg2));
                arg2 = 0;
            }
            arg4 = jshl16(arg4);
            if (arg1 < 0) {
                arg4 = jsub(arg4, jmul(var7, arg1));
                arg1 = 0;
            }
            if (var8 < var9) {
                arg0 = jsub(arg0, arg1);
                arg1 = jsub(arg1, arg2);
                arg2 = pix3d_scanline[arg2];
                while (true) {
                    arg1--;
                    if (arg1 < 0) {
                        while (true) {
                            arg0--;
                            if (arg0 < 0) {
                                return;
                            }
                            pix3d225_flat_raster(pix2d_pixels, arg2, (uint32_t)arg6, jshr16(arg4), jshr16(arg5));
                            arg4 = jadd(arg4, var7);
                            arg5 = jadd(arg5, var9);
                            arg2 = jadd(arg2, pix2d_width);
                        }
                    }
                    pix3d225_flat_raster(pix2d_pixels, arg2, (uint32_t)arg6, jshr16(arg3), jshr16(arg5));
                    arg3 = jadd(arg3, var8);
                    arg5 = jadd(arg5, var9);
                    arg2 = jadd(arg2, pix2d_width);
                }
            } else {
                arg0 = jsub(arg0, arg1);
                arg1 = jsub(arg1, arg2);
                arg2 = pix3d_scanline[arg2];
                while (true) {
                    arg1--;
                    if (arg1 < 0) {
                        while (true) {
                            arg0--;
                            if (arg0 < 0) {
                                return;
                            }
                            pix3d225_flat_raster(pix2d_pixels, arg2, (uint32_t)arg6, jshr16(arg5), jshr16(arg4));
                            arg4 = jadd(arg4, var7);
                            arg5 = jadd(arg5, var9);
                            arg2 = jadd(arg2, pix2d_width);
                        }
                    }
                    pix3d225_flat_raster(pix2d_pixels, arg2, (uint32_t)arg6, jshr16(arg5), jshr16(arg3));
                    arg3 = jadd(arg3, var8);
                    arg5 = jadd(arg5, var9);
                    arg2 = jadd(arg2, pix2d_width);
                }
            }
        }
    }
}
