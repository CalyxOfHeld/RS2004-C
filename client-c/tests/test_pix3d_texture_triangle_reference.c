#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/pix3d_texture225.h"
#include "gfx/pix3d_texture_cache225.h"
#include "gfx/pix8.h"
#include "core/jagfile.h"
#include "core/jrandom.h"
#include <stdio.h>
#include <stdlib.h>

static int read_i32(FILE *f,int32_t *v) {
    unsigned char b[4];
    if(fread(b,1,4,f)!=4) return 0;
    *v=(int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);
    return 1;
}
int main(int argc,char **argv) {
    if(argc!=2&&argc!=3) { fprintf(stderr,"usage: texture-triangle-reference fixture [archive]\n"); return 2; }
    FILE *f=fopen(argv[1],"rb");
    if(!f) { perror(argv[1]); return 2; }
    int32_t magic;
    if(!read_i32(f,&magic)||magic!=0x50335432) { fprintf(stderr,"bad P3T2 fixture\n"); return 2; }
    pix3d_init();
    Jagfile *archive=argc==3?jagfile_load_file(argv[2],NULL):NULL;
    if(argc==3&&!archive) return 2;
    long cases=0,words=0,differences=0;
    for(int detail=0;detail<2;detail++) {
        int32_t found,count;
        if(!read_i32(f,&found)||found!=detail||!read_i32(f,&count)) return 2;
        pix3d_low_detail=detail==0;
        if(archive) {
            pix3d225_texture_cache_init(detail==0);
            for(int id=0;id<50;id++) {
                char name[8];snprintf(name,sizeof(name),"%d",id);
                Pix8 *image=pix8_load(archive,name,0,NULL);
                if(!image) continue;
                if(detail==0&&image->crop_w==128) pix8_shrink(image,NULL);
                else pix8_crop(image,NULL);
                pix3d225_texture_cache_set_image(id,image);
                pix8_free(image);
            }
            JRandom random;jrandom_set_seed(&random,4225+detail);
            double effective=0.8+(jrandom_next_double(&random)*0.03-0.015);
            pix3d225_texture_cache_set_brightness(effective);
        }
        for(int n=0;n<count;n++) {
            int32_t id,opaque,texel_count,case_count;
            if(!read_i32(f,&id)||!read_i32(f,&opaque)||!read_i32(f,&texel_count)) return 2;
            if(texel_count!=(detail==0?16384:65536)) return 2;
            uint32_t *texels=malloc((size_t)texel_count*sizeof(uint32_t));
            if(!texels) return 2;
            for(int i=0;i<texel_count;i++) { int32_t value; if(!read_i32(f,&value)) return 2; texels[i]=(uint32_t)value; }
            if(archive) {
                bool native_opaque=false;
                const uint32_t *native_texels=pix3d225_texture_cache_get(id,&native_opaque);
                if(!native_texels) return 2;
                if(native_opaque!=(opaque!=0)) differences++;
                for(int i=0;i<texel_count;i++) if(native_texels[i]!=texels[i]) differences++;
            }
            if(!read_i32(f,&case_count)) return 2;
            for(int ncase=0;ncase<case_count;ncase++) {
                int32_t c[22],length;
                for(int i=0;i<22;i++) if(!read_i32(f,&c[i])) return 2;
                if(!read_i32(f,&length)||length!=c[0]*c[1]+8) return 2;
                uint32_t *data=malloc((size_t)length*sizeof(uint32_t));
                if(!data) return 2;
                for(int i=0;i<length;i++) data[i]=((uint32_t)i*0x045D9F3Bu^(uint32_t)c[21])&0xFFFFFFu;
                pix2d_bind(data,c[0],c[1]); pix3d_init_2d();
                pix3d_hclip=c[2]!=0;
                if(archive) pix3d225_texture_cache_triangle(id,&c[3]);
                else pix3d225_texture_triangle(texels,opaque!=0,&c[3]);
                for(int i=0;i<length;i++) {
                    int32_t expected; if(!read_i32(f,&expected)) return 2;
                    if(data[i]!=(uint32_t)expected) {
                        if(differences<10) fprintf(stderr,"detail=%d id=%d case=%d word=%d got=%08X expected=%08X\n",
                            detail,id,ncase,i,(unsigned)data[i],(unsigned)expected);
                        differences++;
                    }
                }
                cases++;words+=length;free(data);
            }
            free(texels);
        }
        if(archive) pix3d225_texture_cache_clear();
    }
    fclose(f);
    printf("Pix3D texture triangle: %ld cases, %ld framebuffer/guard words, %ld mismatches\n",cases,words,differences);
    return differences?1:0;
}
