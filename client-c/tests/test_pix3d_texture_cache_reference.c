#include "core/jagfile.h"
#include "core/jrandom.h"
#include "gfx/pix8.h"
#include "gfx/pix3d_texture_cache225.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_i32(FILE *f,int32_t *v) {
    unsigned char b[4]; if(fread(b,1,4,f)!=4) return 0;
    *v=(int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);
    return 1;
}
/* client.updateTextures: only used textures 17/24 rotate, and lowMemory returns.
 * Expected index movement follows the original loop, then the already verified
 * cache preparation converts a separate image to comparable texels. */
static long animation_controls(Jagfile *archive) {
    long differences=0,words=0;
    const int ids[]={17,24},steps[]={0,1,3,65};
    for(int detail=0;detail<2;detail++) for(int n=0;n<2;n++) {
        char name[8];snprintf(name,sizeof(name),"%d",ids[n]);
        Pix8 *image=pix8_load(archive,name,0,NULL);
        if(!image)return 1;
        if(detail==0&&image->crop_w==128)pix8_shrink(image,NULL);
        else pix8_crop(image,NULL);
        pix3d225_texture_cache_init(detail==0);
        pix3d225_texture_cache_set_image(ids[n],image);
        pix3d225_texture_cache_set_brightness(0.8);
        bool opaque=false;
        const uint32_t *initial=pix3d225_texture_cache_get(ids[n],&opaque);
        size_t count=(size_t)image->width*image->height;
        size_t texel_count=(detail==0?4096u:16384u)*4u;
        uint32_t *snapshot=malloc(texel_count*sizeof(*snapshot));
        uint8_t *scratch=malloc(count);
        if(!initial||!snapshot||!scratch)return 1;
        memcpy(snapshot,initial,texel_count*sizeof(*snapshot));
        uint64_t since=pix3d225_texture_cache_cycle();
        if(!pix3d225_texture_cache_animate_used(since,3))return 1;
        const uint32_t *unused=pix3d225_texture_cache_get(ids[n],&opaque);
        for(size_t i=0;i<texel_count;i++)if(unused[i]!=snapshot[i])differences++;
        words+=(long)texel_count;
        for(int step=0;step<4;step++) {
            since=pix3d225_texture_cache_cycle();
            if(!pix3d225_texture_cache_get(ids[n],&opaque))return 1;
            if(!pix3d225_texture_cache_animate_used(since,steps[step]))return 1;
            if(detail!=0) {
                int mask=(int)count-1,shift=image->width*steps[step]*2;
                for(int i=0;i<=mask;i++)scratch[i]=image->pixels[(i-shift)&mask];
                memcpy(image->pixels,scratch,count);
            }
            pix3d225_texture_cache_set_image(0,image);
            pix3d225_texture_cache_set_brightness(0.8);
            const uint32_t *expected=pix3d225_texture_cache_get(0,&opaque);
            const uint32_t *actual=pix3d225_texture_cache_get(ids[n],&opaque);
            if(!actual||!expected)return 1;
            for(size_t i=0;i<texel_count;i++)if(actual[i]!=expected[i])differences++;
            words+=(long)texel_count;
        }
        free(snapshot);free(scratch);pix8_free(image);
        pix3d225_texture_cache_clear();
    }
    printf("Texture animation source controls: textures17/24, both detail modes, unused and delta0/1/3/65; %ld words, %ld mismatches\n",words,differences);
    return differences;
}

int main(int argc,char **argv) {
    if(argc!=3) { fprintf(stderr,"usage: texture-cache-reference archive fixture\n"); return 2; }
    FILE *f=fopen(argv[2],"rb"); if(!f) return 2;
    int32_t magic; if(!read_i32(f,&magic)||magic!=0x50335431) return 2;
    Jagfile *archive=jagfile_load_file(argv[1],NULL); if(!archive) return 2;
    long words=0,differences=0,records=0;
    for(int detail=0;detail<2;detail++) {
        int32_t found,count;
        if(!read_i32(f,&found)||found!=detail||!read_i32(f,&count)) return 2;
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
        JRandom random;jrandom_set_seed(&random,225+detail);
        double effective=0.8+(jrandom_next_double(&random)*0.03-0.015);
        pix3d225_texture_cache_set_brightness(effective);
        for(int n=0;n<count;n++) {
            int32_t id,opaque,texel_count,case_count;
            if(!read_i32(f,&id)||!read_i32(f,&opaque)||!read_i32(f,&texel_count)) return 2;
            bool native_opaque=false;
            const uint32_t *texels=pix3d225_texture_cache_get(id,&native_opaque);
            if(!texels) { fprintf(stderr,"cache missed texture %d detail %d\n",id,detail);return 1; }
            if(native_opaque!=(opaque!=0)) differences++;
            for(int i=0;i<texel_count;i++) {
                int32_t expected; if(!read_i32(f,&expected)) return 2;
                if(texels[i]!=(uint32_t)expected) {
                    if(differences<10) fprintf(stderr,"detail=%d id=%d texel=%d got=%08X expected=%08X\n",
                        detail,id,i,(unsigned)texels[i],(unsigned)expected);
                    differences++;
                }
            }
            if(!read_i32(f,&case_count)) return 2;
            for(int i=0;i<case_count;i++) {
                int32_t params[15],length;
                for(int j=0;j<15;j++) if(!read_i32(f,&params[j])) return 2;
                if(!read_i32(f,&length)) return 2;
                if(fseek(f,(long)length*4,SEEK_CUR)!=0) return 2;
            }
            records++;words+=texel_count;
        }
        pix3d225_texture_cache_clear();
    }
    fclose(f);
    printf("Pix3D texture cache: %ld real-cache records, %ld texel words, %ld mismatches\n",records,words,differences);
    differences+=animation_controls(archive);
    return differences?1:0;
}
