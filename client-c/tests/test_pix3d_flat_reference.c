#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#ifndef PIX3D225_BASELINE
#include "gfx/pix3d_flat225.h"
#endif
#include <stdio.h>
#include <stdlib.h>

static uint32_t read_word(FILE *f) {
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) { fprintf(stderr,"truncated flat oracle fixture\n"); exit(2); }
    return (uint32_t)b[0]<<24 | (uint32_t)b[1]<<16 | (uint32_t)b[2]<<8 | b[3];
}
static int32_t signed_word(uint32_t v) {
    return v <= INT32_MAX ? (int32_t)v : -1 - (int32_t)(UINT32_MAX-v);
}
int main(int argc, char **argv) {
    const char *path=argc>1?argv[1]:"tests/fixtures/pix3d-flat-reference.bin";
    FILE *f=fopen(path,"rb");
    if(!f) { perror(path); return 2; }
    if(read_word(f)!=0x50334631) { fprintf(stderr,"wrong flat fixture magic\n"); return 2; }
    uint32_t cases=read_word(f), failures=0, checked=0;
    uint64_t words=0, mismatches=0;
    for(uint32_t n=0;n<cases;n++) {
        int32_t c[18];
        for(int j=0;j<18;j++) c[j]=signed_word(read_word(f));
        uint32_t count=read_word(f);
        if(c[0]<0 || c[0]>1 || c[1]<1 || c[1]>765 || c[2]<1 || c[2]>503 ||
           count!=(uint32_t)(c[1]*c[2]+8)) { fprintf(stderr,"invalid flat fixture case\n"); return 2; }
        uint32_t *data=malloc(count*sizeof(*data));
        if(!data) return 2;
        for(uint32_t j=0;j<count;j++) data[j]=(j*UINT32_C(0x045D9F3B) ^ (uint32_t)c[17]) & 0xFFFFFF;
        pix2d_bind(data,c[1],c[2]);
        pix2d_set_clip(c[3],c[4],c[5],c[6]);
        pix3d_init_2d(); pix3d_set_trans(c[7]); pix3d_set_hclip(c[8]!=0);
#ifdef PIX3D225_BASELINE
        bool skip=c[0]==0;
        if(!skip) pix3d_flat_triangle(c[10],c[11],c[12],c[13],c[14],c[15],(uint32_t)c[9]);
#else
        bool skip=false;
        if(c[0]==0) pix3d225_flat_raster(data,c[16],(uint32_t)c[9],c[10],c[11]);
#ifdef PIX3D225_VIA_PUBLIC
        else pix3d_flat_triangle(c[10],c[11],c[12],c[13],c[14],c[15],(uint32_t)c[9]);
#else
        else pix3d225_flat_triangle(c[10],c[11],c[12],c[13],c[14],c[15],(uint32_t)c[9]);
#endif
#endif
        bool bad=false;
        for(uint32_t j=0;j<count;j++) {
            uint32_t expected=read_word(f);
            if(!skip && data[j]!=expected) {
                if(!bad && failures<5) fprintf(stderr,"case %u mode=%d alpha=%d hclip=%d word=%u: C=%08x Java=%08x\n",
                    n,c[0],c[7],c[8],j,data[j],expected);
                bad=true; mismatches++;
            }
        }
        if(!skip) { checked++; words+=count; if(bad) failures++; }
        free(data);
    }
    if(fgetc(f)!=EOF) { fprintf(stderr,"trailing flat fixture data\n"); return 2; }
    fclose(f); free(pix3d_scanline); pix3d_scanline=NULL;
    printf("Pix3D flat reference: %u/%u cases match; %llu words checked, %llu mismatches\n",
           checked-failures,checked,(unsigned long long)words,(unsigned long long)mismatches);
    return failures?1:0;
}
