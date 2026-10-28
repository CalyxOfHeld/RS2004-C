/* Original World3D allocation/addLoc/setBridge and World.build occluders.
 * The existing native loaders supply real-cache models and terrain. */
#include "dash3d/scene_build225.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define CELLS (4*104*104)
#define CAPACITY (CELLS+104*104)
/* model/serial: the owner's model when bounds were last computed. */
typedef struct {WorldLocation *owner;int bitset,shape,rotation;bool shared,skew;Model *model;uint32_t serial;} ModelBinding225;
typedef struct {int location,model;SceneEntityDraw225 draw;void *user;} Temporary225;
struct SceneBuild225 {
    SceneTraversal225 scene;
    SceneGeometry225 *geometry;
    SceneModel225 *models;
    size_t model_count,model_capacity;
    ModelBinding225 *bindings;
    size_t base_models,base_locations,temporary_count;
    uint32_t temporary_generation;
    Temporary225 temporary[5000];
    int object_ground[CELLS];size_t object_count;
    Occluder225 pools[4][500];size_t pool_counts[4];
    int extra;
    /* Heights that copy a base model's bounds.max_y (wall, decor, ground decor,
     * base location), grouped by model: model i's are height_target
     * [height_start[i]..height_start[i+1]). Built by the first refresh. */
    int32_t **height_target;size_t *height_start;bool heights_indexed;
};
static int cell(int l,int x,int z){return (l*104+x)*104+z;}
static int32_t jint(uint32_t value){return value<=INT32_MAX?(int32_t)value:-1-(int32_t)(UINT32_MAX-value);}
static int32_t square_sum(int32_t a,int32_t b){return jint((uint32_t)a*(uint32_t)a+(uint32_t)b*(uint32_t)b);}
static int depth(int32_t value,double bias){return value<0?0:(int)(sqrt((double)value)+bias);}
static SceneGround225 *ensure(SceneBuild225 *b,int l,int x,int z){
    int index=cell(l,x,z),id=b->scene.grid[index];
    if(id<0){
        /* A bridge can move this index's original Ground down a level.
         * Preserve that identity when the vacated cell is populated again.
         * One bridge shift needs at most one extra Ground per map cell. */
        id=b->scene.grounds[index].id<0?index:CELLS+b->extra++;
        b->scene.grid[index]=id;
        SceneGround225 *g=&b->scene.grounds[id];
        g->id=id;g->level=g->occlude_level=l;g->x=x;g->z=z;
        g->bridge=g->prev=g->next=-1;
    }
    return &b->scene.grounds[id];
}
static SceneGround225 *ensure_below(SceneBuild225 *b,int l,int x,int z){
    for(int level=l;level>=0;level--)ensure(b,level,x,z);
    return ensure(b,l,x,z);
}
static void model_bounds(SceneModel225 *out,Model *model,bool shared,bool skew){
    memset(out,0,sizeof(*out));out->model=model;
    int radius2=0,min_y=0;
    for(int i=0;i<model->num_points;i++){
        int x=model->point_x[i],y=model->point_y[i],z=model->point_z[i];
        int square=square_sum(x,z),negative_y=jint(0u-(uint32_t)y);
        if(square>radius2)radius2=square;
        if(negative_y>out->bounds.max_y)out->bounds.max_y=negative_y;
        if(y>min_y)min_y=y;
    }
    /* calculateNormals chooses AABB for shared lighting; hillskew then uses
     * calculateBoundsY, retaining radius but rounding depths upward. */
    double radius_bias=shared?0.0:0.99;
    double depth_bias=shared && !skew?0.0:0.99;
    out->bounds.radius=(int)(sqrt((double)radius2)+radius_bias);
    int radius=out->bounds.radius,height=out->bounds.max_y;
    out->bounds.min_depth=depth(square_sum(radius,height),depth_bias);
    out->bounds.max_depth=jint((uint32_t)out->bounds.min_depth+(uint32_t)depth(square_sum(radius,min_y),depth_bias));
}
static void encode_key(uint8_t key[32],uint32_t id,uint32_t generation){
    memset(key,0,32);for(int i=0;i<4;i++){key[i]=(uint8_t)(id>>(i*8));key[4+i]=(uint8_t)(generation>>(i*8));}
}
static bool model_key(SceneBuild225 *b,WorldLocation *owner,const LocType *loc,uint8_t key[32],int *max_y){
    if(!owner || !owner->model){memset(key,0,32);*max_y=0;return true;}
    if(b->model_count>=b->model_capacity)return false;
    SceneModel225 *out=&b->models[b->model_count];model_bounds(out,owner->model,loc->sharelight,loc->hillskew);
    b->bindings[b->model_count]=(ModelBinding225){owner,owner->bitset,owner->shape,owner->rotation,loc->sharelight,loc->hillskew,owner->model,owner->model_serial};
    encode_key(key,(uint32_t)++b->model_count,0);*max_y=out->bounds.max_y;return true;
}
const SceneModel225 *world3d_scene_resolve225(void *context,const uint8_t key[32]){
    SceneBuild225 *b=context;if(!b || !key)return NULL;
    uint32_t id=0,generation=0;for(int i=0;i<4;i++){id|=(uint32_t)key[i]<<(8*i);generation|=(uint32_t)key[i+4]<<(8*i);}
    for(int i=8;i<32;i++)if(key[i])return NULL;
    if(generation!=(id<=b->base_models?0:b->temporary_generation))return NULL;
    return id && id<=b->model_count?&b->models[id-1]:NULL;
}
static bool add_occluder(SceneBuild225 *b,int top,int type,int min_x,int max_x,
    int min_z,int max_z,int min_y,int max_y){
    size_t index=b->pool_counts[top];if(index>=500)return false;
    Occluder225 *o=&b->pools[top][index];b->pool_counts[top]++;
    o->type=type;o->min_x=min_x;o->max_x=max_x;o->min_z=min_z;o->max_z=max_z;
    o->min_y=min_y;o->max_y=max_y;o->min_tile_x=min_x/128;o->max_tile_x=max_x/128;
    o->min_tile_z=min_z/128;o->max_tile_z=max_z/128;return true;
}
static bool build_occluders(SceneBuild225 *b,const World *w){
    int (*map)[105][105]=malloc(sizeof(w->occludemap));if(!map)return false;
    memcpy(map,w->occludemap,sizeof(w->occludemap));bool ok=true;
    for(int top=0;top<4 && ok;top++)for(int l=0;l<=top && ok;l++)
    for(int z=0;z<=104 && ok;z++)for(int x=0;x<=104 && ok;x++){
        int mask=1<<(top*3),other=2<<(top*3),roof=4<<(top*3);
        if(map[l][x][z]&mask){
            int z0=z,z1=z,l0=l,l1=l;
            while(z0>0 && (map[l][x][z0-1]&mask))z0--;
            while(z1<104 && (map[l][x][z1+1]&mask))z1++;
            while(l0>0){bool all=true;for(int zz=z0;zz<=z1;zz++)if(!(map[l0-1][x][zz]&mask))all=false;if(!all)break;l0--;}
            while(l1<top){bool all=true;for(int zz=z0;zz<=z1;zz++)if(!(map[l1+1][x][zz]&mask))all=false;if(!all)break;l1++;}
            if((l1+1-l0)*(z1-z0+1)>=8){
                ok=add_occluder(b,top,1,x*128,x*128,z0*128,z1*128+128,w->heightmap[l1][x][z0]-240,w->heightmap[l0][x][z0]);
                for(int ll=l0;ll<=l1;ll++)for(int zz=z0;zz<=z1;zz++)map[ll][x][zz]&=~mask;
            }
        }
        if(map[l][x][z]&other){
            int x0=x,x1=x,l0=l,l1=l;
            while(x0>0 && (map[l][x0-1][z]&other))x0--;
            while(x1<104 && (map[l][x1+1][z]&other))x1++;
            while(l0>0){bool all=true;for(int xx=x0;xx<=x1;xx++)if(!(map[l0-1][xx][z]&other))all=false;if(!all)break;l0--;}
            while(l1<top){bool all=true;for(int xx=x0;xx<=x1;xx++)if(!(map[l1+1][xx][z]&other))all=false;if(!all)break;l1++;}
            if((l1+1-l0)*(x1-x0+1)>=8){
                ok=add_occluder(b,top,2,x0*128,x1*128+128,z*128,z*128,w->heightmap[l1][x0][z]-240,w->heightmap[l0][x0][z]);
                for(int ll=l0;ll<=l1;ll++)for(int xx=x0;xx<=x1;xx++)map[ll][xx][z]&=~other;
            }
        }
        if(map[l][x][z]&roof){
            int x0=x,x1=x,z0=z,z1=z;
            while(z0>0 && (map[l][x][z0-1]&roof))z0--;
            while(z1<104 && (map[l][x][z1+1]&roof))z1++;
            while(x0>0){bool all=true;for(int zz=z0;zz<=z1;zz++)if(!(map[l][x0-1][zz]&roof))all=false;if(!all)break;x0--;}
            while(x1<104){bool all=true;for(int zz=z0;zz<=z1;zz++)if(!(map[l][x1+1][zz]&roof))all=false;if(!all)break;x1++;}
            if((x1-x0+1)*(z1-z0+1)>=4){
                int h=w->heightmap[l][x0][z0];
                ok=add_occluder(b,top,4,x0*128,x1*128+128,z0*128,z1*128+128,h,h);
                for(int xx=x0;xx<=x1;xx++)for(int zz=z0;zz<=z1;zz++)map[l][xx][zz]&=~roof;
            }
        }
    }
    free(map);return ok;
}
static bool place(SceneBuild225 *b,World3D *native,const WorldMapLocation *meta,
    WorldLocation *parts[2],const LocType *type){
    int l=meta->level,x=meta->x,z=meta->z,shape=meta->shape,rotation=meta->rotation;
    uint32_t bits=(uint32_t)x+((uint32_t)z<<7)+((uint32_t)meta->id<<14)+UINT32_C(0x40000000);
    if(!meta->active)bits+=UINT32_C(0x80000000);
    int bitset=bits<=INT32_MAX?(int)bits:-1-(int)(UINT32_MAX-bits);
    int y=(native->heightmap[l][x][z]+native->heightmap[l][x+1][z]+
        native->heightmap[l][x+1][z+1]+native->heightmap[l][x][z+1])>>2;
    uint8_t key[2][32]={{0}};int max_y[2]={0};
    for(int i=0;i<2;i++)if(!model_key(b,parts[i],type,key[i],&max_y[i]))return false;
    if(!parts[0] && !parts[1])return true;
    SceneGround225 *g;
    if(shape==22){
        g=ensure(b,l,x,z);g->ground_decor=(SceneGroundDecor225){true,x*128+64,y,z*128+64,bitset,max_y[0],{0}};
        memcpy(g->ground_decor.model,key[0],32);
    }else if(shape<4){
        g=ensure_below(b,l,x,z);SceneWall225 *wall=&g->wall;
        wall->present=true;wall->x=x*128+64;wall->y=y;wall->z=z*128+64;wall->bitset=bitset;
        wall->type_a=(shape==1 || shape==3)?16<<rotation:1<<rotation;
        wall->type_b=shape==2?1<<((rotation+1)&3):0;
        wall->max_y_a=max_y[0];wall->max_y_b=max_y[1];memcpy(wall->model_a,key[0],32);memcpy(wall->model_b,key[1],32);
    }else if(shape<=8){
        g=ensure_below(b,l,x,z);SceneDecor225 *decor=&g->decor;
        decor->present=true;decor->x=parts[0]->x;decor->y=y;decor->z=parts[0]->z;decor->bitset=bitset;
        decor->type=shape<=5?1<<rotation:(shape-5)*256;
        decor->angle=shape<=5?rotation*512:rotation;
        decor->max_y=max_y[0];memcpy(decor->model,key[0],32);
    }else{
        if(b->scene.location_count>=native->map_location_count)return false;
        int width=(shape==10 || shape==11)?((rotation&1)?type->length:type->width):1;
        int length=(shape==10 || shape==11)?((rotation&1)?type->width:type->length):1;
        /* World ignores addLoc's refusal at scene edges or a full tile.
         * No part of the footprint is inserted in either case. */
        if(x+width>104 || z+length>104)return true;
        for(int xx=x;xx<x+width;xx++)for(int zz=z;zz<z+length;zz++){
            int ground=b->scene.grid[cell(l,xx,zz)];
            if(ground>=0 && b->scene.grounds[ground].loc_count>=5)return true;
        }
        int id=(int)b->scene.location_count++;SceneLocation225 *loc=&b->scene.locations[id];
        loc->id=id;loc->level=l;loc->x=x*128+width*64;loc->z=z*128+length*64;loc->y=y;
        loc->yaw=shape==11?256:0;loc->min_x=x;loc->min_z=z;loc->max_x=x+width-1;loc->max_z=z+length-1;
        loc->bitset=bitset;int info=shape+(rotation<<6);loc->info=info<128?info:info-256;
        loc->max_y=max_y[0];memcpy(loc->model,key[0],32);
        for(int xx=x;xx<x+width;xx++)for(int zz=z;zz<z+length;zz++){
            g=ensure_below(b,l,xx,zz);int span=(xx>x?1:0)+(xx<x+width-1?4:0)+(zz>z?8:0)+(zz<z+length-1?2:0);
            g->locs[g->loc_count]=id;g->loc_span[g->loc_count++]=span;g->loc_spans|=span;
        }
    }
    return true;
}
SceneBuild225 *world3d_construct225(World *world,World3D *native,LocConfig *locations){
    if(!world || !world->build225 || !native || !locations || native->max_level!=4 || native->max_tile_x!=104 || native->max_tile_z!=104)return NULL;
    SceneBuild225 *b=calloc(1,sizeof(*b));if(!b)return NULL;
    SceneTraversal225 *s=&b->scene;s->max_level=4;s->grid_x=s->grid_z=104;
    s->ground_count=CAPACITY;s->grounds=calloc(CAPACITY,sizeof(*s->grounds));
    s->locations=calloc(native->map_location_count+5001,sizeof(*s->locations));s->grid=malloc(CELLS*sizeof(*s->grid));
    s->occlusion.tile_cycles=calloc(4*105*105,sizeof(int32_t));s->occlusion.heights=&native->heightmap[0][0][0];
    s->occlusion.max_x=s->occlusion.max_z=104;s->occlusion.levels=4;
    b->geometry=calloc(CAPACITY,sizeof(*b->geometry));b->model_capacity=native->map_location_count*2+5000+3*CELLS;
    b->models=calloc(b->model_capacity+1,sizeof(*b->models));
    b->bindings=calloc(native->map_location_count*2+1,sizeof(*b->bindings));
    uint16_t *used=calloc(CELLS,sizeof(*used));
    if(!s->grounds || !s->locations || !s->grid || !s->occlusion.tile_cycles || !b->geometry || !b->models || !b->bindings || !used){free(used);world3d_destroy_scene225(b);return NULL;}
    for(int i=0;i<CAPACITY;i++)s->grounds[i].id=-1;
    for(int i=0;i<CELLS;i++)s->grid[i]=-1;
    for(int x=0;x<104;x++)for(int z=0;z<104;z++)ensure(b,0,x,z);
    bool ok=true;
    for(size_t i=0;i<native->map_location_count && ok;i++){
        const WorldMapLocation *meta=&native->map_locations[i];LocType type;
        if(!loc_get(locations,meta->id,&type)){ok=false;break;}
        GroundTile *tile=&native->tiles[meta->level][meta->x][meta->z];WorldLocation *parts[2]={NULL,NULL};int count=0;
        int at=cell(meta->level,meta->x,meta->z);
        for(int j=0;j<tile->loc_count && count<2;j++)if(!(used[at]&(1<<j)) && tile->locs[j].shape==meta->shape &&
            tile->locs[j].rotation==meta->rotation && ((tile->locs[j].bitset>>14)&32767)==meta->id){
            used[at]|=(uint16_t)(1<<j);parts[count++]=&tile->locs[j];if(meta->shape!=2)break;
        }
        ok=place(b,native,meta,parts,&type);
        if(!ok)fprintf(stderr,"[Scene225] Location construction failed id=%d tile=(%d,%d,%d) shape=%d rotation=%d size=%dx%d\n",
            meta->id,meta->level,meta->x,meta->z,meta->shape,meta->rotation,type.width,type.length);
        loc_free_entry(&type);
    }
    free(used);
    if(ok)for(int l=0;l<4;l++){
      for(int x=0;x<104;x++)for(int z=0;z<104;z++){
        GroundTile *tile=&native->tiles[l][x][z];if(!tile->has_underlay && !tile->has_overlay)continue;
        SceneGround225 *g=ensure_below(b,l,x,z);
        if(tile->has_underlay)b->geometry[g->id].underlay=&tile->underlay;
        if(tile->has_overlay)b->geometry[g->id].overlay=&tile->overlay;
        g->underlay=tile->has_underlay;g->overlay=tile->has_overlay;
    }
      /* World.build assigns each level before higher-level tiles can create
       * additional empty Ground objects below themselves. Those retain 0. */
      for(int x=1;x<103;x++)for(int z=1;z<103;z++){
        int id=s->grid[cell(l,x,z)];if(id<0)continue;
        s->grounds[id].draw_level=(world->tile_flags[l][x][z]&8)?0:
            l>0 && (world->tile_flags[1][x][z]&2)?l-1:l;
      }
    }
    if(ok)for(int x=0;x<104;x++)for(int z=0;z<104;z++)if(world->tile_flags[1][x][z]&2){
        int old=s->grid[cell(0,x,z)];
        for(int l=0;l<3;l++){
            int id=s->grid[cell(l,x,z)]=s->grid[cell(l+1,x,z)];if(id>=0)s->grounds[id].level--;
        }
        if(s->grid[cell(0,x,z)]<0){
            int id=CELLS+b->extra++;SceneGround225 *g=&s->grounds[id];
            g->id=id;g->x=x;g->z=z;g->bridge=g->prev=g->next=-1;s->grid[cell(0,x,z)]=id;
        }
        s->grounds[s->grid[cell(0,x,z)]].bridge=old;s->grid[cell(3,x,z)]=-1;
    }
    if(!ok || !build_occluders(b,world)){world3d_destroy_scene225(b);return NULL;}
    b->base_models=b->model_count;b->base_locations=s->location_count;b->temporary_generation=1;
    return b;
}
void world3d_destroy_scene225(SceneBuild225 *b){
    if(!b)return;
    free(b->scene.grounds);free(b->scene.locations);free(b->scene.grid);
    free(b->scene.occlusion.tile_cycles);free(b->geometry);free(b->models);free(b->bindings);
    free(b->height_target);free(b->height_start);free(b);
}
SceneTraversal225 *world3d_scene225(SceneBuild225 *b){return b?&b->scene:NULL;}
const SceneGeometry225 *world3d_scene_geometry225(SceneBuild225 *b){return b?b->geometry:NULL;}
Occluder225 *world3d_scene_pool225(SceneBuild225 *b,int32_t level,size_t *count){
    if(!b || level<0 || level>=4 || !count)return NULL;
    *count=b->pool_counts[level];return b->pools[level];
}
static bool current_height(SceneBuild225 *b,const uint8_t key[32],int32_t *height){
    static const uint8_t empty[32]={0};if(!memcmp(key,empty,32)){*height=0;return true;}
    const SceneModel225 *model=world3d_scene_resolve225(b,key);if(!model || !model->model)return false;
    *height=model->bounds.max_y;return true;
}
/* Base model index a non-empty key names, or -1. */
static long base_model_index(const SceneBuild225 *b,const uint8_t key[32]){
    static const uint8_t empty[32]={0};if(!memcmp(key,empty,32))return -1;
    uint32_t id=0;for(int i=0;i<4;i++)id|=(uint32_t)key[i]<<(8*i);
    return id && id<=b->base_models?(long)id-1:-1;
}
static void index_height(SceneBuild225 *b,const uint8_t key[32],int32_t *height,bool fill){
    long m=base_model_index(b,key);if(m<0)return;
    if(fill)b->height_target[b->height_start[m+1]++]=height;else b->height_start[m+2]++;
}
/* After a full refresh: index every height by the base model it copies, so
 * later refreshes update only the heights of models whose bounds changed. */
