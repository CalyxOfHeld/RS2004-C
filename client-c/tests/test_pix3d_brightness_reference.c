#include "core/jagfile.h"
#include "core/jrandom.h"
#include "gfx/pix8.h"
#include "gfx/pix3d.h"
#include "gfx/pix3d_texture_cache225.h"
#include <stdio.h>
#include <stdlib.h>

static int read_i32(FILE *f,int32_t *v) {
    unsigned char b[4]; if(fread(b,1,4,f)!=4) return 0;
    *v=(int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);return 1;
}
static int read_double(FILE *f,double *v) {
    unsigned char b[8];if(fread(b,1,8,f)!=8) return 0;
    union {uint64_t u;double d;} value={0};
    for(int i=0;i<8;i++) value.u=(value.u<<8)|b[i];
    *v=value.d;return 1;
}
int main(int argc,char **argv) {
    if(argc!=3) {fprintf(stderr,"usage: brightness-reference archive fixture\n");return 2;}
    FILE *f=fopen(argv[2],"rb");if(!f) return 2;
    int32_t magic,id;if(!read_i32(f,&magic)||magic!=0x50334231||!read_i32(f,&id)||id!=17) return 2;
    Jagfile *archive=jagfile_load_file(argv[1],NULL);if(!archive) return 2;
    Pix8 *image=pix8_load(archive,"17",0,NULL);if(!image) return 2;
    pix8_shrink(image,NULL);
    pix3d225_texture_cache_init(true);
    pix3d225_texture_cache_set_image(id,image);
    math_random_seed(225);
    long differences=0,words=0;
    uint64_t initial_state=math_random_state();
    double preview=pix3d225_preview_brightness(0.8);
    if(math_random_state()!=initial_state) differences++;
    for(int phase=0;phase<2;phase++) {
        double recorded,effective=pix3d225_effective_brightness(phase==0?0.8:0.6);
        if(!read_double(f,&recorded)) return 2;
        if(recorded!=effective) differences++;
        if(phase==0&&preview!=effective) differences++;
        pix3d_init_colour_table(effective);
        pix3d225_texture_cache_set_brightness(effective);
        if(phase==0) pix3d_set_texture_palette_gamma(id,image->palette,image->palette_count,effective);
        for(int i=0;i<65536;i++) {
            int32_t expected;if(!read_i32(f,&expected)) return 2;
            if(pix3d_colour_table[i]!=expected) differences++;
        }
        int32_t length;if(!read_i32(f,&length)||length!=16384) return 2;
        bool opaque=false;
        const uint32_t *texels=pix3d225_texture_cache_get(id,&opaque);
        if(!texels) return 2;
        for(int i=0;i<length;i++) {
            int32_t expected;if(!read_i32(f,&expected)) return 2;
            if(texels[i]!=(uint32_t)expected) differences++;
        }
        int32_t average,translucent;
        if(!read_i32(f,&average)||!read_i32(f,&translucent)) return 2;
        if(pix3d_texture_average(id)!=average||opaque==(translucent!=0)) differences++;
        words+=65536+length;
    }
    pix3d225_texture_cache_clear();pix8_free(image);fclose(f);
    printf("Pix3D brightness lifecycle: 2 states, %ld palette/texel words, %ld mismatches\n",words,differences);
    return differences?1:0;
}
