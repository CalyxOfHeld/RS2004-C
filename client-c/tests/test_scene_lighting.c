#include "dash3d/world.h"
#include "gfx/pix3d.h"
#include "gfx/pix2d.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int context;
static int rd(FILE *f) {unsigned char b[4];assert(fread(b,1,4,f)==4);return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static void same(int want,int got,const char *field,int index) {
    if(want!=got){fprintf(stderr,"scene lighting case=%d %s index=%d expected=%d actual=%d\n",context,field,index,want,got);abort();}
}
static void normals(FILE *f,ModelNormal *n,int count) {
    assert(n);
    for(int i=0;i<count;i++){same(rd(f),n[i].x,"normal.x",i);same(rd(f),n[i].y,"normal.y",i);same(rd(f),n[i].z,"normal.z",i);same(rd(f),n[i].w,"normal.w",i);}
}
static void before(FILE *f,Model *m) {
    assert(m);same(rd(f),m->num_points,"vertices",0);same(rd(f),m->num_faces,"faces",0);
    for(int i=0;i<m->num_points;i++){same(rd(f),m->point_x[i],"vertex.x",i);same(rd(f),m->point_y[i],"vertex.y",i);same(rd(f),m->point_z[i],"vertex.z",i);}
    normals(f,m->vertex_normals_original,m->num_points);
}
static void faces(FILE *f,Model *m) {
    for(int i=0;i<m->num_faces;i++) {
        int type=rd(f),a=rd(f),b=rd(f),c=rd(f);if(type==-1)type=255;
        same(type,m->face_render_type?m->face_render_type[i]:0,"face.type",i);
        if(type==255)continue; // Hidden-face shades do not reach the renderer.
        same(a,m->face_colour_a[i],"face.A",i);
        if(!(type&1)){same(b,m->face_colour_b[i],"face.B",i);same(c,m->face_colour_c[i],"face.C",i);}
    }
}
static unsigned char *raw(const char *path,size_t *length) {
    FILE *f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);*length=ftell(f);rewind(f);
    unsigned char *data=malloc(*length);assert(data && fread(data,1,*length,f)==*length);fclose(f);return data;
}
static Model *triangle(bool flat) {
    int x[]={0,128,0},y[]={0,0,0},z[]={0,0,128},a=0,b=1,c=2,col=500,type=flat?1:0;
    Model src={.num_points=3,.num_faces=1,.point_x=x,.point_y=y,.point_z=z,.face_vertex_a=&a,.face_vertex_b=&b,.face_vertex_c=&c,.face_colour=&col,.face_render_type=&type};
    Model *m=model_copy(&src,NULL);model_calculate_normals_deferred(m,70,800,-50,-10,-50);return m;
}
static void boundaries(void) {
    Model *a=triangle(false),*b=triangle(false);b->point_z[2]=256;
    assert(model_merge_normals(a,b,0,0,0,true));
    assert(a->face_render_type[0]==0 && b->face_render_type[0]==0);
    assert(a->vertex_normals[0].y==-512 && a->vertex_normals[0].w==2);
    assert(a->vertex_normals[2].y==-256 && a->vertex_normals[2].w==1);
    b->point_z[2]=128;model_reset_scene_normals(a);model_reset_scene_normals(b);
    assert(model_merge_normals(a,b,0,0,0,true));assert(a->face_render_type[0]==255 && b->face_render_type[0]==255);
    model_reset_scene_normals(a);model_reset_scene_normals(b);model_translate(b,7,0,0);
    assert(model_merge_normals(a,b,0,-7,0,false));assert(a->face_render_type[0]==0 && b->face_render_type[0]==0);
    assert(a->vertex_normals[2].w==2 && b->vertex_normals[2].w==2);
    Model *copy=model_copy(a,NULL);loc_clear_model_cache(true);assert(copy->vertex_normals!=a->vertex_normals);
    model_reset_scene_normals(copy);assert(copy->vertex_normals[2].w==1 && a->vertex_normals[2].w==2);
    Arena frame=arena_create(4096);Model *temporary=model_copy(a,&frame);assert(temporary && temporary->vertex_normals!=a->vertex_normals);
    model_reset_scene_normals(temporary);assert(temporary->vertex_normals[2].w==1);model_free_unshared(temporary);arena_reset(&frame);
    assert(a->vertex_normals[2].w==2);arena_free(&frame);
    Model *flat=triangle(true);int shade=flat->face_colour_a[0];model_apply_scene_lighting(flat,64,213,-50,-10,-50);
    assert(flat->face_colour_a[0]==shade && flat->vertex_normals_original[0].w==0);
    model_free_unshared(flat);model_free_unshared(copy);model_free_unshared(a);model_free_unshared(b);
    static uint32_t pixels[256*256];pix2d_bind(pixels,256,256);pix3d_init_2d();pix3d_init_colour_table(0.8);pix3d_begin_scene();
    Model *pick=triangle(false);int x[]={-32,0,32},y[]={-32,32,-32};
    for(int v=0;v<3;v++){pick->point_x[v]=x[v];pick->point_y[v]=y[v];pick->point_z[v]=0;}
    model_calculate_normals(pick,64,768,-50,-10,-50);pick->face_render_type[0]=255;
    assert(!model_draw_pickable(pick,0,0,0,0,0,400,128,128));
    for(int i=0;i<256*256;i++)assert(pixels[i]==0);
    pick->face_render_type[0]=0;assert(model_draw_pickable(pick,0,0,0,0,0,400,128,128));
    pix3d_end_scene();model_free_unshared(pick);
    puts("PASS: exact two/three-vertex boundary, bidirectional sums, hidden faces, no-hide pair, vertical offset, flat shade and independent retained normal ownership");
}
int main(void) {
    setbuf(stdout,NULL);pix3d_init();boundaries();Arena perm=arena_create(64*1024*1024);
    Jagfile *cfg=jagfile_load_file("../engine/data/pack/client/config",&perm),*models=jagfile_load_file("../engine/data/pack/client/models",&perm);assert(cfg && models);
    model_init_repo(models,&perm);LocConfig locs;loc_unpack(&locs,cfg);
    FILE *f=fopen("tests/fixtures/scene-lighting-reference.bin","rb");assert(f && rd(f)==0x534c5431);
    int cases=rd(f);
    for(context=0;context<cases;context++) {
        int id=rd(f),shape=rd(f),rot=rd(f);rd(f);int h[4];for(int i=0;i<4;i++)h[i]=rd(f);
        int dx=rd(f),dy=rd(f),dz=rd(f),hide=rd(f);LocType loc;assert(loc_get(&locs,id,&loc));
        Model *a=loc_get_model(&loc,shape,rot,NULL),*b=loc_get_model(&loc,shape,rot,NULL);
        loc_skew_model(a,&loc,h[0],h[1],h[2],h[3]);loc_skew_model(b,&loc,h[0],h[1],h[2],h[3]);
        before(f,a);before(f,b);assert(model_merge_normals(a,b,dx,dy,dz,hide));
        normals(f,a->vertex_normals,a->num_points);normals(f,b->vertex_normals,b->num_points);
        model_apply_scene_lighting(a,64,213,-50,-10,-50);model_apply_scene_lighting(b,64,213,-50,-10,-50);
        faces(f,a);faces(f,b);model_free_unshared(a);model_free_unshared(b);loc_free_entry(&loc);
    }
    printf("PASS: %d original real-cache model pair merges, pre-skew normals, post-merge sums, rendered face shades and hidden-face states\n",cases);
    World *w=calloc(1,sizeof(*w));World3D *s=calloc(1,sizeof(*s));assert(w && s);world_init(w,104,104);world3d_init(s,104,104,4);
    size_t mlen,llen;unsigned char *mdata=raw("tests/fixtures/scene-lighting/m50_50.raw",&mlen),*ldata=raw("tests/fixtures/scene-lighting/l50_50.raw",&llen);
    world_load_ground(w,mdata,mlen,20,20,3180,3180);world_load_locations(w,ldata,llen,&locs,s,NULL,20,20);
    world_build(w,s,NULL,NULL);long start=ftell(f);int count=0;
    for(int pass=0;pass<4;pass++) {
        if(pass){
            loc_clear_model_cache(true);
            if(pass==3){world3d_reset(s);world_reset(w);world_load_ground(w,mdata,mlen,20,20,3180,3180);world_load_locations(w,ldata,llen,&locs,s,NULL,20,20);world_build(w,s,NULL,NULL);}
            else assert(world3d_build_models(s));
            fseek(f,start,SEEK_SET);
        }
        count=rd(f);
        for(context=0;context<count;context++) {
            int l=rd(f),x=rd(f),z=rd(f),id=rd(f),shape=rd(f),part=rd(f);GroundTile *t=&s->tiles[l][x][z];Model *found=NULL;
            for(int j=0,idx=0;j<t->loc_count;j++) if(t->locs[j].shape==shape && ((t->locs[j].bitset>>14)&32767)==id) {
                if(idx++==part){found=t->locs[j].model;break;}
            }
            if(!found){fprintf(stderr,"Missing native map model at %d,%d,%d id=%d shape=%d part=%d\n",l,x,z,id,shape,part);abort();}
            before(f,found);faces(f,found);
        }
        assert(fgetc(f)==EOF);
    }
    printf("PASS: %d original full Lumbridge map models including real skew, bridge levels, walls, scenery and ground decor; three deterministic builds plus a full scene reload with cache eviction\n",count);
    fclose(f);free(mdata);free(ldata);world3d_dispose(s);free(s);free(w);loc_clear_model_cache(true);loc_free(&locs);arena_free(&perm);return 0;
}
