/* Original World3D.drawTile callbacks connected to drawTileUnderlay,
 * drawTileOverlay and Model.draw. Traversal alone chooses the draw order. */
#include "dash3d/scene_raster225.h"
#include <string.h>

typedef struct {
    SceneTraversal225 *scene;
    SceneRaster225 *raster;
    SceneEmit225 observer;
    void *observer_user;
} SceneDraw225;

static bool draw_call(void *user,const SceneCall225 *call){
    SceneDraw225 *draw=user;
    SceneTraversal225 *s=draw->scene;
    SceneRaster225 *r=draw->raster;
    r->failed_call=*call;
    if(draw->observer && !draw->observer(draw->observer_user,call)){
        r->error=5;return false;
    }
    bool ok;
    if(call->kind==SCENE225_MODEL){
        const SceneModel225 *model=r->resolve(r->resolve_user,call->model);
        if(!model || !model->model){r->error=3;return false;}
        ModelPick225 pick={r->picking.check_hover,model->pickable,
            r->picking.mouse_x,r->picking.mouse_y,call->bitset,false};
        ok=model_render_world_bounds_pick225(model->model,&model->bounds,
            call->yaw,r->view.pitch,r->view.yaw,call->dx,call->dy,call->dz,&pick);
        if(pick.picked){
            if(r->picking.count>=r->picking.capacity){r->error=7;return false;}
            r->picking.bitsets[r->picking.count++]=call->bitset;
        }
        r->models++;
    }else{
        if(call->ground_id<0 || (size_t)call->ground_id>=r->geometry_count){
            r->error=2;return false;
        }
        const SceneGeometry225 *geometry=&r->geometry[call->ground_id];
        if(call->kind==SCENE225_UNDERLAY){
            if(!geometry->underlay || call->level<0 || call->level>=s->max_level ||
               call->x<0 || call->x>=s->grid_x || call->z<0 || call->z>=s->grid_z ||
               !s->occlusion.heights){r->error=2;return false;}
            size_t stride=(size_t)s->grid_z+1;
            size_t index=((size_t)call->level*((size_t)s->grid_x+1)+call->x)*stride+call->z;
            const int32_t *height=s->occlusion.heights+index;
            int32_t heights[4]={height[0],height[stride],height[stride+1],height[1]};
            ok=world3d_underlay225(geometry->underlay,heights,call->x,call->z,&r->view);
            r->underlays++;
        }else if(call->kind==SCENE225_OVERLAY && geometry->overlay){
            /* Both ordinary and bridge drawTile overlay calls pass true. */
            ok=world3d_overlay225(geometry->overlay,call->x,call->z,true,&r->view);
            r->overlays++;
        }else{r->error=2;return false;}
    }
    if(!ok){r->error=4;return false;}
    return true;
}

bool world3d_render_scene225(SceneTraversal225 *scene,int32_t root_id,SceneRaster225 *raster){
    if(!raster)return false;
    raster->models=raster->underlays=raster->overlays=0;
    raster->error=0;
    memset(&raster->failed_call,0,sizeof(raster->failed_call));
    if(!scene || !raster->geometry || raster->geometry_count<scene->ground_count ||
       !raster->resolve || raster->picking.count>raster->picking.capacity ||
       (raster->picking.capacity && !raster->picking.bitsets) ||
       scene->grid_x<=0 || scene->grid_z<=0 || scene->max_level<=0 ||
       scene->occlusion.max_x!=scene->grid_x || scene->occlusion.max_z!=scene->grid_z ||
       scene->occlusion.levels!=scene->max_level ||
       raster->view.pitch<0 || raster->view.pitch>=2048 ||
       raster->view.yaw<0 || raster->view.yaw>=2048){raster->error=1;return false;}
    raster->view.eye_x=scene->eye_x;
    raster->view.eye_y=scene->eye_y;
    raster->view.eye_z=scene->eye_z;
    scene->taking_input=raster->view.taking_input;
    SceneDraw225 draw={scene,raster,scene->emit,scene->emit_user};
    scene->emit=draw_call;scene->emit_user=&draw;
    bool ok=world3d_traverse_frame225(scene,root_id);
    raster->view.taking_input=scene->taking_input;
    scene->emit=draw.observer;scene->emit_user=draw.observer_user;
    /* World3D.draw returns normally after its two sweeps even when tiles are
     * deferred. Only an actual traversal failure is fatal; retain its pending
     * count and takingInput state exactly as the original return does. */
    if(!ok){if(!raster->error)raster->error=6;return false;}
    return true;
}
