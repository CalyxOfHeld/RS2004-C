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
    unsigned char b[4];if(fread(b,1,4,f)!=4){fputs("truncated picking fixture\n",stderr);exit(2);}
    return (uint32_t)b[0]<<24 | (uint32_t)b[1]<<16 | (uint32_t)b[2]<<8 | b[3];
}
static int32_t integer(FILE *f) {uint32_t v=word(f);return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static uint64_t wide(FILE *f) {uint64_t v=(uint64_t)word(f)<<32;return v|word(f);}
static uint64_t frame_hash(const uint32_t *pixels) {
    uint64_t h=UINT64_C(0xcbf29ce484222325);
    for(int j=0;j<128*128+8;j++){h^=pixels[j];h*=UINT64_C(0x100000001b3);}
    return h;
}
int main(int argc,char **argv) {
    if(argc!=4){fputs("usage: test_model_picking_reference models-archive fixture textures-archive\n",stderr);return 2;}
    FILE *f=fopen(argv[2],"rb");if(!f){perror(argv[2]);return 2;}
    if(word(f)!=0x4d504b32){fputs("expected MPK2 fixture\n",stderr);return 2;}
    union {uint64_t u;double d;} brightness;brightness.u=wide(f);
    if(!(brightness.d>0.7 && brightness.d<0.9))return 2;
    pix3d_init();pix3d_init_colour_table(brightness.d);pix3d_low_detail=true;pix3d_jagged=true;
    Arena arena=arena_create(32*1024*1024);
    Jagfile *models=jagfile_load_file(argv[1],&arena),*textures=jagfile_load_file(argv[3],&arena);
    if(!models || !textures)return 2;
    model_init_repo(models,&arena);
    pix3d225_texture_cache_init(true);
    for(int id=0;id<50;id++) {
        char name[8];snprintf(name,sizeof(name),"%d",id);Pix8 *image=pix8_load(textures,name,0,NULL);if(!image)return 2;
        if(image->crop_w==128)pix8_shrink(image,NULL);else pix8_crop(image,NULL);
        pix3d225_texture_cache_set_image(id,image);pix8_free(image);
    }
    pix3d225_texture_cache_set_brightness(brightness.d);
    uint32_t cases=word(f),failures=0,pick_differences=0,frame_differences=0,state_differences=0,hits=0;
    if(!cases || cases>100000)return 2;
    int cached_id=-1;Model *m=NULL;
    for(uint32_t n=0;n<cases;n++) {
        int32_t c[14];for(int j=0;j<14;j++)c[j]=integer(f);
        int32_t vertices=integer(f),faces=integer(f),alpha=integer(f),clip=integer(f),expected_count=integer(f);
        uint32_t expected_list[3];for(int j=0;j<3;j++)expected_list[j]=word(f);
        uint64_t expected_hash=wide(f);
        if(c[0]<0 || c[7]<0 || c[7]>256 || expected_count<1 || expected_count>2)return 2;
        if(cached_id!=c[0]) {
            m=model_load(c[0],&arena);if(!m)return 2;cached_id=c[0];model_calculate_normals(m,64,768,-50,-10,-50);
        }
        if(m->num_points!=vertices || m->num_faces!=faces){fputs("picking model metadata differs\n",stderr);return 1;}
        uint32_t storage[128*128+16],*pixels=storage+8;
        for(int j=0;j<128*128+16;j++)storage[j]=0x19372b;
        pix2d_bind(pixels,128,128);pix3d_init_2d();pix3d_set_trans(c[7]);pix3d_set_hclip(c[8]!=0);
        ModelPick225 pick={c[9]!=0,c[10]!=0,c[11],c[12],c[13],true};
        if(!model_render_world_pick225(m,c[1],c[2],c[3],c[4],c[5],c[6],&pick)) {
            fprintf(stderr,"picking render rejected case %u\n",n);return 2;
        }
        int count=1;uint32_t list[3]={0x11223344,0x55667788,0x76543210};
        if(pick.picked){list[count++]=(uint32_t)pick.bitset;hits++;}
        bool bad_pick=count!=expected_count;
        for(int j=0;j<3;j++)if(list[j]!=expected_list[j])bad_pick=true;
        bool bad_frame=frame_hash(pixels)!=expected_hash;
        bool bad_state=pix3d_trans!=alpha || pix3d_hclip!=(clip!=0);
        for(int j=0;j<8;j++)if(storage[j]!=0x19372b){fputs("picking prefix guard changed\n",stderr);return 1;}
        if(bad_pick || bad_frame || bad_state) {
            if(failures<8)printf("case %u model=%d mouse=%d,%d coarse=%d gate=%d/%d: pick=%d/%d frame=%d state=%d\n",
                n,c[0],c[11],c[12],c[10],c[9],c[13],count,expected_count,bad_frame,bad_state);
            failures++;
        }
        pick_differences+=bad_pick;frame_differences+=bad_frame;state_differences+=bad_state;
    }
    uint32_t predicates=word(f),predicate_failures=0;if(predicates>100000)return 2;
    for(uint32_t n=0;n<predicates;n++) {
        int32_t c[8];for(int j=0;j<8;j++)c[j]=integer(f);bool expected=word(f)!=0;
        if(model_point_within_triangle225(c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7])!=expected)predicate_failures++;
    }
    if(fgetc(f)!=EOF){fputs("trailing picking fixture data\n",stderr);return 2;}
    fclose(f);free(pix3d_scanline);pix3d_scanline=NULL;pix3d225_texture_cache_clear();arena_free(&arena);
    printf("Model picking reference: %u/%u match; %u hits; picks=%u frame hashes=%u states=%u differences\n",
        cases-failures,cases,hits,pick_differences,frame_differences,state_differences);
    printf("Original face bounds: %u/%u match\n",predicates-predicate_failures,predicates);
    return failures || predicate_failures?1:0;
}
