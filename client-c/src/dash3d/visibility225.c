/* Original bundled revision-225 World3D.java:1088 init / :1162 testPoint. */
#include "dash3d/visibility225.h"
#include "gfx/pix3d.h"
#include <stdlib.h>

static int32_t jint(uint32_t v){return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static int32_t add(int32_t a,int32_t b){return jint((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return jint((uint32_t)a-(uint32_t)b);}
static int32_t mul(int32_t a,int32_t b){return jint((uint32_t)a*(uint32_t)b);}
static int32_t shr16(int32_t a){return jint(((uint32_t)a>>16)|(a<0?UINT32_C(0xffff0000):0));}
static int32_t project(int32_t v,int32_t depth){return jint((uint32_t)v<<9)/depth;}

bool world3d_visibility_point225(const VisibilityProjection225 *p,int32_t x,int32_t z,int32_t y){
    int32_t xx=shr16(add(mul(z,p->sin_yaw),mul(x,p->cos_yaw)));
    int32_t zz=shr16(sub(mul(z,p->cos_yaw),mul(x,p->sin_yaw)));
    int32_t depth=shr16(add(mul(y,p->sin_pitch),mul(zz,p->cos_pitch)));
    int32_t yy=shr16(sub(mul(y,p->cos_pitch),mul(zz,p->sin_pitch)));
    if(depth<50 || depth>3500)return false;
    int32_t sx=add(p->center_x,project(xx,depth)),sy=add(p->center_y,project(yy,depth));
    return sx>=p->left && sx<=p->right && sy>=p->top && sy<=p->bottom;
}

bool world3d_visibility_init225(Visibility225 *state,const int32_t distance[9],
    int32_t above,int32_t width,int32_t height,int32_t below){
    if(!state || !distance || above<0 || above>32767 || below<0 || below>32767 || width<0 || height<0)return false;
    bool (*samples)[32][53][53]=calloc(9,sizeof(*samples));if(!samples)return false;
    VisibilityProjection225 *p=&state->projection;
    p->left=0;p->top=0;p->right=width;p->bottom=height;p->center_x=width/2;p->center_y=height/2;
    for(int pitch=128;pitch<=384;pitch+=32)for(int yaw=0;yaw<2048;yaw+=64){
        p->sin_pitch=pix3d_sin_table[pitch];p->cos_pitch=pix3d_cos_table[pitch];
        p->sin_yaw=pix3d_sin_table[yaw];p->cos_yaw=pix3d_cos_table[yaw];
        int pi=(pitch-128)/32,yi=yaw/64;
        for(int x=-26;x<=26;x++)for(int z=-26;z<=26;z++){
            bool visible=false;
            for(int y=-below;y<=above;y+=128){
                if(world3d_visibility_point225(p,x*128,z*128,add(distance[pi],y))){visible=true;break;}
            }
            samples[pi][yi][x+26][z+26]=visible;
        }
    }
    for(int pi=0;pi<8;pi++)for(int yi=0;yi<32;yi++)for(int x=-25;x<25;x++)for(int z=-25;z<25;z++){
        bool visible=false;
        for(int dx=-1;dx<=1 && !visible;dx++)for(int dz=-1;dz<=1;dz++){
            int xx=x+dx+26,zz=z+dz+26,next=(yi+1)%31;
            if(samples[pi][yi][xx][zz] || samples[pi][next][xx][zz] ||
                samples[pi+1][yi][xx][zz] || samples[pi+1][next][xx][zz]){visible=true;break;}
        }
        state->matrix[pi][yi][x+25][z+25]=visible;
    }
    free(samples);return true;
}
