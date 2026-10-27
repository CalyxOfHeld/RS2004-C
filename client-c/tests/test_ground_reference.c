#include "dash3d/ground225.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/pix3d_gouraud225.h"
#include "gfx/pix3d_texture_cache225.h"
#include "gfx/pix8.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {int kind,x,z;int32_t heights[4];TileUnderlay u;TileOverlay o;} TileCase;
static uint32_t word(FILE *f){
    unsigned char b[4];if(fread(b,1,4,f)!=4){fputs("truncated ground fixture\n",stderr);exit(2);}
    return (uint32_t)b[0]<<24 | (uint32_t)b[1]<<16 | (uint32_t)b[2]<<8 | b[3];
}
static int32_t integer(FILE *f){uint32_t v=word(f);return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
int main(int argc,char **argv){
    if(argc!=3){fputs("usage: test_ground_reference textures-archive fixture\n",stderr);return 2;}
    FILE *f=fopen(argv[2],"rb");if(!f){perror(argv[2]);return 2;}
    if(word(f)!=0x47323235){fputs("expected G225 fixture\n",stderr);return 2;}
    union{uint64_t u;double d;} brightness;brightness.u=(uint64_t)word(f)<<32;brightness.u|=word(f);
    if(!(brightness.d>0.7 && brightness.d<0.9))return 2;
    uint32_t tile_count=word(f);if(!tile_count || tile_count>10000)return 2;
    TileCase *tiles=calloc(tile_count,sizeof(*tiles));if(!tiles)return 2;
    for(uint32_t n=0;n<tile_count;n++){
        TileCase *t=&tiles[n];t->kind=integer(f);t->x=integer(f);t->z=integer(f);for(int j=0;j<4;j++)t->heights[j]=integer(f);
        if(t->kind==0){
            t->u.southwest_color=integer(f);t->u.southeast_color=integer(f);t->u.northeast_color=integer(f);t->u.northwest_color=integer(f);
            t->u.texture_id=integer(f);t->u.rgb=integer(f);t->u.flat=word(f)!=0;
        }else if(t->kind==1){
            t->o.flat=word(f)!=0;t->o.shape=integer(f);t->o.rotation=integer(f);t->o.background_rgb=integer(f);t->o.foreground_rgb=integer(f);
            t->o.triangle_texture_id=word(f)?0:-1;t->o.vertex_count=integer(f);t->o.triangle_count=integer(f);
            if(t->o.vertex_count<0 || t->o.vertex_count>6 || t->o.triangle_count<0 || t->o.triangle_count>10)return 2;
            for(int j=0;j<t->o.vertex_count;j++){t->o.vertex_x[j]=integer(f);t->o.vertex_y[j]=integer(f);t->o.vertex_z[j]=integer(f);}
            for(int j=0;j<t->o.triangle_count;j++){
                t->o.triangle_vertex_a[j]=integer(f);t->o.triangle_vertex_b[j]=integer(f);t->o.triangle_vertex_c[j]=integer(f);
                t->o.triangle_color_a[j]=integer(f);t->o.triangle_color_b[j]=integer(f);t->o.triangle_color_c[j]=integer(f);t->o.triangle_texture[j]=integer(f);
            }
        }else return 2;
    }
    pix3d_init();pix3d_init_colour_table(brightness.d);
    Arena arena=arena_create(8*1024*1024);Jagfile *textures=jagfile_load_file(argv[1],&arena);if(!textures)return 2;
    uint32_t cases=word(f),failed=0,state_differences=0;uint64_t checked=0,differences=0;
    if(!cases || cases>100000)return 2;
    int detail=-1;
    for(uint32_t n=0;n<cases;n++){
        int32_t c[17];for(int j=0;j<17;j++)c[j]=integer(f);
        int alpha=integer(f),clip=integer(f),click_x=integer(f),click_z=integer(f);uint32_t count=word(f);
        if(c[0]<0 || (uint32_t)c[0]>=tile_count || count!=64*64+8 || c[7]<0 || c[7]>1)return 2;
        if(detail!=c[7]){
            detail=c[7];pix3d_low_detail=detail!=0;pix3d225_texture_cache_init(pix3d_low_detail);
            for(int id=0;id<50;id++){
                char name[8];snprintf(name,sizeof(name),"%d",id);Pix8 *image=pix8_load(textures,name,0,NULL);if(!image)return 2;
                if(pix3d_low_detail && image->crop_w==128)pix8_shrink(image,NULL);else pix8_crop(image,NULL);
                pix3d225_texture_cache_set_image(id,image);pix8_free(image);
            }
            pix3d225_texture_cache_set_brightness(brightness.d);
        }
        uint32_t storage[64*64+16],*pixels=storage+8;for(uint32_t j=0;j<count+8;j++)storage[j]=0x19372b;
        pix2d_bind(pixels,64,64);pix3d_init_2d();pix3d_jagged=c[8]!=0;pix3d_set_trans(c[13]);pix3d_set_hclip(c[14]!=0);
        Ground225 view={c[1],c[2],c[3],c[4],c[5],c[6]!=0,c[10]!=0,c[11],c[12],c[15],c[16]};TileCase *t=&tiles[c[0]];
        bool accepted=t->kind==0?world3d_underlay225(&t->u,t->heights,t->x,t->z,&view):world3d_overlay225(&t->o,t->x,t->z,c[9]!=0,&view);
        if(!accepted){fprintf(stderr,"ground render rejected case %u tile=%d\n",n,c[0]);return 2;}
        uint32_t bad=0;for(uint32_t j=0;j<count;j++){
            uint32_t expected=word(f);if(pixels[j]!=expected){if(!bad && failed<8)printf("case %u tile=%d kind=%d first=%u C=%08x Java=%08x\n",n,c[0],t->kind,j,pixels[j],expected);bad++;}
        }
        for(int j=0;j<8;j++)if(storage[j]!=0x19372b){fputs("ground prefix guard changed\n",stderr);return 1;}
        bool state=pix3d_trans!=alpha || pix3d_hclip!=(clip!=0) || view.click_tile_x!=click_x || view.click_tile_z!=click_z;
        if(state && failed<8)printf("case %u state C=%d/%d/%d,%d Java=%d/%d/%d,%d\n",n,pix3d_trans,pix3d_hclip,view.click_tile_x,view.click_tile_z,alpha,clip,click_x,click_z);
        if(bad || state){failed++;if(failed<=8)printf("case %u: %u differing words\n",n,bad);}
        differences+=bad;checked+=count;state_differences+=state;
    }
    uint32_t table=word(f),bad_table=0;if(table!=50)return 2;
    for(uint32_t n=0;n<table;n++)if(integer(f)!=world3d_texture_hsl225[n])bad_table++;
    uint32_t lights=word(f),bad_lights=0;if(lights>100000)return 2;
    for(uint32_t n=0;n<lights;n++){int32_t shade=integer(f),hsl=integer(f),expected=integer(f);if(world3d_ground_lightness225(shade,hsl)!=expected)bad_lights++;}
    uint32_t picks=word(f),bad_picks=0;if(picks>100000)return 2;
    for(uint32_t n=0;n<picks;n++){int32_t c[8];for(int j=0;j<8;j++)c[j]=integer(f);bool expected=word(f)!=0;if(world3d_point_inside_triangle225(c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7])!=expected)bad_picks++;}
    if(fgetc(f)!=EOF){fputs("trailing ground fixture data\n",stderr);return 2;}
    fclose(f);free(tiles);free(pix3d_scanline);pix3d_scanline=NULL;pix3d225_texture_cache_clear();arena_free(&arena);
    printf("Ground reference: %u/%u match; %llu words, %llu pixel/guard differences, %u state differences\n",cases-failed,cases,(unsigned long long)checked,(unsigned long long)differences,state_differences);
    printf("Ground helpers: texture table %u/%u, lightness %u/%u, picking %u/%u match\n",table-bad_table,table,lights-bad_lights,lights,picks-bad_picks,picks);
    return failed || bad_table || bad_lights || bad_picks?1:0;
}
