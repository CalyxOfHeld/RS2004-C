/* Bundled revision-225 Pix3D.getTexels value preparation. */
#include "gfx/pix3d_texture_cache225.h"
#include "gfx/pix3d.h"
#include "gfx/pix3d_texture225.h"
#include "core/jrandom.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t *indices;
    uint32_t *palette;
    uint32_t *adjusted;
    uint32_t *texels;
    int width,height,palette_count;
    bool translucent;
    uint64_t last_access;
} Texture225;
static Texture225 entries[50];
static bool cache_low_detail;
static uint64_t generation;
static uint64_t access_cycle;
uint64_t pix3d225_texture_cache_generation(void){return generation;}

double pix3d225_effective_brightness(double requested) {
    return requested+(math_random()*0.03-0.015);
}
double pix3d225_preview_brightness(double requested) {
    JRandom snapshot={math_random_state()};
    return requested+(jrandom_next_double(&snapshot)*0.03-0.015);
}

void pix3d225_texture_cache_clear(void) {
    access_cycle=0;
    generation++;
    for(int id=0;id<50;id++) {
        Texture225 *t=&entries[id];
        free(t->indices);free(t->palette);free(t->adjusted);free(t->texels);
        memset(t,0,sizeof(*t));
    }
}
void pix3d225_texture_cache_init(bool low_detail) {
    pix3d225_texture_cache_clear();
    cache_low_detail=low_detail;
}
void pix3d225_texture_cache_set_image(int id,const Pix8 *image) {
    if(id<0||id>=50||!image||image->width<=0||image->height<=0||image->palette_count<=0) return;
    generation++;
    Texture225 *t=&entries[id];
    free(t->indices);free(t->palette);free(t->adjusted);free(t->texels);
    memset(t,0,sizeof(*t));
    size_t count=(size_t)image->width*image->height;
    t->indices=malloc(count);
    t->palette=malloc((size_t)image->palette_count*sizeof(uint32_t));
    if(!t->indices||!t->palette) {
        free(t->indices);free(t->palette);memset(t,0,sizeof(*t));return;
    }
    memcpy(t->indices,image->pixels,count);
    memcpy(t->palette,image->palette,(size_t)image->palette_count*sizeof(uint32_t));
    t->width=image->width;t->height=image->height;t->palette_count=image->palette_count;
}
void pix3d225_texture_cache_set_brightness(double effective_brightness) {
    generation++;
    for(int id=0;id<50;id++) {
        Texture225 *t=&entries[id];
        free(t->adjusted);free(t->texels);
        t->adjusted=NULL;t->texels=NULL;t->translucent=false;
        if(!t->palette) continue;
        t->adjusted=malloc((size_t)t->palette_count*sizeof(uint32_t));
        if(!t->adjusted) continue;
        for(int i=0;i<t->palette_count;i++)
            t->adjusted[i]=pix3d_gamma_correct(t->palette[i],effective_brightness);
    }
}
const uint32_t *pix3d225_texture_cache_get(int id,bool *opaque) {
    if(id<0||id>=50) return NULL;
    Texture225 *t=&entries[id];
    t->last_access=access_cycle++;
    if(!t->indices||!t->adjusted) return NULL;
    if(!t->texels) {
        if(cache_low_detail) {
            if(t->width!=64||t->height!=64) return NULL;
        } else if (!((t->width==64||t->width==128)&&t->height==t->width)) return NULL;
        size_t plane=cache_low_detail?4096u:16384u;
        t->texels=malloc(plane*4u*sizeof(uint32_t));
        if(!t->texels) return NULL;
        if(cache_low_detail) {
            for(size_t i=0;i<plane;i++) {
                uint32_t value=t->adjusted[t->indices[i]]&0xF8F8FFu;
                t->texels[i]=value;
                if(!value) t->translucent=true;
                t->texels[i+plane]=(value-(value>>3))&0xF8F8FFu;
                t->texels[i+2*plane]=(value-(value>>2))&0xF8F8FFu;
                t->texels[i+3*plane]=(value-(value>>2)-(value>>3))&0xF8F8FFu;
            }
        } else {
            if(t->width==64&&t->height==64) {
                for(int y=0;y<128;y++) for(int x=0;x<128;x++)
                    t->texels[x+(y<<7)]=t->adjusted[t->indices[(x>>1)+((y>>1)<<6)]];
            } else if(t->width==128&&t->height==128) {
                for(size_t i=0;i<plane;i++) t->texels[i]=t->adjusted[t->indices[i]];
            }
            for(size_t i=0;i<plane;i++) {
                uint32_t value=t->texels[i]&0xF8F8FFu;
                t->texels[i]=value;
                if(!value) t->translucent=true;
                t->texels[i+plane]=(value-(value>>3))&0xF8F8FFu;
                t->texels[i+2*plane]=(value-(value>>2))&0xF8F8FFu;
                t->texels[i+3*plane]=(value-(value>>2)-(value>>3))&0xF8F8FFu;
            }
        }
    }
    if(opaque) *opaque=!t->translucent;
    return t->texels;
}

void pix3d225_texture_cache_triangle(int id,const int32_t args[18]) {
    bool opaque;
    const uint32_t *texels=pix3d225_texture_cache_get(id,&opaque);
    if(texels) pix3d225_texture_triangle(texels,opaque,args);
}

uint64_t pix3d225_texture_cache_cycle(void){return access_cycle;}
bool pix3d225_texture_cache_animate_used(uint64_t since,int elapsed) {
    if(cache_low_detail)return true;
    static uint8_t scratch[16384];const int animated[]={17,24};
    for(int j=0;j<2;j++) {
        Texture225 *t=&entries[animated[j]];if(t->last_access<since || !t->indices)continue;
        size_t count=(size_t)t->width*t->height;
        if(!count || count>sizeof(scratch) || (count&(count-1)))return false;
        size_t shift=((size_t)t->width*(unsigned)elapsed*2)&(count-1);
        for(size_t i=0;i<count;i++)scratch[i]=t->indices[(i-shift)&(count-1)];
        memcpy(t->indices,scratch,count);free(t->texels);t->texels=NULL;t->translucent=false;
    }
    return true;
}