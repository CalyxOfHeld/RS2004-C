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
    if(fread(b,1,4,f)!=4) {fputs("truncated world fixture\n",stderr);exit(2);}
    return (uint32_t)b[0]<<24 | (uint32_t)b[1]<<16 | (uint32_t)b[2]<<8 | b[3];
}
static int32_t integer(FILE *f) {
    uint32_t v=word(f);return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);
}
int main(int argc,char **argv) {
    if(argc!=4) {fputs("usage: test_model_world_reference models-archive fixture textures-archive\n",stderr);return 2;}
    FILE *f=fopen(argv[2],"rb");if(!f){perror(argv[2]);return 2;}
    if(word(f)!=0x4d575231){fputs("expected MWR1 fixture\n",stderr);return 2;}
    union {uint64_t u;double d;} brightness;
    brightness.u=(uint64_t)word(f)<<32;brightness.u|=word(f);
    if(!(brightness.d>0.7 && brightness.d<0.9))return 2;
    pix3d_init();pix3d_init_colour_table(brightness.d);
    Arena arena=arena_create(64*1024*1024);
    Jagfile *models=jagfile_load_file(argv[1],&arena),*textures=jagfile_load_file(argv[3],&arena);
    if(!models || !textures)return 2;
    model_init_repo(models,&arena);
    uint32_t cases=word(f),failed=0;uint64_t differences=0,checked=0,state_differences=0;
    if(!cases || cases>10000)return 2;
    int detail=-1;
    for(uint32_t n=0;n<cases;n++) {
        int32_t c[11];for(int k=0;k<11;k++)c[k]=integer(f);
        int32_t vertices=integer(f),faces=integer(f),alpha=integer(f),hclip=integer(f);
        uint32_t count=word(f);
        if(count!=128*128+8 || c[0]<0 || c[7]<0 || c[7]>1 || c[8]<0 || c[8]>1 || c[9]<0 || c[9]>256)return 2;
        Model *m=model_load(c[0],&arena);
        if(!m || m->num_points!=vertices || m->num_faces!=faces){fputs("world model metadata differs\n",stderr);return 1;}
        model_calculate_normals(m,64,768,-50,-10,-50);
        if(detail!=c[7]) {
            detail=c[7];pix3d_low_detail=detail!=0;pix3d225_texture_cache_init(pix3d_low_detail);
            for(int id=0;id<50;id++) {
                char name[8];snprintf(name,sizeof(name),"%d",id);
                Pix8 *image=pix8_load(textures,name,0,NULL);if(!image)return 2;
                if(pix3d_low_detail && image->crop_w==128)pix8_shrink(image,NULL);else pix8_crop(image,NULL);
                pix3d225_texture_cache_set_image(id,image);
#ifdef MODEL_WORLD225_BASELINE
                pix3d_set_texture_palette_gamma(id,image->palette,image->palette_count,brightness.d);
                size_t total=(size_t)image->width*image->height;
                uint32_t *rgb=malloc(total*sizeof(*rgb));if(!rgb)return 2;
                for(size_t k=0;k<total;k++) {
                    rgb[k]=image->pixels[k]?pix3d_gamma_correct(image->palette[image->pixels[k]],brightness.d):0;
                    if(image->pixels[k] && !rgb[k])rgb[k]=1;
                }
                pix3d_set_texture(id,rgb,image->width,image->height);free(rgb);
#endif
                pix8_free(image);
            }
            pix3d225_texture_cache_set_brightness(brightness.d);
        }
        pix3d_jagged=c[8]!=0;
        uint32_t storage[128*128+16],*pixels=storage+8;
        for(uint32_t j=0;j<count+8;j++)storage[j]=0x19372b;
        pix2d_bind(pixels,128,128);pix3d_init_2d();pix3d_set_trans(c[9]);pix3d_set_hclip(c[10]!=0);
#ifdef MODEL_WORLD225_BASELINE
        pix3d_begin_scene();
        model_draw(m,c[1],c[2],c[3],c[4],c[5],c[6]);
        pix3d_end_scene();
#else
        if(!model_render_world225(m,c[1],c[2],c[3],c[4],c[5],c[6])) {
            fprintf(stderr,"world render rejected case %u model=%d\n",n,c[0]);return 2;
        }
#endif
        uint32_t mismatch=0;
        for(uint32_t j=0;j<count;j++) {
            uint32_t expected=word(f);
            if(pixels[j]!=expected) {
                if(!mismatch && failed<8)printf("case %u model=%d first=%u C=%08x Java=%08x\n",n,c[0],j,pixels[j],expected);
                mismatch++;
            }
        }
        for(int j=0;j<8;j++)if(storage[j]!=0x19372b){fputs("world prefix guard changed\n",stderr);return 1;}
        bool state=pix3d_trans!=alpha || pix3d_hclip!=(hclip!=0);
        if(state && failed<8)printf("case %u model=%d state C=%d/%d Java=%d/%d\n",n,c[0],pix3d_trans,pix3d_hclip,alpha,hclip);
        if(mismatch || state) {if(failed<8)printf("case %u: %u differing words\n",n,mismatch);failed++;}
        differences+=mismatch;checked+=count;state_differences+=state;
    }
    if(fgetc(f)!=EOF){fputs("trailing world fixture data\n",stderr);return 2;}
    fclose(f);free(pix3d_scanline);pix3d_scanline=NULL;pix3d225_texture_cache_clear();arena_free(&arena);
    printf("Model world reference: %u/%u match; %llu words, %llu pixel/guard differences, %llu state differences\n",
        cases-failed,cases,(unsigned long long)checked,(unsigned long long)differences,(unsigned long long)state_differences);
    return failed?1:0;
}
