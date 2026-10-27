/* Pix3D.java textureRaster 2073-2471. Perspective and shade advance every
 * eight pixels as in the bundled revision-225 client. */
#include "gfx/pix3d_texture225.h"
#include "gfx/pix3d.h"
#include "gfx/pix2d.h"
#include <limits.h>

static int32_t jint(uint32_t v) { return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v); }
static int32_t jadd(int32_t a,int32_t b) { return jint((uint32_t)a+(uint32_t)b); }
static int32_t jsub(int32_t a,int32_t b) { return jint((uint32_t)a-(uint32_t)b); }
static int32_t jmul(int32_t a,int32_t b) { return jint((uint32_t)a*(uint32_t)b); }
static int32_t jshl(int32_t a,unsigned n) { return jint((uint32_t)a<<(n&31)); }
static int32_t jshr(int32_t a,unsigned n) {
    n&=31; if(!n) return a;
    uint32_t v=(uint32_t)a>>n;
    return jint(a<0?v|(UINT32_MAX<<(32-n)):v);
}
static int32_t jdiv(int32_t a,int32_t b) { return a==INT32_MIN && b==-1?INT32_MIN:a/b; }

void pix3d225_texture_raster(uint32_t *dst,const uint32_t *texels,
    int32_t u,int32_t v,int32_t offset,int32_t left,int32_t right,
    int32_t shadeA,int32_t shadeB,int32_t a,int32_t b,int32_t c,
    int32_t da,int32_t db,int32_t dc,bool opaque) {
    if(left>=right) return;
    int32_t shadeStep,groups;
    if(pix3d_hclip) {
        shadeStep=jdiv(jsub(shadeB,shadeA),jsub(right,left));
        if(right>pix2d_safe_width) right=pix2d_safe_width;
        if(left<0) { shadeA=jsub(shadeA,jmul(left,shadeStep)); left=0; }
        if(left>=right) return;
        groups=jshr(jsub(right,left),3);
        shadeStep=jshl(shadeStep,12);
        shadeA=jshl(shadeA,9);
    } else {
        groups=jsub(right,left)>7?jshr(jsub(right,left),3):0;
        shadeStep=groups?jshr(jmul(jsub(shadeB,shadeA),pix3d_div_table[groups]),6):0;
        shadeA=jshl(shadeA,9);
    }
    offset=jadd(offset,left);
    int32_t centerDelta=jsub(left,pix3d_center_x);
    int32_t aa=jadd(a,jmul(jshr(da,3),centerDelta));
    int32_t bb=jadd(b,jmul(jshr(db,3),centerDelta));
    int32_t cc=jadd(c,jmul(jshr(dc,3),centerDelta));
    int32_t denominator=jshr(cc,pix3d_low_detail?12:14);
    int32_t nextU=0,nextV=0;
    int32_t maxU=pix3d_low_detail?4032:16256;
    if(denominator) {
        u=jdiv(aa,denominator);
        v=jdiv(bb,denominator);
        if(u<0) u=0; else if(u>maxU) u=maxU;
    }
    a=jadd(aa,da); b=jadd(bb,db); c=jadd(cc,dc);
    denominator=jshr(c,pix3d_low_detail?12:14);
    if(denominator) {
        nextU=jdiv(a,denominator);
        nextV=jdiv(b,denominator);
        if(nextU<7) nextU=7; else if(nextU>maxU) nextU=maxU;
    }
    int32_t du=jshr(jsub(nextU,u),3);
    int32_t dv=jshr(jsub(nextV,v),3);
    u=jadd(u,pix3d_low_detail?(jshr(shadeA,3)&0xC0000):(shadeA&0x600000));
    int32_t shift=jshr(shadeA,23)&31;
    int32_t tail=jsub(right,left)&7;
    for(int32_t group=0;group<groups;group++) {
        for(int pixel=0;pixel<8;pixel++) {
            int32_t index=pix3d_low_detail?((v&0xFC0)+jshr(u,6)):((v&0x3F80)+jshr(u,7));
            uint32_t rgb=texels[index]>>shift;
            if(opaque || rgb) dst[offset]=rgb;
            offset=jadd(offset,1);
            u=jadd(u,du); v=jadd(v,dv);
        }
        u=nextU; v=nextV;
        a=jadd(a,da); b=jadd(b,db); c=jadd(c,dc);
        denominator=jshr(c,pix3d_low_detail?12:14);
        if(denominator) {
            nextU=jdiv(a,denominator);
            nextV=jdiv(b,denominator);
            if(nextU<7) nextU=7; else if(nextU>maxU) nextU=maxU;
        }
        du=jshr(jsub(nextU,u),3); dv=jshr(jsub(nextV,v),3);
        shadeA=jadd(shadeA,shadeStep);
        u=jadd(u,pix3d_low_detail?(jshr(shadeA,3)&0xC0000):(shadeA&0x600000));
        shift=jshr(shadeA,23)&31;
    }
    while(tail-- > 0) {
        int32_t index=pix3d_low_detail?((v&0xFC0)+jshr(u,6)):((v&0x3F80)+jshr(u,7));
        uint32_t rgb=texels[index]>>shift;
        if(opaque || rgb) dst[offset]=rgb;
        offset=jadd(offset,1);
        u=jadd(u,du); v=jadd(v,dv);
    }
}
