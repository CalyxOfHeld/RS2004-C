/* Diagnostic baseline, deliberately NOT a member of make test: full scene
 * parity is still failing. Never treat successful comparison setup as parity. */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "core/arena.h"
#include "core/jagfile.h"
#include "core/jrandom.h"
#include "config/flo.h"
#include "config/loc.h"
#include "config/seq.h"
#include "gfx/anim.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/pix8.h"
#include "dash3d/world.h"
#include "dash3d/location_anim.h"
#include <stdio.h>
#include <stdlib.h>

static uint32_t word(FILE *f) {
    unsigned char b[4];if(fread(b,1,4,f)!=4){fputs("truncated scene fixture\n",stderr);exit(2);}
    return (uint32_t)b[0]<<24 | (uint32_t)b[1]<<16 | (uint32_t)b[2]<<8 | b[3];
}
static int32_t integer(FILE *f){uint32_t v=word(f);return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static uint8_t *read_map(const char *dir,const char *name,size_t *length) {
    char path[1024];snprintf(path,sizeof(path),"%s/%s",dir,name);FILE *f=fopen(path,"rb");if(!f){perror(path);return NULL;}
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return NULL;}long len=ftell(f);rewind(f);if(len<=0){fclose(f);return NULL;}
    uint8_t *data=malloc((size_t)len);if(!data){fclose(f);return NULL;}
    if(fread(data,1,(size_t)len,f)!=(size_t)len){free(data);fclose(f);return NULL;}
    fclose(f);*length=(size_t)len;return data;
}
static Jagfile *jag(const char *dir,const char *name,Arena *arena) {
    char path[1024];snprintf(path,sizeof(path),"%s/%s",dir,name);return jagfile_load_file(path,arena);
}
int main(int argc,char **argv) {
    if(argc!=4 && argc!=5){fputs("usage: test_scene_reference cache-directory map-directory fixture [capture-directory]\n",stderr);return 2;}
    FILE *f=fopen(argv[3],"rb");if(!f){perror(argv[3]);return 2;}
    if(word(f)!=0x53445731){fputs("expected SDW1 fixture\n",stderr);return 2;}
    int width=integer(f),height=integer(f);uint32_t cases=word(f);
    if(width!=512 || height!=334 || !cases || cases>1000)return 2;
    union {uint64_t u;double d;} brightness;brightness.u=(uint64_t)word(f)<<32;brightness.u|=word(f);
    if(!(brightness.d>0.7 && brightness.d<0.9))return 2;
    int hue=integer(f),light=integer(f);printf("Original build hue/light offsets %d/%d; native build retains its existing neutral offsets\n",hue,light);
    pix3d_init();pix3d_init_colour_table(brightness.d);
    Arena arena=arena_create(128*1024*1024);
    Jagfile *models=jag(argv[1],"models",&arena),*cfg=jag(argv[1],"config",&arena),*textures=jag(argv[1],"textures",&arena);
    if(!models || !cfg || !textures)return 2;
    model_init_repo(models,&arena);if(!anim_unpack(models,&arena))return 2;
    LocConfig locs;loc_unpack(&locs,cfg);SeqConfig sequences;seq_unpack(&sequences,cfg);
    math_random_seed(225);FloConfig floors;flo_unpack(&floors,cfg);
    World *world=calloc(1,sizeof(*world));World3D *scene=calloc(1,sizeof(*scene));if(!world || !scene)return 2;
    world_init(world,104,104);world3d_init(scene,104,104,4);scene->sequences=&sequences;
    size_t ground_len,loc_len;uint8_t *ground=read_map(argv[2],"m50_50.raw",&ground_len),*locations=read_map(argv[2],"l50_50.raw",&loc_len);
    if(!ground || !locations)return 2;
    world_load_ground(world,ground,ground_len,20,20,3180,3180);math_random_seed(225);
    world_load_locations(world,locations,loc_len,&locs,scene,NULL,20,20);free(ground);free(locations);
    // Both texture detail modes must be prepared before their scene draws.
    // Build the map under the first mode, just as the original fixture does.
    int detail=-1;uint32_t failures=0;uint64_t differences=0,checked=0;
    uint32_t *storage=malloc(((size_t)width*height+16)*sizeof(*storage));if(!storage)return 2;uint32_t *pixels=storage+8;
    for(uint32_t n=0;n<cases;n++) {
        int32_t c[8];for(int j=0;j<8;j++)c[j]=integer(f);
        int alpha=integer(f),clip=integer(f);uint32_t count=word(f);
        if(count!=(uint32_t)(width*height+8) || c[6]<0 || c[6]>1)return 2;
        if(detail!=c[6]) {
            detail=c[6];pix3d_low_detail=detail!=0;
            for(int id=0;id<50;id++) {
                char name[8];snprintf(name,sizeof(name),"%d",id);Pix8 *image=pix8_load(textures,name,0,NULL);if(!image)return 2;
                if(pix3d_low_detail && image->crop_w==128)pix8_shrink(image,NULL);else pix8_crop(image,NULL);
                pix3d_set_texture_palette_gamma(id,image->palette,image->palette_count,brightness.d);
                size_t total=(size_t)image->width*image->height;uint32_t *rgb=malloc(total*sizeof(*rgb));if(!rgb)return 2;
                for(size_t j=0;j<total;j++){rgb[j]=image->pixels[j]?pix3d_gamma_correct(image->palette[image->pixels[j]],brightness.d):0;if(image->pixels[j] && !rgb[j])rgb[j]=1;}
                pix3d_set_texture(id,rgb,image->width,image->height);free(rgb);pix8_free(image);
            }
        }
        if(n==0){world_build(world,scene,&floors,NULL);location_anim_tick(scene,&locs,0);}
        for(uint32_t j=0;j<count+8;j++)storage[j]=0x19372b;
        pix2d_bind(pixels,width,height);pix3d_init_2d();pix3d_set_trans(0);pix3d_set_hclip(false);
        world3d_render(scene,c[0],c[1],c[2],c[3],c[4],0,c[5]);pix3d_end_scene();
        uint64_t different=0,guards=0;for(uint32_t j=0;j<count;j++) {
            if(pixels[j]!=word(f)){different++;if(j>=(uint32_t)(width*height))guards++;}
        }
        for(int j=0;j<8;j++)if(storage[j]!=0x19372b){fputs("scene prefix guard changed\n",stderr);return 1;}
        bool state=pix3d_trans!=alpha || pix3d_hclip!=(clip!=0);
        printf("scene %u: %llu/%u differing words, %llu guard differences, raster state differs=%d; camera=%d,%d,%d pitch=%d yaw=%d plane=%d texture-detail=%d original-lowMemory=%d\n",
            n,(unsigned long long)different,count,(unsigned long long)guards,state,c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7]);
        if(argc==5) {
            char path[1024];snprintf(path,sizeof(path),"%s/frame-%u.bmp",argv[4],n);
            SDL_Surface *surface=SDL_CreateRGBSurfaceFrom(pixels,width,height,32,width*4,0xff0000,0xff00,0xff,0);
            if(!surface || SDL_SaveBMP(surface,path)!=0){fprintf(stderr,"capture failed: %s\n",SDL_GetError());return 2;}SDL_FreeSurface(surface);
        }
        checked+=count;differences+=different;failures+=different!=0 || state;
    }
    if(fgetc(f)!=EOF){fputs("trailing scene fixture data\n",stderr);return 2;}
    fclose(f);free(storage);world3d_dispose(scene);free(scene);free(world);flo_free(&floors);loc_free(&locs);seq_free(&sequences);arena_free(&arena);
    printf("Static scene reference: %u/%u match; %llu words checked, %llu differences\n",cases-failures,cases,(unsigned long long)checked,(unsigned long long)differences);
    return failures?1:0;
}
