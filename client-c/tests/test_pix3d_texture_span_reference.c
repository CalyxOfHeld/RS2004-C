#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/pix3d_texture225.h"
#include <stdio.h>
#include <stdlib.h>

static int read_i32(FILE *f,int32_t *v) {
    unsigned char b[4];
    if(fread(b,1,4,f)!=4) return 0;
    *v=(int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);
    return 1;
}
int main(int argc,char **argv) {
    if(argc!=2) { fprintf(stderr,"usage: texture-span-reference fixture\n"); return 2; }
    FILE *f=fopen(argv[1],"rb");
    if(!f) { perror(argv[1]); return 2; }
    int32_t magic;
    if(!read_i32(f,&magic)||magic!=0x50335431) { fprintf(stderr,"bad P3T1 fixture\n"); return 2; }
    pix3d_init();
    long cases=0,words=0,differences=0;
    for(int detail=0;detail<2;detail++) {
        int32_t found,count;
        if(!read_i32(f,&found)||found!=detail||!read_i32(f,&count)) return 2;
        pix3d_low_detail=detail==0;
        for(int n=0;n<count;n++) {
            int32_t id,opaque,texel_count,case_count;
            if(!read_i32(f,&id)||!read_i32(f,&opaque)||!read_i32(f,&texel_count)) return 2;
            if(texel_count!=(detail==0?16384:65536)) { fprintf(stderr,"bad texel count\n"); return 2; }
            uint32_t *texels=malloc((size_t)texel_count*sizeof(uint32_t));
            if(!texels) return 2;
            for(int i=0;i<texel_count;i++) { int32_t value; if(!read_i32(f,&value)) return 2; texels[i]=(uint32_t)value; }
            if(!read_i32(f,&case_count)) return 2;
            for(int ncase=0;ncase<case_count;ncase++) {
                int32_t c[15],length;
                for(int i=0;i<15;i++) if(!read_i32(f,&c[i])) return 2;
                if(!read_i32(f,&length)||length!=c[0]*c[1]+8) return 2;
                uint32_t *data=malloc((size_t)length*sizeof(uint32_t));
                if(!data) return 2;
                for(int i=0;i<length;i++) data[i]=((uint32_t)i*0x045D9F3Bu^(uint32_t)c[14])&0xFFFFFFu;
                pix2d_bind(data,c[0],c[1]);
                pix3d_init_2d();
                pix3d_hclip=c[4]!=0;
                pix3d225_texture_raster(data,texels,0,0,c[13],c[2],c[3],
                    c[5],c[6],c[7],c[8],c[9],c[10],c[11],c[12],opaque!=0);
                for(int i=0;i<length;i++) {
                    int32_t expected;
                    if(!read_i32(f,&expected)) return 2;
                    if(data[i]!=(uint32_t)expected) {
                        if(differences<10) fprintf(stderr,"detail=%d id=%d case=%d word=%d got=%08X expected=%08X\n",
                            detail,id,ncase,i,(unsigned)data[i],(unsigned)expected);
                        differences++;
                    }
                }
                words+=length; cases++; free(data);
            }
            free(texels);
        }
    }
    fclose(f);
    printf("Pix3D texture span: %ld cases, %ld framebuffer/guard words, %ld mismatches\n",cases,words,differences);
    return differences?1:0;
}
