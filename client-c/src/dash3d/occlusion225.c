/* Bundled revision-225 World3D.java:2028-2424. Isolated from legacy scene. */
#include "dash3d/occlusion225.h"
#include <stdio.h>

static int32_t jint(uint32_t v){return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static int32_t add(int32_t a,int32_t b){return jint((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return jint((uint32_t)a-(uint32_t)b);}
static int32_t neg(int32_t a){return jint(0u-(uint32_t)a);}
static int32_t mul(int32_t a,int32_t b){return jint((uint32_t)a*(uint32_t)b);}
static int32_t shr8(int32_t a){return jint(((uint32_t)a>>8)|(a<0?UINT32_C(0xff000000):0));}
static int32_t slope(int32_t a,int32_t eye,int32_t distance){return jint((uint32_t)sub(a,eye)<<8)/distance;}
static int32_t project(int32_t base,int32_t delta,int32_t distance){return add(base,shr8(mul(delta,distance)));}
static int lo(int a){return a<0?0:a;}
static int hi(int a){return a>50?50:a;}

bool world3d_occlusion_activate225(Occlusion225 *s,const bool map[51][51]){
    if(!s || !map || s->count>500 || (s->count && !s->pool))return false;
    s->active_count=0;
    for(size_t i=0;i<s->count;i++){
        Occluder225 *o=&s->pool[i];int32_t d;bool visible=false;
        if(o->type==1){
            int x=add(sub(o->min_tile_x,s->eye_tile_x),25);
            if(x<0 || x>50)continue;
            int z0=lo(add(sub(o->min_tile_z,s->eye_tile_z),25)),z1=hi(add(sub(o->max_tile_z,s->eye_tile_z),25));
            for(int z=z0;z<=z1;z++)if(map[x][z]){visible=true;break;}
            if(!visible)continue;
            d=sub(s->eye_x,o->min_x);
            if(d>32)o->mode=1;else{if(d>=-32)continue;o->mode=2;d=neg(d);}
            o->min_delta_z=slope(o->min_z,s->eye_z,d);o->max_delta_z=slope(o->max_z,s->eye_z,d);
            o->min_delta_y=slope(o->min_y,s->eye_y,d);o->max_delta_y=slope(o->max_y,s->eye_y,d);
        }else if(o->type==2){
            int z=add(sub(o->min_tile_z,s->eye_tile_z),25);
            if(z<0 || z>50)continue;
            int x0=lo(add(sub(o->min_tile_x,s->eye_tile_x),25)),x1=hi(add(sub(o->max_tile_x,s->eye_tile_x),25));
            for(int x=x0;x<=x1;x++)if(map[x][z]){visible=true;break;}
            if(!visible)continue;
            d=sub(s->eye_z,o->min_z);
            if(d>32)o->mode=3;else{if(d>=-32)continue;o->mode=4;d=neg(d);}
            o->min_delta_x=slope(o->min_x,s->eye_x,d);o->max_delta_x=slope(o->max_x,s->eye_x,d);
            o->min_delta_y=slope(o->min_y,s->eye_y,d);o->max_delta_y=slope(o->max_y,s->eye_y,d);
        }else if(o->type==4){
            d=sub(o->min_y,s->eye_y);if(d<=128)continue;
            int z0=lo(add(sub(o->min_tile_z,s->eye_tile_z),25)),z1=hi(add(sub(o->max_tile_z,s->eye_tile_z),25));
            int x0=lo(add(sub(o->min_tile_x,s->eye_tile_x),25)),x1=hi(add(sub(o->max_tile_x,s->eye_tile_x),25));
            for(int x=x0;x<=x1 && !visible;x++)for(int z=z0;z<=z1;z++)if(map[x][z]){visible=true;break;}
            if(!visible)continue;
            o->mode=5;o->min_delta_x=slope(o->min_x,s->eye_x,d);o->max_delta_x=slope(o->max_x,s->eye_x,d);
            o->min_delta_z=slope(o->min_z,s->eye_z,d);o->max_delta_z=slope(o->max_z,s->eye_z,d);
        }else continue;
        s->active[s->active_count++]=(uint16_t)i;
    }
    return true;
}
bool world3d_occluded225(const Occlusion225 *s,int32_t x,int32_t y,int32_t z){
    for(size_t i=0;i<s->active_count;i++){
        const Occluder225 *o=&s->pool[s->active[i]];int32_t d,a,b,c,e;
        if(o->mode==1 || o->mode==2){
            d=o->mode==1?sub(o->min_x,x):sub(x,o->min_x);if(d<=0)continue;
            a=project(o->min_z,o->min_delta_z,d);b=project(o->max_z,o->max_delta_z,d);
            c=project(o->min_y,o->min_delta_y,d);e=project(o->max_y,o->max_delta_y,d);
            if(z>=a && z<=b && y>=c && y<=e)return true;
        }else if(o->mode==3 || o->mode==4){
            d=o->mode==3?sub(o->min_z,z):sub(z,o->min_z);if(d<=0)continue;
            a=project(o->min_x,o->min_delta_x,d);b=project(o->max_x,o->max_delta_x,d);
            c=project(o->min_y,o->min_delta_y,d);e=project(o->max_y,o->max_delta_y,d);
            if(x>=a && x<=b && y>=c && y<=e)return true;
        }else if(o->mode==5){
            d=sub(y,o->min_y);if(d<=0)continue;
            a=project(o->min_x,o->min_delta_x,d);b=project(o->max_x,o->max_delta_x,d);
            c=project(o->min_z,o->min_delta_z,d);e=project(o->max_z,o->max_delta_z,d);
            if(x>=a && x<=b && z>=c && z<=e)return true;
        }
    }
    return false;
}
static size_t cell(const Occlusion225 *s,int l,int x,int z){return ((size_t)l*(s->max_x+1)+x)*(s->max_z+1)+z;}
static int32_t height(const Occlusion225 *s,int l,int x,int z){return s->heights[((size_t)l*(s->max_x+1)+x)*(s->max_z+1)+z];}
static bool corners(Occlusion225 *s,int l,int x,int z,int32_t h){
    int xx=x*128,zz=z*128;
    return world3d_occluded225(s,xx+1,sub(height(s,l,x,z),h),zz+1) &&
        world3d_occluded225(s,xx+127,sub(height(s,l,x+1,z),h),zz+1) &&
        world3d_occluded225(s,xx+127,sub(height(s,l,x+1,z+1),h),zz+127) &&
        world3d_occluded225(s,xx+1,sub(height(s,l,x,z+1),h),zz+127);
}
bool world3d_tile_occluded225(Occlusion225 *s,int32_t l,int32_t x,int32_t z){
    int32_t *cache=&s->tile_cycles[cell(s,l,x,z)];
    if(*cache==neg(s->cycle))return false;
    if(*cache==s->cycle)return true;
    bool result=corners(s,l,x,z,0);*cache=result?s->cycle:neg(s->cycle);return result;
}
bool world3d_object_occluded225(Occlusion225 *s,int32_t l,int32_t x,int32_t z,int32_t h){
    return world3d_tile_occluded225(s,l,x,z) && corners(s,l,x,z,h);
}
bool world3d_loc_occluded225(Occlusion225 *s,int32_t l,int32_t x0,int32_t x1,int32_t z0,int32_t z1,int32_t h){
    if(x0==x1 && z0==z1)return world3d_tile_occluded225(s,l,x0,z0) && corners(s,l,x0,z0,h);
    for(int x=x0;x<=x1;x++)for(int z=z0;z<=z1;z++)if(s->tile_cycles[cell(s,l,x,z)]==neg(s->cycle))return false;
    int a=x0*128+1,b=z0*128+2,c=x1*128-1,d=z1*128-1;int32_t y=sub(height(s,l,x0,z0),h);
    return world3d_occluded225(s,a,y,b) && world3d_occluded225(s,c,y,b) && world3d_occluded225(s,a,y,d) && world3d_occluded225(s,c,y,d);
}
bool world3d_wall_occluded225(Occlusion225 *s,int32_t l,int32_t x,int32_t z,int32_t type){
    if(!world3d_tile_occluded225(s,l,x,z))return false;
    int xx=x*128,zz=z*128;int32_t y=sub(height(s,l,x,z),1),y120=sub(y,120),y230=sub(y,230),y238=sub(y,238);
    if(type==1 || type==2 || type==4 || type==8){
        int ax=xx,az=zz,bx=xx,bz=zz;bool front=false;
        if(type==1){bz+=128;front=xx>s->eye_x;}
        if(type==2){az+=128;bx+=128;bz+=128;front=zz<s->eye_z;}
        if(type==4){ax+=128;bx+=128;bz+=128;front=xx<s->eye_x;}
        if(type==8){bx+=128;front=zz>s->eye_z;}
        if(front && (!world3d_occluded225(s,ax,y,az) || !world3d_occluded225(s,bx,y,bz)))return false;
        if(l>0 && (!world3d_occluded225(s,ax,y120,az) || !world3d_occluded225(s,bx,y120,bz)))return false;
        return world3d_occluded225(s,ax,y230,az) && world3d_occluded225(s,bx,y230,bz);
    }
    if(!world3d_occluded225(s,xx+64,y238,zz+64))return false;
    if(type==16)return world3d_occluded225(s,xx,y230,zz+128);
    if(type==32)return world3d_occluded225(s,xx+128,y230,zz+128);
    if(type==64)return world3d_occluded225(s,xx+128,y230,zz);
    if(type==128)return world3d_occluded225(s,xx,y230,zz);
    puts("Warning unsupported wall type");return true;
}
