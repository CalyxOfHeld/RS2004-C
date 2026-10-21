#include "dash3d/world3d.h"
#include <stdlib.h>
#include <math.h>
#include <limits.h>

/* World3D.buildModels runs before bridge relinking. The native scene stores
 * locations at their anchors, so this temporary index reconstructs the original
 * wall/loc/ground-decor buckets, including every tile of a large loc's span. */
typedef struct {Model *model;int x,z,width,length;bool active;} LightingEntry;
typedef struct {LightingEntry *walls[2],*ground;int head,tail;} LightingCell;
typedef struct {LightingEntry *entry;int next;} LightingRef;
typedef struct {World3D *scene;LightingCell *cells;LightingRef *refs;} LightingBuild;

static LightingCell *cell(LightingBuild *b,int l,int x,int z) {
    return &b->cells[(l*b->scene->max_tile_x+x)*b->scene->max_tile_z+z];
}
static int height(World3D *s,int l,int x,int z) {
    return (s->heightmap[l][x][z]+s->heightmap[l][x+1][z]+
        s->heightmap[l][x][z+1]+s->heightmap[l][x+1][z+1])/4;
}
static bool merge_location(LightingBuild *b,LightingEntry *e,int level,int x,int z) {
    World3D *s=b->scene;int start=x,end=x+e->width,last=z+e->length;
    bool hide=true;
    for(int l=level;l<=level+1 && l<s->max_level;l++) {
        for(int xx=start;xx<=end;xx++) if(xx>=0 && xx<s->max_tile_x)
            for(int zz=z-1;zz<=last;zz++) if(zz>=0 && zz<s->max_tile_z &&
                (!hide || xx>=end || zz>=last || (zz<z && xx!=x))) {
                LightingCell *c=cell(b,l,xx,zz);int dy=height(s,l,xx,zz)-height(s,level,x,z);
                for(int part=0;part<2;part++) {
                    LightingEntry *n=c->walls[part];
                    if(n && n->active && !model_merge_normals(e->model,n->model,
                        (xx-x)*128+(1-e->width)*64,dy,(zz-z)*128+(1-e->length)*64,hide)) return false;
                }
                for(int ref=c->head;ref;ref=b->refs[ref].next) {
                    LightingEntry *n=b->refs[ref].entry;
                    if(n->active && !model_merge_normals(e->model,n->model,
                        (n->x-x)*128+(n->width-e->width)*64,dy,
                        (n->z-z)*128+(n->length-e->length)*64,hide)) return false;
                }
            }
        start--;hide=false;
    }
    return true;
}
static void finish(LightingEntry *e) {
    model_apply_scene_lighting(e->model,64,768*(int)sqrt(5100.0)>>8,-50,-10,-50);
    // Equivalent to Java applyLighting releasing vertexNormal: later entries
    // must not merge this model again. Retain immutable data for a C rebuild.
    e->active=false;
}

bool world3d_build_models(World3D *s) {
    if(!s) return false;
    size_t count=0,refs=0;
    for(int l=0;l<s->max_level;l++) for(int x=0;x<s->max_tile_x;x++) for(int z=0;z<s->max_tile_z;z++) {
        GroundTile *t=&s->tiles[l][x][z];
        for(int j=0;j<t->loc_count;j++) {
            WorldLocation *loc=&t->locs[j];int shape=loc->shape;
            if(!loc->model->vertex_normals_original || (shape>=4 && shape<=8)) continue;
            count++;
            if(shape>=9 && shape<=21) {
                int width=shape==10 || shape==11 ? loc->max_tile_x-x+1:1;
                int length=shape==10 || shape==11 ? loc->max_tile_z-z+1:1;
                if(width<1 || length<1 || x+width>s->max_tile_x || z+length>s->max_tile_z) return false;
                refs+=(size_t)width*length;
            }
        }
    }
    if(!count) return true;
    if(refs>INT_MAX-1) return false;
    LightingEntry *entries=calloc(count,sizeof(*entries));
    LightingBuild b={s,calloc((size_t)s->max_level*s->max_tile_x*s->max_tile_z,sizeof(LightingCell)),calloc(refs+1,sizeof(LightingRef))};
    if(!entries || !b.cells || !b.refs) {free(entries);free(b.cells);free(b.refs);return false;}
    size_t index=0;int next=1;
    for(int l=0;l<s->max_level;l++) for(int x=0;x<s->max_tile_x;x++) for(int z=0;z<s->max_tile_z;z++) {
        GroundTile *t=&s->tiles[l][x][z];LightingCell *c=cell(&b,l,x,z);
        for(int j=0;j<t->loc_count;j++) {
            WorldLocation *loc=&t->locs[j];int shape=loc->shape;
            if(!loc->model->vertex_normals_original || (shape>=4 && shape<=8)) continue;
            LightingEntry *e=&entries[index++];e->model=loc->model;e->x=x;e->z=z;e->active=true;
            e->width=shape==10 || shape==11 ? loc->max_tile_x-x+1:1;
            e->length=shape==10 || shape==11 ? loc->max_tile_z-z+1:1;
            model_reset_scene_normals(e->model);
            if(shape<4) {
                if(shape==2 && c->walls[0] && j>0 && t->locs[j-1].bitset==loc->bitset) c->walls[1]=e;
                else {c->walls[0]=e;c->walls[1]=NULL;}
            } else if(shape==22) c->ground=e;
            else for(int xx=x;xx<x+e->width;xx++) for(int zz=z;zz<z+e->length;zz++) {
                LightingCell *span=cell(&b,l,xx,zz);b.refs[next].entry=e;
                if(span->tail) b.refs[span->tail].next=next;else span->head=next;
                span->tail=next++;
            }
        }
    }
    bool ok=true;
    for(int l=0;l<s->max_level && ok;l++) for(int x=0;x<s->max_tile_x && ok;x++) for(int z=0;z<s->max_tile_z && ok;z++) {
        LightingCell *c=cell(&b,l,x,z);LightingEntry *a=c->walls[0],*other=c->walls[1];
        if(a && a->active) {
            ok=merge_location(&b,a,l,x,z);
            if(ok && other && other->active) {
                ok=merge_location(&b,other,l,x,z) && model_merge_normals(a->model,other->model,0,0,0,false);
                if(ok) finish(other);
            }
            if(ok) finish(a);
        }
        for(int ref=c->head;ref && ok;ref=b.refs[ref].next) {
            LightingEntry *e=b.refs[ref].entry;
            if(e->active) {ok=merge_location(&b,e,l,x,z);if(ok) finish(e);}
        }
        a=c->ground;
        if(ok && a && a->active) {
            static const int dx[]={1,0,1,1},dz[]={0,1,1,-1};
            for(int k=0;k<4 && ok;k++) {
                int xx=x+dx[k],zz=z+dz[k];
                if(xx<0 || zz<0 || xx>=s->max_tile_x || zz>=s->max_tile_z) continue;
                LightingEntry *n=cell(&b,l,xx,zz)->ground;
                if(n && n->active) ok=model_merge_normals(a->model,n->model,dx[k]*128,0,dz[k]*128,true);
            }
            if(ok) finish(a);
        }
    }
    free(entries);free(b.cells);free(b.refs);return ok;
}
