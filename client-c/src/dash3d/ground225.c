/* Bundled revision-225 World3D.java drawTileUnderlay/drawTileOverlay,
 * mulLightness and pointInsideTriangle. Separate from installed traversal. */
#include "dash3d/ground225.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/pix3d_gouraud225.h"
#include "gfx/pix3d_texture225.h"
#include "gfx/pix3d_texture_cache225.h"

const int32_t world3d_texture_hsl225[50]={41,39248,41,4643,41,41,41,41,41,41,41,41,41,41,41,43086,41,41,41,41,41,41,41,8602,41,28992,41,41,41,41,41,5056,41,41,41,41,41,41,41,41,41,41,41,41,41,41,3131,41,41,41};
static int32_t jint(uint32_t v){return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static int32_t add(int32_t a,int32_t b){return jint((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return jint((uint32_t)a-(uint32_t)b);}
static int32_t mul(int32_t a,int32_t b){return jint((uint32_t)a*(uint32_t)b);}
static int32_t shr16(int32_t a){return jint(((uint32_t)a>>16)|(a<0?UINT32_C(0xffff0000):0));}
static int32_t sum(int32_t a,int32_t b,int32_t c,int32_t d){return shr16(add(mul(a,b),mul(c,d)));}
static int32_t diff(int32_t a,int32_t b,int32_t c,int32_t d){return shr16(sub(mul(a,b),mul(c,d)));}
static int32_t project(int32_t v,int32_t z){return jint((uint32_t)v<<9)/z;}
static int32_t cross(int32_t ax,int32_t bx,int32_t cx,int32_t ay,int32_t by,int32_t cy){
    return sub(mul(sub(ax,bx),sub(cy,by)),mul(sub(ay,by),sub(cx,bx)));
}

int32_t world3d_ground_lightness225(int32_t shade,int32_t hsl){
    int32_t light=mul(sub(127,shade),hsl&0x7f)/160;
    if(light<2)light=2;else if(light>126)light=126;
    return add(hsl&0xff80,light);
}
bool world3d_point_inside_triangle225(int32_t mx,int32_t my,int32_t ya,int32_t yb,int32_t yc,
    int32_t xa,int32_t xb,int32_t xc){
    if(my<ya && my<yb && my<yc)return false;
    if(my>ya && my>yb && my>yc)return false;
    if(mx<xa && mx<xb && mx<xc)return false;
    if(mx>xa && mx>xb && mx>xc)return false;
    int32_t a=sub(mul(sub(my,ya),sub(xb,xa)),mul(sub(mx,xa),sub(yb,ya)));
    int32_t b=sub(mul(sub(my,yc),sub(xa,xc)),mul(sub(mx,xc),sub(ya,yc)));
    int32_t c=sub(mul(sub(my,yb),sub(xc,xb)),mul(sub(mx,xb),sub(yc,yb)));
    return mul(a,c)>0 && mul(c,b)>0;
}

typedef struct {int32_t x,y,z,sx,sy;} GroundVertex225;
static GroundVertex225 transform(int32_t x,int32_t y,int32_t z,const Ground225 *view){
    int32_t sw=pix3d_sin_table[view->yaw&2047],cw=pix3d_cos_table[view->yaw&2047];
    int32_t sp=pix3d_sin_table[view->pitch&2047],cp=pix3d_cos_table[view->pitch&2047];
    x=sub(x,view->eye_x);y=sub(y,view->eye_y);z=sub(z,view->eye_z);
    int32_t xx=sum(z,sw,x,cw),zz=diff(z,cw,x,sw),yy=diff(y,cp,zz,sp);
    zz=sum(y,sp,zz,cp);
    GroundVertex225 v={xx,yy,zz,0,0};
    if(zz>=50){v.sx=add(pix3d_center_x,project(xx,zz));v.sy=add(pix3d_center_y,project(yy,zz));}
    return v;
}
static bool triangle(const GroundVertex225 *v,int a,int b,int c,int32_t ca,int32_t cb,int32_t cc,
    int texture,int ta,int tb,int tc,int tx,int tz,Ground225 *view){
    if(cross(v[a].sx,v[b].sx,v[c].sx,v[a].sy,v[b].sy,v[c].sy)<=0)return true;
    pix3d_set_hclip(v[a].sx<0 || v[b].sx<0 || v[c].sx<0 || v[a].sx>pix2d_safe_width || v[b].sx>pix2d_safe_width || v[c].sx>pix2d_safe_width);
    if(view->taking_input && world3d_point_inside_triangle225(view->mouse_x,view->mouse_y,
        v[a].sy,v[b].sy,v[c].sy,v[a].sx,v[b].sx,v[c].sx)){
        view->click_tile_x=tx;view->click_tile_z=tz;
    }
    if(texture==-1){
        if(ca!=12345678)pix3d225_gouraud_triangle(v[a].sx,v[b].sx,v[c].sx,v[a].sy,v[b].sy,v[c].sy,ca,cb,cc);
    }else{
        if(texture<0 || texture>=50)return false;
        if(view->low_memory){
            ca=world3d_ground_lightness225(ca,world3d_texture_hsl225[texture]);
            cb=world3d_ground_lightness225(cb,world3d_texture_hsl225[texture]);
            cc=world3d_ground_lightness225(cc,world3d_texture_hsl225[texture]);
            pix3d225_gouraud_triangle(v[a].sx,v[b].sx,v[c].sx,v[a].sy,v[b].sy,v[c].sy,ca,cb,cc);
        }else{
            bool opaque=false;const uint32_t *texels=pix3d225_texture_cache_get(texture,&opaque);if(!texels)return false;
            int32_t args[18]={v[a].sy,v[b].sy,v[c].sy,v[a].sx,v[b].sx,v[c].sx,ca,cb,cc,
                v[ta].x,v[tb].x,v[tc].x,v[ta].y,v[tb].y,v[tc].y,v[ta].z,v[tb].z,v[tc].z};
            pix3d225_texture_triangle(texels,opaque,args);
        }
    }
    return true;
}
bool world3d_underlay225(const TileUnderlay *tile,const int32_t heights[4],int32_t tx,int32_t tz,Ground225 *view){
    if(!tile || !heights || !view || !pix3d_scanline)return false;
    int32_t x=jint((uint32_t)tx<<7),z=jint((uint32_t)tz<<7);
    GroundVertex225 v[4]={transform(x,heights[0],z,view),transform(add(x,128),heights[1],z,view),
        transform(add(x,128),heights[2],add(z,128),view),transform(x,heights[3],add(z,128),view)};
    for(int i=0;i<4;i++)if(v[i].z<50)return true;
    pix3d_set_trans(0);
    if(!triangle(v,2,3,1,tile->northeast_color,tile->northwest_color,tile->southeast_color,tile->texture_id,
        tile->flat?0:2,tile->flat?1:3,tile->flat?3:1,tx,tz,view))return false;
    return triangle(v,0,1,3,tile->southwest_color,tile->southeast_color,tile->northwest_color,tile->texture_id,0,1,3,tx,tz,view);
}
bool world3d_overlay225(const TileOverlay *tile,int32_t tx,int32_t tz,bool draw,Ground225 *view){
    if(!tile || !view || !pix3d_scanline || tile->vertex_count<0 || tile->vertex_count>6 || tile->triangle_count<0 || tile->triangle_count>10)return false;
    GroundVertex225 v[6];
    for(int i=0;i<tile->vertex_count;i++){
        v[i]=transform(tile->vertex_x[i],tile->vertex_y[i],tile->vertex_z[i],view);
        if(v[i].z<50)return true;
    }
    pix3d_set_trans(0);if(!draw)return true;
    for(int i=0;i<tile->triangle_count;i++){
        int a=tile->triangle_vertex_a[i],b=tile->triangle_vertex_b[i],c=tile->triangle_vertex_c[i];
        if(a<0 || b<0 || c<0 || a>=tile->vertex_count || b>=tile->vertex_count || c>=tile->vertex_count)return false;
        if(tile->flat && tile->vertex_count<4)return false;
        if(!triangle(v,a,b,c,tile->triangle_color_a[i],tile->triangle_color_b[i],tile->triangle_color_c[i],tile->triangle_texture[i],
            tile->flat?0:a,tile->flat?1:b,tile->flat?3:c,tx,tz,view))return false;
    }
    return true;
}
