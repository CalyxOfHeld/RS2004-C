/* Bundled revision-225 World3D.java:1196-1265, before drawTile traversal. */
#include "dash3d/scene_selection225.h"
#include "gfx/pix3d.h"
#include <stddef.h>

static int32_t jint(uint32_t v){return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static int32_t subtract(int32_t a,int32_t b){return jint((uint32_t)a-(uint32_t)b);}

bool world3d_select_tiles225(const SceneGrid225 *g,const Visibility225 *visibility,
    SceneSelection225 *v,int32_t eye_x,int32_t eye_y,int32_t eye_z,
    int32_t pitch,int32_t yaw,int32_t top_level){
    if(!g || !g->tiles || !visibility || !v || g->max_x<1 || g->max_x>104 || g->max_z<1 || g->max_z>104 ||
        g->max_level<1 || g->max_level>4 || g->min_level<0 || g->min_level>g->max_level ||
        pitch<97 || pitch>383 || yaw<0 || yaw>2047 || top_level<0 || top_level>=g->max_level)return false;
    if(eye_x<0)eye_x=0;else if(eye_x>=g->max_x*128)eye_x=g->max_x*128-1;
    if(eye_z<0)eye_z=0;else if(eye_z>=g->max_z*128)eye_z=g->max_z*128-1;
    v->cycle=jint((uint32_t)v->cycle+1);
    v->sin_pitch=pix3d_sin_table[pitch];v->cos_pitch=pix3d_cos_table[pitch];
    v->sin_yaw=pix3d_sin_table[yaw];v->cos_yaw=pix3d_cos_table[yaw];
    v->map_pitch=(pitch-128)/32;v->map_yaw=yaw/64;
    v->eye_x=eye_x;v->eye_y=eye_y;v->eye_z=eye_z;
    v->eye_tile_x=eye_x/128;v->eye_tile_z=eye_z/128;v->top_level=top_level;
    v->min_x=v->eye_tile_x-25;if(v->min_x<0)v->min_x=0;
    v->min_z=v->eye_tile_z-25;if(v->min_z<0)v->min_z=0;
    v->max_x=v->eye_tile_x+25;if(v->max_x>g->max_x)v->max_x=g->max_x;
    v->max_z=v->eye_tile_z+25;if(v->max_z>g->max_z)v->max_z=g->max_z;
    v->tiles_remaining=0;
    for(int level=g->min_level;level<g->max_level;level++)for(int x=v->min_x;x<v->max_x;x++)for(int z=v->min_z;z<v->max_z;z++){
        SceneTileState225 *t=&g->tiles[((size_t)level*g->max_x+x)*g->max_z+z];
        if(!t->present)continue;
        bool on_map=visibility->matrix[v->map_pitch][v->map_yaw][x-v->eye_tile_x+25][z-v->eye_tile_z+25];
        if(t->draw_level<=top_level && (on_map || subtract(t->height,eye_y)>=2000)){
            t->visible=true;t->update=true;t->contains_locs=t->loc_count>0;v->tiles_remaining++;
        }else{
            t->visible=false;t->update=false;t->check_loc_spans=0;
        }
    }
    return true;
}
