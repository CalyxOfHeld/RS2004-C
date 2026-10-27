#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/model.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/pix3d_gouraud225.h"
#include "gfx/pix3d_texture_cache225.h"
#include "gfx/pix8.h"
#include <stdio.h>
#include <stdlib.h>

static uint32_t word(FILE *f) {
    unsigned char b[4];
    if(fread(b,1,4,f)!=4) { fputs("truncated model fixture\n",stderr); exit(2); }
    return (uint32_t)b[0]<<24 | (uint32_t)b[1]<<16 | (uint32_t)b[2]<<8 | b[3];
}
static int32_t integer(FILE *f) {
    uint32_t v=word(f);
    return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);
}
int main(int argc,char **argv) {
    if(argc!=3 && argc!=4) { fputs("usage: test_model_draw_reference models-archive fixture [textures-archive]\n",stderr); return 2; }
    FILE *f=fopen(argv[2],"rb");
    if(!f) { perror(argv[2]); return 2; }
    uint32_t magic=word(f);
    if(magic!=0x4D445232 && magic!=0x4D445233) { fputs("expected MDR2/MDR3 model fixture\n",stderr); return 2; }
    union { uint64_t u; double d; } brightness;
    brightness.u=(uint64_t)word(f)<<32; brightness.u|=word(f);
    if(!(brightness.d>0.7 && brightness.d<0.9)) return 2;
    pix3d_init(); pix3d_init_colour_table(brightness.d);
    Arena arena=arena_create(64*1024*1024);
    Jagfile *archive=jagfile_load_file(argv[1],&arena);
    if(!archive) { fputs("cannot load model archive\n",stderr); return 2; }
    model_init_repo(archive,&arena);
    Jagfile *textures=argc==4?jagfile_load_file(argv[3],&arena):NULL;
    int detail=-1;
    uint32_t cases=word(f),failures=0;
    uint64_t mismatches=0,words=0;
    if(!cases || cases>10000) return 2;
    for(uint32_t n=0;n<cases;n++) {
        int32_t c[10]={0};
        if(magic==0x4D445233) for(int k=0;k<10;k++) c[k]=integer(f);
        else { for(int k=0;k<4;k++) c[k]=integer(f); c[7]=integer(f);c[8]=c[9]=1; }
        int32_t vertices=integer(f),faces=integer(f);
        uint32_t count=word(f);
        if(count!=128*128+8 || c[0]<0) return 2;
        Model *m=model_load(c[0],&arena);
        if(!m || m->num_points!=vertices || m->num_faces!=faces) { fputs("model metadata differs\n",stderr); return 1; }
        model_calculate_normals(m,64,768,-50,-10,-50);
        if(detail!=c[8]) {
            detail=c[8];pix3d_low_detail=detail!=0;
            pix3d225_texture_cache_init(pix3d_low_detail);
            if(textures) for(int id=0;id<50;id++) {
                char name[8]; snprintf(name,sizeof(name),"%d",id);
                Pix8 *image=pix8_load(textures,name,0,NULL);
                if(!image) return 2;
                if(pix3d_low_detail && image->crop_w==128) pix8_shrink(image,NULL);
                else pix8_crop(image,NULL);
                pix3d225_texture_cache_set_image(id,image);pix8_free(image);
            }
            pix3d225_texture_cache_set_brightness(brightness.d);
        }
        pix3d_jagged=c[9]!=0;
        uint32_t pixels[128*128+8];
        for(uint32_t i=0;i<count;i++) pixels[i]=0x19372B;
        pix2d_bind(pixels,128,128); pix3d_init_2d();
        pix3d_set_hclip(false); pix3d_set_trans(0);
#ifdef MODEL_DRAW225_BASELINE
        model_render_obj(m,c[1],c[2],c[3],c[4],c[5],c[6],c[7]);
#else
        if(!model_render_simple225(m,c[1],c[2],c[3],c[4],c[5],c[6],c[7])) return 2;
#endif
        uint32_t differences=0,guard_differences=0;
        for(uint32_t i=0;i<count;i++) {
            uint32_t expected=word(f);
            if(pixels[i]!=expected) {
                if(!differences) printf("case %u model=%d yaw=%d first word=%u C=%08x Java=%08x\n",n,c[0],c[2],i,pixels[i],expected);
                differences++;
                if(i>=128*128) guard_differences++;
            }
        }
        printf("case %u model=%d: %u differences, %u guard differences\n",n,c[0],differences,guard_differences);
        words+=count; mismatches+=differences; failures+=differences!=0;
    }
    if(fgetc(f)!=EOF) { fputs("trailing model fixture data\n",stderr); return 2; }
    fclose(f); free(pix3d_scanline); pix3d_scanline=NULL;
    pix3d225_texture_cache_clear();arena_free(&arena);
    printf("Model draw reference: %u/%u cases match; %llu words checked, %llu mismatches\n",cases-failures,cases,(unsigned long long)words,(unsigned long long)mismatches);
    return failures?1:0;
}