static void index_heights(SceneBuild225 *b){
    b->height_start=calloc(b->base_models+2,sizeof(*b->height_start));if(!b->height_start)return;
    for(int fill=0;fill<2;fill++){
        for(size_t i=0;i<b->scene.ground_count;i++){
            SceneGround225 *g=&b->scene.grounds[i];
            if(g->wall.present){index_height(b,g->wall.model_a,&g->wall.max_y_a,fill);index_height(b,g->wall.model_b,&g->wall.max_y_b,fill);}
            if(g->decor.present)index_height(b,g->decor.model,&g->decor.max_y,fill);
            if(g->ground_decor.present)index_height(b,g->ground_decor.model,&g->ground_decor.max_y,fill);
        }
        for(size_t i=0;i<b->base_locations;i++)index_height(b,b->scene.locations[i].model,&b->scene.locations[i].max_y,fill);
        if(!fill){
            for(size_t m=0;m<b->base_models;m++)b->height_start[m+2]+=b->height_start[m+1];
            b->height_target=malloc((b->height_start[b->base_models+1]+1)*sizeof(*b->height_target));
            if(!b->height_target){free(b->height_start);b->height_start=NULL;return;}
        }
    }
    b->heights_indexed=true;
}
bool world3d_scene_refresh225(SceneBuild225 *b){
    if(!b)return false;
    for(size_t i=0;i<b->base_models;i++){
        ModelBinding225 *binding=&b->bindings[i];WorldLocation *loc=binding->owner;
        if(!loc || !loc->model || loc->bitset!=binding->bitset || loc->shape!=binding->shape || loc->rotation!=binding->rotation)return false;
        /* Bounds depend only on the model's points; recompute them only when
         * the location was given another model (e.g. an animation frame). */
        if(loc->model==binding->model && loc->model_serial==binding->serial)continue;
        model_bounds(&b->models[i],loc->model,binding->shared,binding->skew);
        binding->model=loc->model;binding->serial=loc->model_serial;
        if(b->heights_indexed)for(size_t k=b->height_start[i];k<b->height_start[i+1];k++)*b->height_target[k]=b->models[i].bounds.max_y;
    }
    /* Indexed: every key resolved at the first refresh and keys never change
     * within a build, so only the heights updated above can differ. */
    if(b->heights_indexed)return true;
    for(size_t i=0;i<b->scene.ground_count;i++){
        SceneGround225 *g=&b->scene.grounds[i];
        if(g->wall.present && (!current_height(b,g->wall.model_a,&g->wall.max_y_a) || !current_height(b,g->wall.model_b,&g->wall.max_y_b)))return false;
        if(g->decor.present && !current_height(b,g->decor.model,&g->decor.max_y))return false;
        if(g->ground_decor.present && !current_height(b,g->ground_decor.model,&g->ground_decor.max_y))return false;
    }
    for(size_t i=0;i<b->base_locations;i++)if(!current_height(b,b->scene.locations[i].model,&b->scene.locations[i].max_y))return false;
    index_heights(b);return true;
}
static bool temporary_model(void *context,const SceneLocation225 *location,uint8_t key[32],int32_t *max_y){
    SceneBuild225 *b=context;
    if(location->id<0 || (size_t)location->id<b->base_locations)return false;
    size_t index=(size_t)location->id-b->base_locations;if(index>=b->temporary_count)return false;
    Temporary225 *temp=&b->temporary[index];if(temp->location!=location->id || !temp->draw)return false;
    const SceneModel225 *model=temp->draw(temp->user);if(!model || !model->model)return false;
    b->models[temp->model]=*model;*max_y=model->bounds.max_y;
    encode_key(key,(uint32_t)temp->model+1,b->temporary_generation);return true;
}
bool world3d_scene_temporary225(SceneBuild225 *b,int32_t level,int32_t x,int32_t y,int32_t z,
    int32_t radius,int32_t yaw,bool expand,int32_t bitset,const SceneModel225 *direct,
    SceneEntityDraw225 draw_entity,void *entity_user){
    if(!b || level<0 || level>=4 || radius<0 || yaw<0 || yaw>=2048)return false;
    if((!direct || !direct->model) && !draw_entity)return true;
    if(b->temporary_count>=5000 || b->model_count>=b->model_capacity)return false;
    int min_x=jint((uint32_t)x-(uint32_t)radius),max_x=jint((uint32_t)x+(uint32_t)radius);
    int min_z=jint((uint32_t)z-(uint32_t)radius),max_z=jint((uint32_t)z+(uint32_t)radius);
    if(expand){
        if(yaw>640 && yaw<1408)max_z=jint((uint32_t)max_z+128u);
        if(yaw>1152 && yaw<1920)max_x=jint((uint32_t)max_x+128u);
        if(yaw>1664 || yaw<384)min_z=jint((uint32_t)min_z-128u);
        if(yaw>128 && yaw<896)min_x=jint((uint32_t)min_x-128u);
    }
    min_x/=128;max_x/=128;min_z/=128;max_z/=128;
    return world3d_scene_temporary_tiles225(b,level,x,y,z,yaw,bitset,direct,draw_entity,entity_user,min_x,max_x,min_z,max_z);
}
bool world3d_scene_temporary_tiles225(SceneBuild225 *b,int32_t level,int32_t x,int32_t y,int32_t z,
    int32_t yaw,int32_t bitset,const SceneModel225 *direct,SceneEntityDraw225 draw_entity,void *entity_user,
    int32_t min_x,int32_t max_x,int32_t min_z,int32_t max_z){
    if(!b || level<0 || level>=4 || yaw<0 || yaw>=2048)return false;
    if((!direct || !direct->model) && !draw_entity)return true;
    if(b->temporary_count>=5000 || b->model_count>=b->model_capacity)return false;
    if(min_x<0 || min_z<0 || max_x>=104 || max_z>=104 || min_x>max_x || min_z>max_z)return false;
    for(int xx=min_x;xx<=max_x;xx++)for(int zz=min_z;zz<=max_z;zz++){
        int id=b->scene.grid[cell(level,xx,zz)];if(id>=0 && b->scene.grounds[id].loc_count>=5)return false;
    }
    int id=(int)b->scene.location_count++,model=(int)b->model_count++;
    Temporary225 *temp=&b->temporary[b->temporary_count++];*temp=(Temporary225){id,model,draw_entity,entity_user};
    SceneLocation225 *loc=&b->scene.locations[id];memset(loc,0,sizeof(*loc));
    loc->id=id;loc->level=level;loc->x=x;loc->y=y;loc->z=z;loc->yaw=yaw;loc->bitset=bitset;
    loc->min_x=min_x;loc->max_x=max_x;loc->min_z=min_z;loc->max_z=max_z;
    loc->entity_resolution=direct && direct->model?(draw_entity?1:0):2;
    if(direct && direct->model){
        b->models[model]=*direct;loc->max_y=direct->bounds.max_y;
        encode_key(loc->model,(uint32_t)model+1,b->temporary_generation);
    }
    for(int xx=min_x;xx<=max_x;xx++)for(int zz=min_z;zz<=max_z;zz++){
        SceneGround225 *g=ensure_below(b,level,xx,zz);
        int span=(xx>min_x?1:0)+(xx<max_x?4:0)+(zz>min_z?8:0)+(zz<max_z?2:0);
        g->locs[g->loc_count]=id;g->loc_span[g->loc_count++]=span;g->loc_spans|=span;
    }
    b->scene.resolve_entity=temporary_model;b->scene.entity_user=b;return true;
}
void world3d_scene_clear_temporary225(SceneBuild225 *b){
    if(!b)return;
    for(size_t i=0;i<b->temporary_count;i++){
        SceneLocation225 *loc=&b->scene.locations[b->temporary[i].location];
        for(int x=loc->min_x;x<=loc->max_x;x++)for(int z=loc->min_z;z<=loc->max_z;z++){
            int id=b->scene.grid[cell(loc->level,x,z)];if(id<0)continue;
            SceneGround225 *g=&b->scene.grounds[id];
            for(int slot=0;slot<g->loc_count;slot++)if(g->locs[slot]==loc->id){
                g->loc_count--;
                memmove(g->locs+slot,g->locs+slot+1,(size_t)(g->loc_count-slot)*sizeof(*g->locs));
                memmove(g->loc_span+slot,g->loc_span+slot+1,(size_t)(g->loc_count-slot)*sizeof(*g->loc_span));break;
            }
            g->loc_spans=0;for(int slot=0;slot<g->loc_count;slot++)g->loc_spans|=g->loc_span[slot];
        }
    }
    for(size_t i=0;i<b->object_count;i++)memset(&b->scene.grounds[b->object_ground[i]].ground_obj,0,sizeof(SceneGroundObject225));
    b->object_count=0;
    memset(b->models+b->base_models,0,(b->model_count-b->base_models)*sizeof(*b->models));
    b->scene.location_count=b->base_locations;b->model_count=b->base_models;b->temporary_count=0;
    if(++b->temporary_generation==0)b->temporary_generation=1;
}
SceneModel225 world3d_scene_model225(Model *model,bool pickable){
    SceneModel225 out={0};if(model)model_bounds(&out,model,false,false);out.pickable=pickable;return out;
}
bool world3d_scene_objects225(SceneBuild225 *b,int32_t level,int32_t x,int32_t y,int32_t z,
    Model *top,Model *bottom,Model *middle){
    if(!b || level<0 || level>=4 || x<0 || x>=104 || z<0 || z>=104 || !top ||
       b->model_count+3>b->model_capacity)return false;
    SceneGround225 *g=ensure(b,level,x,z);SceneGroundObject225 *o=&g->ground_obj;
    if(!o->present){if(b->object_count>=CELLS)return false;b->object_ground[b->object_count++]=g->id;}
    memset(o,0,sizeof(*o));o->present=true;o->x=x*128+64;o->y=y;o->z=z*128+64;
    o->bitset=(int32_t)(UINT32_C(0x60000000)+(uint32_t)x+((uint32_t)z<<7));
    for(int i=0;i<g->loc_count;i++){
        const SceneModel225 *m=world3d_scene_resolve225(b,b->scene.locations[g->locs[i]].model);
        if(m && m->model && m->model->obj_raise>o->offset)o->offset=m->model->obj_raise;
    }
    Model *models[3]={top,bottom,middle};uint8_t *keys[3]={o->top,o->bottom,o->middle};
    for(int i=0;i<3;i++)if(models[i]){
        b->models[b->model_count]=world3d_scene_model225(models[i],true);
        encode_key(keys[i],(uint32_t)++b->model_count,b->temporary_generation);
    }
    return true;
}
