/* Bundled World3D.draw: compose verified camera selection and activation. */
#include "dash3d/scene_prepare225.h"
#include "dash3d/scene_selection225.h"
#include <stdlib.h>

bool world3d_prepare_scene225(SceneTraversal225 *s,const Visibility225 *visibility,
    int32_t eye_x,int32_t eye_y,int32_t eye_z,int32_t pitch,int32_t yaw,int32_t top_level){
    if(!s || !visibility || !s->grid || !s->grounds || !s->occlusion.heights ||
       s->grid_x<1 || s->grid_x>104 || s->grid_z<1 || s->grid_z>104 ||
       s->max_level<1 || s->max_level>4)return false;
    size_t count=(size_t)s->max_level*s->grid_x*s->grid_z;
    SceneTileState225 *tiles=calloc(count,sizeof(*tiles));if(!tiles)return false;
    bool valid=true;
    for(size_t index=0;index<count;index++){
        int id=s->grid[index];if(id==-1)continue;
        if(id<0 || (size_t)id>=s->ground_count){valid=false;break;}
        SceneGround225 *g=&s->grounds[id];
        int z=(int)(index%s->grid_z),x=(int)(index/s->grid_z%s->grid_x);
        int l=(int)(index/((size_t)s->grid_x*s->grid_z));
        size_t height=((size_t)l*(s->grid_x+1)+x)*(s->grid_z+1)+z;
        tiles[index]=(SceneTileState225){true,g->draw_level,s->occlusion.heights[height],
            g->loc_count,g->visible,g->update,g->contains_locs,g->check_loc_spans};
    }
    SceneGrid225 grid={s->grid_x,s->grid_z,s->max_level,s->min_level,tiles};
    SceneSelection225 view={0};view.cycle=s->cycle;
    bool ok=valid && world3d_select_tiles225(&grid,visibility,&view,
        eye_x,eye_y,eye_z,pitch,yaw,top_level);
    if(ok){
        s->cycle=view.cycle;s->eye_x=view.eye_x;s->eye_y=view.eye_y;s->eye_z=view.eye_z;
        s->eye_tile_x=view.eye_tile_x;s->eye_tile_z=view.eye_tile_z;s->top_level=top_level;
        s->min_x=view.min_x;s->max_x=view.max_x;s->min_z=view.min_z;s->max_z=view.max_z;
        s->tiles_remaining=view.tiles_remaining;
        Occlusion225 *o=&s->occlusion;
        o->eye_x=s->eye_x;o->eye_y=s->eye_y;o->eye_z=s->eye_z;
        o->eye_tile_x=s->eye_tile_x;o->eye_tile_z=s->eye_tile_z;o->cycle=s->cycle;
        o->max_x=s->grid_x;o->max_z=s->grid_z;o->levels=s->max_level;
        ok=world3d_occlusion_activate225(o,visibility->matrix[view.map_pitch][view.map_yaw]);
    }
    if(ok)for(size_t index=0;index<count;index++)if(tiles[index].present){
        SceneGround225 *g=&s->grounds[s->grid[index]];
        g->visible=tiles[index].visible;g->update=tiles[index].update;
        g->contains_locs=tiles[index].contains_locs;g->check_loc_spans=tiles[index].check_loc_spans;
    }
    free(tiles);return ok;
}
