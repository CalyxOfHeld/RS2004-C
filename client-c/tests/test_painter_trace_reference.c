#include "dash3d/ground225.h"
#include "dash3d/scene_raster225.h"
#include "gfx/model.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/pix3d_gouraud225.h"
#include "gfx/pix3d_texture_cache225.h"
#include "gfx/pix8.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include <stdio.h>
#include <stdlib.h>

typedef SceneModel225 Asset;
static uint32_t word(FILE *f){unsigned char b[4];if(fread(b,1,4,f)!=4){fputs("truncated painter trace\n",stderr);exit(2);}return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static int32_t integer(FILE *f){uint32_t v=word(f);return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static int32_t *array(FILE *f,int count){
    int length=integer(f);if(length==-1)return NULL;if(length!=count){fputs("invalid painter array length\n",stderr);exit(2);}
    int32_t *out=calloc(count?count:1,sizeof(*out));if(!out)exit(2);for(int i=0;i<count;i++)out[i]=integer(f);return out;
}
static Asset asset(FILE *f){
    Asset a={0};a.model=calloc(1,sizeof(*a.model));if(!a.model)exit(2);Model *m=a.model;
    m->num_points=integer(f);m->num_faces=integer(f);m->num_t=integer(f);m->priority=integer(f);
    a.bounds.radius=integer(f);a.bounds.max_y=integer(f);(void)integer(f);a.bounds.min_depth=integer(f);a.bounds.max_depth=integer(f);a.pickable=integer(f)!=0;
    if(m->num_points<0 || m->num_points>4096 || m->num_faces<0 || m->num_faces>4096 || m->num_t<0 || m->num_t>4096)exit(2);
    m->point_x=array(f,m->num_points);m->point_y=array(f,m->num_points);m->point_z=array(f,m->num_points);
    m->face_vertex_a=array(f,m->num_faces);m->face_vertex_b=array(f,m->num_faces);m->face_vertex_c=array(f,m->num_faces);
    m->face_colour=array(f,m->num_faces);m->face_colour_a=array(f,m->num_faces);m->face_colour_b=array(f,m->num_faces);m->face_colour_c=array(f,m->num_faces);
    m->face_render_type=array(f,m->num_faces);m->face_priority=array(f,m->num_faces);m->face_alpha=array(f,m->num_faces);
    m->texture_a=array(f,m->num_t);m->texture_b=array(f,m->num_t);m->texture_c=array(f,m->num_t);
    /* Java discards base colors after untextured lighting. Native's generic
     * model contract needs storage, though those faces never read it. */
    if(!m->face_colour){
        for(int i=0;i<m->num_faces;i++)if(m->face_render_type && m->face_render_type[i]!=-1 && (m->face_render_type[i]&3)>=2)exit(2);
        m->face_colour=calloc(m->num_faces?m->num_faces:1,sizeof(*m->face_colour));if(!m->face_colour)exit(2);
    }
    return a;
}
#ifdef PAINTER_TRACE_LIBRARY
/* Reuse the accepted snapshot decoder and cache setup for generated traversal.
 * These helpers expose fixture inputs only; no expected command drives a draw. */
typedef struct {Arena arena;Jagfile *textures;double brightness;int detail;uint64_t generation;} TraceFixture225;
void *painter_trace_open(const char *path,double brightness){
    TraceFixture225 *t=calloc(1,sizeof(*t));if(!t)return NULL;
    pix3d_init();pix3d_init_colour_table(brightness);
    t->arena=arena_create(8*1024*1024);t->textures=jagfile_load_file(path,&t->arena);
    if(!t->textures){arena_free(&t->arena);free(t);return NULL;}
    t->brightness=brightness;t->detail=-1;return t;
}
Asset *painter_trace_load_asset(const char *path,int32_t offset){
    FILE *f=fopen(path,"rb");if(!f)return NULL;
    if(offset<0 || fseek(f,offset,SEEK_SET)){fclose(f);return NULL;}
    Asset *a=malloc(sizeof(*a));if(a)*a=asset(f);fclose(f);return a;
}
void painter_trace_set_brightness(void *fixture,double brightness){
    TraceFixture225 *t=fixture;if(!t)return;
    t->brightness=brightness;t->detail=-1;pix3d_init_colour_table(brightness);
}
void painter_trace_free_asset(Asset *a){if(a){model_free_unshared(a->model);free(a);}}
bool painter_trace_begin(void *fixture,uint32_t *pixels,const int32_t frame[15]){
    TraceFixture225 *t=fixture;
    if(!t || !pixels || frame[1]!=512 || frame[2]!=334 || frame[11]<0 || frame[11]>1)return false;
    if(t->detail!=frame[11] || t->generation!=pix3d225_texture_cache_generation()){
        t->detail=frame[11];pix3d_low_detail=t->detail!=0;pix3d225_texture_cache_init(pix3d_low_detail);
        for(int id=0;id<50;id++){
            char name[8];snprintf(name,sizeof(name),"%d",id);
            Pix8 *image=pix8_load(t->textures,name,0,NULL);if(!image)return false;
            if(pix3d_low_detail && image->crop_w==128)pix8_shrink(image,NULL);else pix8_crop(image,NULL);
            pix3d225_texture_cache_set_image(id,image);pix8_free(image);
        }
        pix3d225_texture_cache_set_brightness(t->brightness);
        pix3d_init_colour_table(t->brightness);
        t->generation=pix3d225_texture_cache_generation();
    }
    pix2d_bind(pixels,512,334);pix3d_init_2d();pix3d_jagged=frame[12]!=0;
    pix3d_set_trans(frame[13]);pix3d_set_hclip(frame[14]!=0);return true;
}
void painter_trace_close(void *fixture){
    TraceFixture225 *t=fixture;if(!t)return;
    pix3d225_texture_cache_clear();arena_free(&t->arena);free(t);
    free(pix3d_scanline);pix3d_scanline=NULL;
}
#else
static bool tile(FILE *f,Ground225 *view,bool draw){
    int kind=integer(f),x=integer(f),z=integer(f);int32_t heights[4];for(int i=0;i<4;i++)heights[i]=integer(f);
    if(kind==0){
        TileUnderlay u={0};u.southwest_color=integer(f);u.southeast_color=integer(f);u.northeast_color=integer(f);u.northwest_color=integer(f);u.texture_id=integer(f);u.rgb=integer(f);u.flat=word(f)!=0;
#ifdef PAINTER_TRACE_SKIP_GROUND
        (void)u;(void)view;(void)draw;(void)x;(void)z;return true;
#else
        return world3d_underlay225(&u,heights,x,z,view);
#endif
    }
    if(kind!=1)exit(2);
    TileOverlay o={0};o.flat=word(f)!=0;o.shape=integer(f);o.rotation=integer(f);o.background_rgb=integer(f);o.foreground_rgb=integer(f);o.triangle_texture_id=word(f)?0:-1;o.vertex_count=integer(f);o.triangle_count=integer(f);
    if(o.vertex_count<0 || o.vertex_count>6 || o.triangle_count<0 || o.triangle_count>10)exit(2);
    for(int i=0;i<o.vertex_count;i++){o.vertex_x[i]=integer(f);o.vertex_y[i]=integer(f);o.vertex_z[i]=integer(f);}
    for(int i=0;i<o.triangle_count;i++){o.triangle_vertex_a[i]=integer(f);o.triangle_vertex_b[i]=integer(f);o.triangle_vertex_c[i]=integer(f);o.triangle_color_a[i]=integer(f);o.triangle_color_b[i]=integer(f);o.triangle_color_c[i]=integer(f);o.triangle_texture[i]=integer(f);}
#ifdef PAINTER_TRACE_SKIP_GROUND
    return true;
#else
    return world3d_overlay225(&o,x,z,draw,view);
#endif
}
int main(int argc,char **argv){
    if(argc!=3){fputs("usage: test_painter_trace_reference textures-archive trace\n",stderr);return 2;}
    FILE *f=fopen(argv[2],"rb");if(!f){perror(argv[2]);return 2;}
    if(word(f)!=0x50323235)return 2;
    union {uint64_t u;double d;} brightness;brightness.u=(uint64_t)word(f)<<32;brightness.u|=word(f);if(!(brightness.d>0.7 && brightness.d<0.9))return 2;
    pix3d_init();pix3d_init_colour_table(brightness.d);Arena arena=arena_create(8*1024*1024);Jagfile *textures=jagfile_load_file(argv[1],&arena);if(!textures)return 2;
    Asset *assets=calloc(10000,sizeof(*assets));if(!assets)return 2;unsigned asset_count=0,frames=0,model_count=0,under_count=0,over_count=0,failed=0;
    uint64_t bad=0,checked=0,state_bad=0;uint32_t *storage=NULL,*pixels=NULL;int frame[15]={0},detail=-1;bool recording=false;Ground225 view={0};
    for(;;){
        unsigned tag=word(f);
        if(tag==0){
            if(recording)return 2;
            for(int i=0;i<15;i++)frame[i]=integer(f);
            if(frame[0]!=(int)frames || frame[1]!=512 || frame[2]!=334 || frame[11]<0 || frame[11]>1)return 2;
            storage=malloc((512*334+16)*sizeof(*storage));if(!storage)return 2;pixels=storage+8;for(int i=0;i<512*334+16;i++)storage[i]=0x19372b;
            if(detail!=frame[11]){
                detail=frame[11];pix3d_low_detail=detail!=0;pix3d225_texture_cache_init(pix3d_low_detail);
                for(int id=0;id<50;id++){char name[8];snprintf(name,sizeof(name),"%d",id);Pix8 *image=pix8_load(textures,name,0,NULL);if(!image)return 2;if(pix3d_low_detail && image->crop_w==128)pix8_shrink(image,NULL);else pix8_crop(image,NULL);pix3d225_texture_cache_set_image(id,image);pix8_free(image);}
                pix3d225_texture_cache_set_brightness(brightness.d);
            }
            pix2d_bind(pixels,512,334);pix3d_init_2d();pix3d_jagged=frame[12]!=0;pix3d_set_trans(frame[13]);pix3d_set_hclip(frame[14]!=0);
            view=(Ground225){frame[4],frame[7],frame[8],frame[6],frame[3],frame[10]!=0,false,0,0,-1,-1};recording=true;
        }else if(tag==1){
            unsigned id=word(f),bytes=word(f);if(!recording || id!=asset_count || id>=10000 || bytes>1000000)return 2;
            long start=ftell(f);assets[id]=asset(f);if(ftell(f)-start!=(long)bytes)return 2;asset_count++;
        }else if(tag==2){
            if(!recording)return 2;
            int c[12];for(int i=0;i<12;i++)c[i]=integer(f);if(c[0]<0 || (unsigned)c[0]>=asset_count)return 2;
            if(pix3d_trans!=c[10] || pix3d_hclip!=(c[11]!=0))state_bad++;
            if(c[2]!=pix3d_sin_table[frame[6]] || c[3]!=pix3d_cos_table[frame[6]] || c[4]!=pix3d_sin_table[frame[3]] || c[5]!=pix3d_cos_table[frame[3]])return 2;
            Asset *a=&assets[c[0]];
#ifdef PAINTER_TRACE_SKIP_MODELS
            (void)a;
#else
#ifdef PAINTER_TRACE_RECOMPUTE_BOUNDS
            bool ok=model_render_world225(a->model,c[1],frame[6],frame[3],c[6],c[7],c[8]);
#else
            bool ok=model_render_world_bounds225(a->model,&a->bounds,c[1],frame[6],frame[3],c[6],c[7],c[8]);
#endif
            if(!ok){fprintf(stderr,"recorded model rejected frame=%u asset=%d\n",frames,c[0]);return 2;}
#endif
            model_count++;
        }else if(tag==3 || tag==4){
            if(!recording)return 2;
            int c[7];for(int i=0;i<7;i++)c[i]=integer(f);
            if(pix3d_trans!=c[0] || pix3d_hclip!=(c[1]!=0))state_bad++;
            if(!tile(f,&view,tag==3 || c[6]!=0))return 2;
            if(tag==3)under_count++;else over_count++;
        }else if(tag==5){
            if(!recording)return 2;
            int index=integer(f),alpha=integer(f),clip=integer(f),remaining=integer(f);unsigned count=word(f);
            if(index!=(int)frames || remaining!=0 || count!=512*334+8)return 2;
            uint64_t before=bad;for(unsigned i=0;i<count;i++){uint32_t expected=word(f);if(pixels[i]!=expected){if(bad<8)printf("frame %u word %u C=%08x Java=%08x\n",frames,i,pixels[i],expected);bad++;}checked++;}
            bool state=pix3d_trans!=alpha || pix3d_hclip!=(clip!=0);state_bad+=state;
            for(int i=0;i<8;i++)if(storage[i]!=0x19372b){fputs("painter prefix guard damaged\n",stderr);return 1;}
            if(before!=bad || state)failed++;
            free(storage);storage=NULL;pixels=NULL;recording=false;frames++;
        }else if(tag==9){
            if(recording || word(f)!=frames || word(f)!=model_count || word(f)!=under_count || word(f)!=over_count || word(f)!=asset_count || fgetc(f)!=EOF)return 2;
            break;
        }else return 2;
    }
    for(unsigned i=0;i<asset_count;i++)model_free_unshared(assets[i].model);
    free(assets);fclose(f);pix3d225_texture_cache_clear();arena_free(&arena);free(pix3d_scanline);pix3d_scanline=NULL;
    printf("Painter command replay: %u/%u frames; %llu framebuffer/guard words, %llu differences; %llu entry/final state differences; prefix guards intact\n",frames-failed,frames,(unsigned long long)checked,(unsigned long long)bad,(unsigned long long)state_bad);
    printf("Original commands: %u models, %u underlays, %u overlays; %u model snapshots\n",model_count,under_count,over_count,asset_count);
    return failed || state_bad?1:0;
}
#endif
